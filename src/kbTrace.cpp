/***************************************************************************
* kbTrace.cpp                                                              *
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
#include "kbRandom.h"
#include "kbPng.h"
#include <chrono>
#include <cmath>


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
    // Backstop: a NaN channel would cast to INT_MIN below and slip past the
    // clamps as pure black, hiding the bug as pepper noise. Flag it magenta
    // so a shading NaN is loud in the image instead of a black speck.
    if( !std::isfinite( color.red + color.green + color.blue ) )
        return Pixel( 255, 0, 255 );
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

	// The scene decides WHAT is rendered, the config decides HOW WELL:
	// depth of field activates simply because the scene defined a lens
	// (dof_samples = 0 turns it off; it also controls the quality).
	const bool useDof = ( lens.lensRadius > 0 && lens.focalDist > 0 && Config.dof_samples > 0 );
	const int  aa     = ( Config.aa_samples < 1 ) ? 1 : Config.aa_samples;
	const int  nl     = useDof ? Config.dof_samples : 1;

	// Render rows in parallel. Each row is an independent unit of work; "ray" is
	// firstprivate so every thread gets its own scratch copy (origin/type already
	// set) rather than racing on a shared one. "dynamic" scheduling load-balances
	// the uneven per-row cost (busy teapot rows vs. empty background rows).
	#pragma omp parallel for schedule(dynamic) firstprivate(ray)
	for( int i = 0; i < yres; i++ )
	{
		// With a fixed seed, each row's random stream is deterministic no matter
		// which thread renders it -- renders become reproducible run-to-run.
		if( Config.seed != 0 ) kbReseed( (unsigned)i );

		for( int j = 0; j < xres; j++ )
		{
			if ( Config.aa_adaptive )
			{
				I(i,j) = ToneMap ( AdaptiveSuper( i, j, dU, dR, O, ray, scene, Config.aa_threshold, true ) );
				continue;
			}

			// One unified sampling loop: an aa x aa grid over the pixel
			// (optionally jittered), and -- when the scene has a lens -- an
			// nl x nl spread of rays through the aperture for each grid cell.
			Color color( 0, 0, 0 );
			for(int k = 0; k < aa; k++)
			{
				for(int l = 0; l < aa; l++)
				{
					if( aa == 1 && !Config.aa_jitter )
					{
						ray.direction = Unit( O + j * dR - i * dU );
					}
					else if ( Config.aa_jitter )
					{
						double x = rand( 0, 1 );
						double y = rand( 0, 1 );
						ray.direction = Unit( ( O + j * dR - i * dU ) + ( (k + x) * ( dR / aa ) ) - ( ( l + y ) * (dU / aa ) ) );
					}
					else
					{
						// uniform supersampling
						ray.direction = Unit( ( O + j * dR - i * dU ) + ( k * ( dR / aa ) ) - ( l * (dU / aa ) ) );
					}

					if( useDof )
					{
						// Thin-lens model: every ray through the aperture converges
						// on this AA sample's point in the focal plane.
						Vec3 focalPoint = cam.eye + lens.focalDist * ray.direction;
						for( int a = 0; a < nl * nl; a++ )
						{
							// Uniform sample on the aperture disk.
							double r     = lens.lensRadius * sqrt( rand( 0, 1 ) );
							double theta = rand( 0, TwoPi );
							Ray lensRay;
							lensRay.type      = primary_ray;
							lensRay.origin    = cam.eye + U * ( r * cos( theta ) ) + R * ( r * sin( theta ) );
							lensRay.direction = Unit( focalPoint - lensRay.origin );
							color += Trace( lensRay, scene, Config.max_bounces );
						}
					}
					else
					{
						color += Trace( ray, scene, Config.max_bounces );
					}
				}
			}
			I(i,j) = ToneMap( color / ( aa * aa * nl * nl ) );
		}
		if( Config.display_messages && i % 10 == 0 ) { printf( "\r\t%6.2f%c Complete", ( ( i / ( double ) yres ) * 100.0 ), '%' ); fflush( stdout ); }
	}

	if( Config.display_messages )	
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

	// Save the image to a file (.png or .ppm, by extension).
    if( !kbWriteImage( I, fname ) )
	{
		printf( "Could not save file!\n" );
	}
    
	return true;
    }

Color AdaptiveSuper(int i, int j, Vec3 dU, Vec3 dR, Vec3 O, Ray ray, const Scene &scene, double thr, bool first)
{
	if ( first )
	{
		// Pre-pass: send four rays through the corners of the pixel. If they
		// all agree (within thr) the pixel is smooth and their average will
		// do; otherwise refine with the full aa_samples x aa_samples grid.
		Color color[ 4 ];
		Color sum( 0, 0, 0 );
		for( int k = 0; k < 2; k++ )
		{
			for (int l = 0; l < 2; l++ )
			{
				ray.direction = Unit ( O + ( j + k ) * dR - ( i + l ) * dU );
				sum += color[ 2 * k + l ] = Trace( ray, scene, Config.max_bounces );
			}
		}
		Color average = sum / 4;

		for ( int n = 0; n < 4; n++ )
		{
			if ( !CompareColor( color[ n ], average, thr ) )
			{
				// A corner disagrees with the average: this pixel spans an
				// edge or highlight, so refine it with the full sample grid.
				return AdaptiveSuper( i, j, dU, dR, O, ray, scene, thr, false );
			}
		}
		return average;   // all four corners agree; the pixel is smooth
	}

	// Refinement: a full uniform aa_samples x aa_samples grid over the pixel.
	int aa = ( Config.aa_samples < 2 ) ? 2 : Config.aa_samples;
	Color sum( 0, 0, 0 );
	for(int k = 0; k < aa; k++)
	{
		for(int l = 0; l < aa; l++)
		{
			ray.direction = Unit( ( O + j * dR - i * dU ) + ( k * ( dR / aa ) ) - ( l * (dU / aa ) ) );
			sum += Trace( ray, scene, Config.max_bounces );
		}
	}
	return sum / ( aa * aa );
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
