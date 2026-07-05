/***************************************************************************
* shade.C                                                                  *
*                                                                          *
* This file defines the shader for the toy tracer.  The job of the shader  *
* is to determine the color of the surface, viewed from the origin of      *
* the ray that hit the surface, taking into account the surface material,  *
* light sources, and other objects in the scene.                           *
*                                                                          *
* History:                                                                 *
*	10/22/2004	Kevin Brandon											   *
*   10/10/2004  Scene object now contains a list of light source objects.  *
*   09/29/2004  Updated for Fall 2004 class.                               *
*   04/14/2003  Point lights are now point objects with emission.          *
*   04/01/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/

#include "toytracer.h"
#include "kbConfig.h"
#include <stdlib.h>


// Direct/indirect light sample counts. These size fixed-length arrays below,
// so they remain compile-time constants (change here + recompile). All other
// shading toggles are runtime settings in Config (see kbConfig.h).
#define MAX_DL_SAMP 8    // max area-light sample grid dim (sizes a fixed array); actual count = Config.dlSamp
// number of indirect light samples
#define N_IDL_SAMP 1

// samples the projected hemisphere
int SampleProjectedHemisphere(const Vec3 &P, const Vec3 &N, Sample *samples, const int n);

// computes the color/radiance from only the direct light
Color DirectLight	 ( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth );

// computes the color/radiance from both the indirect light ( and if Config.enable_stratify_light is false then it also adds the direct light )
Color IndirectLight	 ( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth);

// computes the color/radiance from a point light source...
Color PointLightShade( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth );

// computes the color after reflection...
Color GetReflection( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth );

Color GetRefraction( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth );

// computes the color for specular highlights
Color GetSpecular( const HitInfo &hit, const Scene &scene, const Object &light, const Color origColor );

thread_local Color diffuse;   // per-thread scratch (was a shared global — a data race under OpenMP)

Color Shade( const HitInfo &hit, const Scene &scene, int max_tree_depth )
{
	// Keep the following line.  Emitters need not be shaded.
	if( hit.object->material.Emitter() ) 
	{
		if( ( hit.ray.type == 3 && Config.enable_stratify_light) )
		{
			return Color(0,0,0);
		}
		else if( !Config.enable_indirect_light )
		{
			return Color( 1, 1, 1 );
		}
		
		return hit.object->material.emission / Pi;
	}

	Color color( 0.0, 0.0, 0.0 );
	
	// here we define a point slightly moved off the point of the surface.
	// this is for shadows and reflections.  Calls to cast and trace will 
	// not function properly becuase there will be an intersection to the 
	// surface we called from.  So we move along the direction of the normal
	Vec3 movedPoint = hit.point + (hit.normal * 0.001);
    
    for( int i = 0; i < scene.num_lights; i++ )
    {
		Object *light = scene.Light[i];
		
		if( !light->material.Emitter() ) continue;   // Skip non-emitters. 

		if(Config.enable_indirect_light)
		{
			// get indirect light...
			color = IndirectLight(hit, scene, *light, movedPoint, max_tree_depth);
			
			if(Config.enable_stratify_light)
			{
				// IndirectLight() counted all light sources as black...
				// we now have to add the direct light sources
				color += DirectLight(hit, scene, *light, movedPoint, max_tree_depth);
			}
		}
		else
		{
			if(Config.enable_area_light)
			{
				// we only look at the direct light source
				color += DirectLight(hit, scene, *light, movedPoint, max_tree_depth) / scene.num_lights;
			}	
			else
			{
				// we only look at the point light source
				color += PointLightShade( hit, scene, *light, movedPoint, max_tree_depth) / scene.num_lights;
			}
		}
	//	if( Config.enable_specular ) color = GetSpecular( hit, scene, *light, color );
		if( Config.enable_reflection ) color = GetReflection( hit, scene, color, max_tree_depth );		
		if( Config.enable_refraction ) color = GetRefraction( hit, scene, color, max_tree_depth );
	}


	
	// here we add ambient color to the scene.
	// this way all the shadows aren't just black
	// Use THIS surface's own diffuse for the ambient term. The shared 'diffuse'
	// scratch may have been clobbered by the recursive reflection shading above.
	Color ambDiffuse = ( hit.object->material.texture == NULL )
		? hit.object->material.diffuse
		: hit.object->material.texture->GetColorFromUV( hit.uv, hit.point );
	color += ambDiffuse * scene.ambient;

	return color;
} 


Color IndirectLight( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth)
{
	
	if(max_tree_depth == 0)
	{
		// if the depth is at 0 then we only look at direct light...
		return DirectLight(hit, scene, light, movedPoint, max_tree_depth);
	}

	// radiance is actually a sum of all the radiance 
	// gained from the direction of each sample
	Color radiance;

	// get our samples...
	Sample samples[N_IDL_SAMP * N_IDL_SAMP];
	SampleProjectedHemisphere(hit.point, hit.normal, samples, N_IDL_SAMP);

	for(int i = 0; i < N_IDL_SAMP * N_IDL_SAMP; i++)
	{			
		Vec3 direcToSamp = Unit( samples[i].P - hit.point);
		Ray toSamp;
		toSamp.direction = direcToSamp;
		toSamp.origin = movedPoint;
		toSamp.type = indirect_ray;
		
		radiance += Trace(toSamp, scene, max_tree_depth);
	}
	
	// irad is the irradance...  Iradiance = the avarage radiance times pi... 
	// (only if the samples are of the *projected* hemisphere)
	Color irad = Pi * ( radiance / ( N_IDL_SAMP * N_IDL_SAMP ) );
	
	Color color = hit.object->material.diffuse * irad;

	return color;

}

Color DirectLight( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth )
{

	Color color;

	// The array of samples that will be passed to GetSamples
	int dl = Config.dlSamp; if( dl < 1 ) dl = 1; if( dl > MAX_DL_SAMP ) dl = MAX_DL_SAMP;
	Sample samples[MAX_DL_SAMP * MAX_DL_SAMP];

	light.GetSamples(hit.point, hit.normal, samples, dl);
	
	// irad is the irradance at the point P.  To find the irradiance we will
	// use a monte carlo sampling method.  Irradiance will equal the avarage
	// of the cosine of the angle between each sample and the normal times the 
	// hit function.  The hit function simply = 0 if the ray from point P to 
	// the sample has something blocking it, and = 1 if the ray is unobstructed.
	// irad = E(TwoPi*L*hit()*cos())
	// L = 1 / TwoPi 
	double irad = 0;
	
	for(int i = 0; i < dl*dl; i++)
	{			
		// check to see if this one pos is in 'shadow'
		Vec3 lightPos = samples[i].P;
		Vec3 direcToLight = Unit(lightPos - hit.point);
		Ray toLight;
		HitInfo junkInfo;
		junkInfo.ignore = &light;
		junkInfo.distance = Infinity;
		toLight.direction = direcToLight;
		toLight.origin = movedPoint;
		if(!Cast(toLight, scene, junkInfo))
		{
			// hit function equals one, so we need to add the cosine of the 
			// angle between point P and the vector to the sample.
			double cosineOfAngle = (float)(direcToLight * hit.normal / (Length(direcToLight) * Length (hit.normal)));
			if(cosineOfAngle < 0) cosineOfAngle = 0;
			irad += cosineOfAngle; 
	 	}
		else
		{
			// hit function equals zero, so do not add anything to irad.
		}
	
	}
	
	// at this point irad is simply a sum of all the cosines to each
	// unobstructed sample we need to devide it by the total number
	// of samples to make it an avarage (and to get irradance)
	irad = irad / (double)(dl*dl);

	color = hit.object->material.diffuse * irad;

	return color;
}

int SampleProjectedHemisphere(const Vec3 &P, const Vec3 &N, Sample *samples, const int n)
{
	// Orientation information...
	Vec3 W = N;
	Vec3 U = Unit(OrthogonalTo(W));
	Vec3 V = Unit (W ^ U);
	
	int count = 0;
	for(int i = 0; i < n; i++)
	{
		for(int j = 0; j < n; j++)
		{
			double rand1 = rand();
			rand1 = rand1 / RAND_MAX;
			rand1 = ( i + rand1) / n;	// stratify samples

			double rand2 = rand();
			rand2 = rand2 / RAND_MAX;
			rand2 = ( j + rand2) / n;	// stratify samples

			double x = sqrt(rand1) * cos(TwoPi * rand2);
			double y = sqrt(rand1) * sin(TwoPi * rand2);

			// x and y give you the point on the disk which we sampled
			// z is just the 'height' above the disk to the hemisphere
			double z = sqrt(1 -  x * x - y * y);

//			Vec2 OnDisk(x, y);
			
			// 
			Vec3 OnHemisphere = P + (U * x + V * y  + W * z);
			samples[count].P = OnHemisphere;
			samples[count].w = TwoPi / (n * n);
			count++;
		}
	}
	return n*n;
}

Color PointLightShade( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth )
{
	Color color;
	Color black(0,0,0);
	
	if( hit.object->material.texture == NULL )
	{
		diffuse = hit.object->material.diffuse;
	}
	else
	{
		diffuse = hit.object->material.texture->GetColorFromUV( hit.uv, hit.point );
	}

	// here we find direction vector of the reflection.
	// R - 2 N (N * R) where R is the direction of the orignal incident ray
	// and N is the normal to the surface
	// R = viewDirec and N = hit.geom.normal
	Vec3 viewDirec = Unit(hit.point - hit.ray.origin);
	Vec3 reflectDirec = Unit(viewDirec - ( 2 * hit.normal) * (viewDirec * hit.normal));

	// You can find the position of a point light source like this...
	Vec3 lightPos = light.GetBounds().Center();

	// here we find the direction of the light:
	// it is easily found by taking the position of 
	// the light and subtracting it by the point of intersection
	Vec3 direcToLight = Unit(lightPos - hit.point);

	if( hit.ray.type != inside_ray )
	{
		if (Config.enable_shading)
		{
			// here we calculate diffuse shading by multiplying the diffuse color 
			// by the cosine of the angle between the normal and direction to the light source
			// cos(theta) = ( A dot B ) / ( Length(A) * Length(B) ) 

			float cosineOfAngle =  (float)(direcToLight * hit.normal /  (Length(direcToLight) * Length(hit.normal)));
			if(cosineOfAngle < 0)	color = black;		// if cosine of the angle is less than zero then there is no direct view to the light source
			else	color = diffuse * cosineOfAngle; 
		}
	
		if(Config.enable_shadows)
		{
			// here we find if we are in a shadow
			Ray toLight;
			HitInfo info;                  
			info.ignore = NULL;									// Don't ignore any objects.
			info.distance = Length(lightPos - hit.point);		// Distance to the light source.
			toLight.direction = direcToLight;					// Direction to the light source.
			
			// the origin must be lifted of the surface of the object
			toLight.origin = movedPoint;	// point that starts in shadow
	
			// To find if we are in the shadow we cast a ray twoards the light
			// if there was an intersection and it was not the light then 
			// we are in the shadow of an object, and the color should be black.
			if(Cast(toLight, scene, info))
			{
				// there was something between us and light
				if(!info.object->material.Emitter())	color = black;
			}
		}
	
		if(Config.enable_specular)
		{
			// here we find the specular highlight
			if(hit.object->material.Phong_exp != 0)
			{
				// R is the reflection direction and L is the direction of the light source
				// RL is R dot L (normalized)
				float RL = (float)((reflectDirec * direcToLight) / (Length(direcToLight) * Length(reflectDirec))); 
				if(RL > 0) 
				{
					// specWeight is how much we should weight the specular color of the object
					float specWeight = pow(RL, hit.object->material.Phong_exp);
			
					// here we linearly interpolate between the specular color and the current color		
					color =  (( specWeight * hit.object->material.specular) + ((1 - specWeight) * color));	
				}
			}		
		}	
	}
	/*
		if(Config.enable_reflection)
		{
			// here we add reflection
			if( hit.object->material.reflectivity > 0 )
			{
				Ray reflectRay;
				Color reflectColor;
				reflectRay.direction = reflectDirec;
				reflectRay.origin = movedPoint;
				if(max_tree_depth != 0)
				{
					// recursivly call Trace to find what color would be seen from the ray reflecRay
					reflectColor = Trace(reflectRay, scene, max_tree_depth );
					// here we linearly interpolate between the reflection and the color of the object without reflection
					color = ( hit.object->material.reflectivity  * reflectColor ) + (( 1 - hit.object->material.reflectivity )* color);
				}
			}
		}
	*/

	/*
	if( Config.enable_refraction)
	{
		if( hit.object->material.refractivity > 0 )
		{
			float n;
			Vec3 normal;
			Ray refractRay;
			Color refractColor;
			if( hit.ray.type != inside_ray )	
			{
				n = 1 / hit.object->material.index_refract;
				refractRay.origin = hit.point + (hit.normal * -0.001);
				refractRay.type = inside_ray;
				normal = hit.normal;
			}
			else
			{
				n = hit.object->material.index_refract;
				refractRay.origin = hit.point + (hit.normal * 0.001);
				refractRay.type = generic_ray;
				normal = -hit.normal;
			}
			Vec3 incident = hit.ray.direction; 
			
			float cosI = normal * incident;
			float sin2T = n * n * ( 1.0 - cosI * cosI );
			if( sin2T <= 1.0f )
			{
				refractRay.direction = n * incident - ( n * cosI + sqrt( 1.0 - sin2T ) ) * normal;
				refractColor = Trace( refractRay, scene, max_tree_depth );
				if( hit.ray.type == inside_ray ) return refractColor;
			}
			color = ( hit.object->material.refractivity * refractColor ) + ( ( 1 - hit.object->material.refractivity ) * color );
		}
	}
	*/
	return color;
}

