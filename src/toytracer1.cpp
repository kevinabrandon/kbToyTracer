/***************************************************************************
* toytracer.C                                                              *
*                                                                          *
* This is the source file for a "Toy" ray tracer.  It defines most of the  *
* fundamental functions using in ray tracing; in particular, "Trace" and   *
* "Cast" ("Shade" is defined in a separate file).  It also defines the     *
* MakeImage function, which casts all the rays from the eye.               *
*                                                                          *
* History:                                                                 *
*	10/22/2004	Kevin Brandon - added timing to the MakeImage()			   *
*   10/16/2004  Check for "ignored" object in "Cast".                      *
*   09/29/2004  Updated for Fall 2004 class.                               *
*   04/14/2003  Point lights are now point objects with emission.          *
*   04/01/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#include "toytracer.h"
#include "Image.h"
#include "kbConfig.h"
#include <chrono>

extern bool bDisplayMessages;

static unsigned long
			NumberOfRays = 0;

// Trace is the most fundamental of all the ray tracing functions.  It
// answers the query "What color do I see looking along the given ray
// in the current scene?"  This is an inherently recursive process, as
// trace may again be called as a result of the ray hitting a reflecting
// object.  To prevent the possibility of infinite recursion, a maximum
// depth is placed on the resulting ray tree.

Color Trace( const Ray &ray, const Scene &scene, int max_tree_depth )
{
    Color   color;               // The color to return.
    HitInfo hitinfo;             // Holds info to pass to shader.
    hitinfo.ignore = NULL;       // Don't ignore any objects.
    hitinfo.distance = Infinity; // Follow the full ray.

    if( max_tree_depth > 0 && Cast( ray, scene, hitinfo ) )
    {
        // The ray hits an object, so shade the point that the ray hit.
        // Cast has put all necessary information for Shade in "hitinfo".
        color = Shade( hitinfo, scene, max_tree_depth - 1 );
    }
    else
    {
        // Either the ray has failed to hit anything, or
        // the recursion has bottomed out.
		if(ray.type != primary_ray && !Config.bg_escaped_rays)
		{
			color = Color(0,0,0);   // GI-era behavior: secondary rays see black
		}
		else
		{
			color = scene.bgcolor;  // class-era behavior when bg_escaped_rays is on
		}
	}
    
    return color;
}

// Cast finds the first point of intersection (if there is one)
// between a ray and a list of geometric objects.  If no intersection
// exists, the function returns false.  Information about the
// closest object hit is returned in "hitinfo".  Note that the single
// object associated with the scene wil typically be an aggregate (so
// that the scene can contain many objects).

bool Cast( const Ray &ray, const Scene &scene, HitInfo &hitinfo )
{
	NumberOfRays++;  // racy under OpenMP by design: a cosmetic stat, not worth the atomic contention
    if( scene.object == NULL || scene.object == hitinfo.ignore ) return false;
    return scene.object->Intersect( ray, hitinfo );
	
}

// This is a trivial tone mapper; it merely maps values that are
// in [0,1] and maps them to integers between 0 and 255.  If the
// real value is above 1, it merely truncates.  A true tone mapper
// would attempt to handle very large values nicely, without
// truncation; that is, it would try to compensate for the fact that
// displays have a very limited dynamic range.

Pixel ToneMap( const Color &color )
    {
    int red   = (int)floor( 256 * color.red   );
    int green = (int)floor( 256 * color.green );
    int blue  = (int)floor( 256 * color.blue  );
    channel r = (channel)( red   >= 255 ? 255 : red   ); 
    channel g = (channel)( green >= 255 ? 255 : green ); 
    channel b = (channel)( blue  >= 255 ? 255 : blue  );
    return Pixel( r, g, b );
    }

// MakeImage casts all the initial rays starting from the eye.
// This trivial version simply casts one ray per pixel, in raster
// order, then writes the pixels out to a file.
bool MakeImage( const Camera &cam, const Lens &lens, const Scene &scene, int xres, int yres, const char *fname )
    {
    Image I( xres, yres );  // Create an image of the given resolution.
    Ray ray;

    ray.origin = cam.eye;     // All initial rays originate from the eye.
    ray.type   = primary_ray; // Only rays generated here are "primary".

    Vec3 G  = Unit( cam.lookat - cam.eye );  // Gaze direction.
    Vec3 U  = Unit( cam.up / G );            // Up vector.
    Vec3 R  = Unit( G ^ U );                 // Right vector.

	int smallRes;
	if( yres < xres )	smallRes = yres;
	else				smallRes = xres;
    Vec3 dU = U * ( 2.0 / ( smallRes - 1 ) );    // Up increments.
    Vec3 dR = R * ( 2.0 / ( smallRes - 1 ) );    // Right increments.
    Vec3 O  = ( cam.vpdist * G ) - (xres/(double)smallRes)*R + (yres/(double)smallRes)*U;    // "Origin" for the raster.
//	Vec3 O  = ( cam.vpdist * G ) - R + U;    // "Origin" for the raster.
//	Vec3 dU = U * ( 2.0 / ( yres - 1 ) );    // Up increments.
//	Vec3 dR = R * ( 2.0 / ( xres - 1 ) );    // Right increments.

	auto liStart = std::chrono::high_resolution_clock::now();

	// Render rows in parallel. Each row is an independent unit of work; "ray" is
	// firstprivate so every thread gets its own scratch copy (origin/type already
	// set) rather than racing on a shared one. "dynamic" scheduling load-balances
	// the uneven per-row cost (busy teapot rows vs. empty background rows).
	#pragma omp parallel for schedule(dynamic) firstprivate(ray)
	for( int i = 0; i < yres; i++ )
	{
		for( int j = 0; j < xres; j++ )
		{
			if ( Config.enable_adaptive_super )
			{
				I(i,j) = ToneMap ( AdaptaveSuper( i, j, dU, dR, O, ray, scene, 0.01, true ) );
			}
			else if( Config.enable_supersample )
			{
				Color color( 0, 0, 0 );
				for(int k = 0; k < Config.numSampsLarge; k++)
				{
					for(int l = 0; l < Config.numSampsLarge; l++)
					{
						if ( Config.enable_sochastic_super )
						{
							double x = rand( 0, 1 );
							double y = rand( 0, 1 );
							ray.direction = Unit( ( O + j * dR - i * dU ) + ( (k + x) * ( dR / Config.numSampsLarge ) ) - ( ( l + y ) * (dU / Config.numSampsLarge ) ) );
							color = color + Trace( ray, scene, Config.numBounces );
						}
						else
						{
							// uniform supersampling
							ray.direction = Unit( ( O + j * dR - i * dU ) + ( k * ( dR / Config.numSampsLarge ) ) - ( l * (dU / Config.numSampsLarge ) ) );
							color = color + Trace( ray, scene, Config.numBounces );
						}
					}
				}
				color = color / ( Config.numSampsLarge * Config.numSampsLarge );
				I(i,j) = ToneMap( color );
			}
			else if( Config.enable_camera_lens && lens.lensRadius )
			{
				Color pixColor( 0.0, 0.0, 0.0 );
				for( int k = 0; k < Config.lensSamps; k++ )
				{
					for( int l = 0; l < Config.lensSamps; l++ )
					{
						Vec3 pointOnFocalPlane = cam.eye + ( lens.focalDist * Unit( O + j * dR - i * dU  ) + ( k * ( dR / Config.numSampsLarge ) ) - ( l * (dU / Config.numSampsLarge ) ) );
						double x = rand( -lens.lensRadius, lens.lensRadius );
						double y = rand( -lens.lensRadius, lens.lensRadius );
						Ray lensRay;
						lensRay.origin = cam.eye + U * x + R * y;
						lensRay.direction = Unit(pointOnFocalPlane - lensRay.origin ); 
						pixColor += Trace( lensRay, scene, Config.numBounces );		
					}
				}
				
				I(i,j) = ToneMap( pixColor / ( Config.lensSamps * Config.lensSamps ) );
			}
			else
			{
				ray.direction = Unit( O + j * dR - i * dU  );
				I(i,j) = ToneMap( Trace( ray, scene, Config.numBounces ) );
			}
		}
		if( bDisplayMessages && i % 10 == 0 ) printf( "\r\t%6.2f%c Complete", ( ( i / ( double ) smallRes ) * 100.0 ), '%' );
	}

	if( bDisplayMessages )	
	{
		std::cout << "\r\t100.00% Complete" << std::endl << std::endl;
	
		auto liFinish = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double>( liFinish - liStart ).count();
		if( time > 60 )
		{
			time = time / 60;
			std::cout << "Time to make image: " << time << " (min)" << std::endl;
		}
		else	std::cout << "Time to make image: " << time << " (sec)" << std::endl;
		std::cout << "Number of rays casted: " << NumberOfRays << std::endl;
	}

	// Save the image to a file.
    if( !I.Write( fname ) )
	{
		printf( "Could not save file!\n" );	
	} 
    
	return true;
    }

Color AdaptaveSuper(int i, int j, Vec3 dU, Vec3 dR, Vec3 O, Ray ray, const Scene &scene, double thr, bool first)
{
	Color average;
	if ( first )
	{
		// this is the first call to AdaptaveSuper()
		// send four rays through corners of pixel
	
		/*
		Color color[ Config.numSampsSmall * Config.numSampsSmall ];
		Color sum(0, 0, 0);
		for( int k = 0; k < Config.numSampsSmall; k++ )
		{
			for( int l = 0; l < Config.numSampsSmall; l++ )
			{
				ray.direction = Unit( ( O + j * dR - i * dU ) + ( k * ( dR / Config.numSampsSmall ) ) - ( l * (dU / Config.numSampsSmall ) ) );
				sum += color[  l + Config.numSampsSmall * k ] = Trace ( ray, scene, Config.numBounces );
			}
		}
		*/
		Color color[ 4 ];
		Color sum( 0, 0, 0 );
		for( int k = 0; k < 2; k++ )
		{
			for (int l = 0; l < 2; l++ )
			{
				ray.direction = Unit ( O + ( j + k ) * dR - ( i + l ) * dU );
				sum += color[ 2 * k + l ] = Trace( ray, scene, Config.numBounces );
			}
		}

		//ray.direction = Unit( O + j * dR - i * dU - dR / 2 + dU / 2 );
		//sum += color [ 4 ] = Trace( ray, scene, Config.numBounces );
		/*
		ray.direction = Unit( O + j * dR - i * dU  );
		sum += color[ 0 ] = Trace( ray, scene, Config.numBounces );
		ray.direction = Unit( O + ( j + 1 ) * dR - i * dU ) ;
		sum += color[ 1 ] = Trace( ray, scene, Config.numBounces );
		ray.direction = Unit( O + j * dR - ( i + 1 ) * dU );
		sum += color[ 2 ] = Trace( ray, scene, Config.numBounces );
		ray.direction = Unit( O + ( j + 1 ) * dR - (i + 1) * dU );
		sum += color[ 3 ] = Trace( ray, scene, Config.numBounces ); 
		*/
		average = sum / 4;
		
		bool diff = false;
		for ( int n = 0; n < 4; n++ )
		{
			if ( !CompareColor( color[ n ], average, thr ) )
			{
				// the four samples are not close enough... we need to get more
				// recursivly call AdaptaveSamp
				return AdaptaveSuper( i, j, dU, dR, O, ray, scene, thr, false );

			}
			else
			{
				// The 4 samples are sufficent
				return average;
			}
		}

	}
	else
	{
		// this is the recursive call
		Color sum( 0, 0, 0 );
		for(int k = 0; k < Config.numSampsLarge; k++)
		{
			for(int l = 0; l < Config.numSampsLarge; l++)
			{
				// uniform supersampling
				ray.direction = Unit( ( O + j * dR - i * dU ) + ( k * ( dR / Config.numSampsLarge ) ) - ( l * (dU / Config.numSampsLarge ) ) );
				sum += Trace( ray, scene, Config.numBounces );
			}
		}
		average = sum / ( Config.numSampsLarge * Config.numSampsLarge );
	}
	return average;
}

bool CompareColor( const Color &A, const Color &B, const double thr )
{
	if ( A.blue > B.blue + thr || A.blue < B.blue - thr ) 
	{
		return false;
	}
	if ( A.green > B.green + thr || A.green < B.green - thr ) 
	{
		return false;
	}
	if ( A.red > B.red + thr || A.red < B.red - thr ) 
	{
		return false;
	}
	return true;
}