Color GetSpecular( const HitInfo &hit, const Scene &scene, const Object &light, const Color origColor )
{
	Color color;

	if(hit.object->material.Phong_exp != 0)
	{	
		Vec3 viewDirec = Unit(hit.point - hit.ray.origin);
		Vec3 reflectDirec = Unit(viewDirec - ( 2 * hit.normal) * (viewDirec * hit.normal));
		Vec3 lightPos = light.GetBounds().Center();
		Vec3 direcToLight = Unit(lightPos - hit.point);
		// RL is R dot L (normalized)
		float RL = (float)((reflectDirec * direcToLight) / (Length(direcToLight) * Length(reflectDirec))); 
		if(RL > 0) 
		{
			// specWeight is how much we should weight the specular color of the object
			float specWeight = pow(RL, hit.object->material.Phong_exp);
	
			// here we linearly interpolate between the specular color and the current color		
			color =  (( specWeight * hit.object->material.specular) + ((1 - specWeight) * origColor ));	
		}
	}
	else return origColor;

	return color;
    
}

Color GetRefraction( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth )
{
	Color color = origColor;

	if( hit.object->material.refractivity > 0 )
	{
		float n;
		Vec3 normal;
		Ray refractRay;
		Color refractColor;
		if( hit.ray.type != inside_ray )	
		{
			n = 1 / hit.object->material.index_refract;
			refractRay.origin = hit.point + (hit.normal * -0.001);
			refractRay.type = inside_ray;
			normal = hit.normal;
		}
		else
		{
			n = hit.object->material.index_refract;
			refractRay.origin = hit.point + (hit.normal * 0.001);
			refractRay.type = generic_ray;
			normal = -hit.normal;
		}
		Vec3 incident = hit.ray.direction; 
		
		double cosI = normal * incident;
		double sin2T = n * n * ( 1.0 - cosI * cosI );
		if( sin2T <= 1.0f )
		{
			refractRay.direction = n * incident - ( n * cosI + sqrt( 1.0 - sin2T ) ) * normal;
			refractColor = Trace( refractRay, scene, max_tree_depth );
			if( refractColor == origColor ) return origColor;
			if( hit.ray.type == inside_ray ) return refractColor;
			color = ( hit.object->material.refractivity * refractColor ) + ( ( 1 - hit.object->material.refractivity ) * color );
		}
		else
		{
			refractRay.origin = hit.point + ( hit.normal * -0.001 );
			refractRay.direction = Unit(incident - ( 2 * -hit.normal) * (incident * -hit.normal));
			refractRay.type = inside_ray;
			refractColor = Trace( refractRay, scene, max_tree_depth );
			if( hit.ray.type == inside_ray ) return refractColor;
			color = ( hit.object->material.refractivity * refractColor ) + ( ( 1 - hit.object->material.refractivity ) * color );
		}
	}
	return color;
}

Color GetReflection( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth )
{
	Color color;

	// here we add reflection
	if( hit.object->material.reflectivity > 0 )
	{
		Ray reflectRay;
		Vec3 viewDirec = Unit(hit.point - hit.ray.origin);
		reflectRay.direction = Unit(viewDirec - ( 2 * hit.normal) * (viewDirec * hit.normal));
		reflectRay.origin = hit.point + (hit.normal * 0.001);
		if( max_tree_depth != 0 )
		{
			// recursivly call Trace to find what color would be seen from the ray reflecRay
			Color reflectColor = Trace(reflectRay, scene, max_tree_depth );
			// here we linearly interpolate between the reflection and the color of the object without reflection
			color = ( hit.object->material.reflectivity  * reflectColor ) + ( ( 1 - hit.object->material.reflectivity ) * origColor );
		}
	}
	else return origColor;

	return color;
}