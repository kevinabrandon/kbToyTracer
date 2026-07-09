/***************************************************************************
* kbShade.cpp                                                              *
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
#include <string.h>
#include <vector>


// Indirect (GI) light sample count. It sizes a fixed-length array below, so
// it remains a compile-time constant (change here + recompile). All other
// shading knobs are runtime settings in Config (see kbConfig.h).
#define N_IDL_SAMP 1

// samples the projected hemisphere
int SampleProjectedHemisphere(const Vec3 &P, const Vec3 &N, Sample *samples, const int n);

// computes the color/radiance from only the direct light
// The light-shading functions return the diffuse/shadow contribution; a Phong
// highlight on a REFRACTIVE surface is diverted into 'deferred_spec' instead
// of the return value, so Shade() can add it after the transmission blend
// (otherwise dense glass multiplies its glints by (1-refractivity) and they
// vanish; the glint is the surface's own reflection of the light source and
// shouldn't fade with transparency).  Opaque surfaces are unaffected.
Color DirectLight	 ( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth, Color &deferred_spec );

// computes the color/radiance from both the indirect light ( and if Config.enable_stratify_light is false then it also adds the direct light )
Color IndirectLight	 ( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth, Color &deferred_spec );

// computes the color/radiance from a point light source...
Color PointLightShade( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth, Color &deferred_spec );

// computes the color after reflection...
Color GetReflection( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth );

Color GetRefraction( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth );

// computes the color for specular highlights
Color GetSpecular( const HitInfo &hit, const Scene &scene, const Object &light, const Color origColor );

thread_local Color diffuse;   // per-thread scratch (was a shared global — a data race under OpenMP)

Color Shade( const HitInfo &hit, const Scene &scene, int max_tree_depth )
{
	// Keep the following line.  Emitters need not be shaded.
	if( hit.Mtl().Emitter() ) 
	{
		if( ( hit.ray.type == 3 && Config.enable_stratify_light) )
		{
			return Color(0,0,0);
		}
		else if( !Config.enable_indirect_light )
		{
			return Color( 1, 1, 1 );
		}
		
		return hit.Mtl().emission / Pi;
	}

	Color color( 0.0, 0.0, 0.0 );

	// Phong highlights on refractive surfaces, deferred past the
	// transmission blend (see the declarations above).
	Color deferredSpec( 0.0, 0.0, 0.0 );

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
			color = IndirectLight(hit, scene, *light, movedPoint, max_tree_depth, deferredSpec);

			if(Config.enable_stratify_light)
			{
				// IndirectLight() counted all light sources as black...
				// we now have to add the direct light sources
				color += DirectLight(hit, scene, *light, movedPoint, max_tree_depth, deferredSpec);
			}
		}
		else
		{
			// The scene decides the light type: a Point emitter gets the analytic
			// single-ray path (with specular); an emitter with actual area (sphere,
			// quad, ...) gets Monte-Carlo soft shadows at Config.shadow_samples^2.
			Color spec( 0.0, 0.0, 0.0 );
			if( strcmp( light->MyNameIs(), "Point" ) == 0 || Config.shadow_samples < 1 )
			{
				color += PointLightShade( hit, scene, *light, movedPoint, max_tree_depth, spec ) / scene.num_lights;
			}
			else
			{
				color += DirectLight( hit, scene, *light, movedPoint, max_tree_depth, spec ) / scene.num_lights;
			}
			deferredSpec += spec / scene.num_lights;   // same per-light weight as the color term
		}
	}

	// Reflection and refraction are view-dependent, not light-dependent: evaluate them
	// ONCE per hit, after all light contributions are summed. (Previously these lived inside
	// the per-light loop, causing a (num_lights)^depth ray explosion and repeated blending.)
	if( Config.enable_reflection ) color = GetReflection( hit, scene, color, max_tree_depth );

	// Ambient (a flat fill so shadows aren't pure black) is added here -- AFTER reflection
	// but BEFORE refraction -- so the refraction blend attenuates it by (1-refractivity).
	// Added after refraction instead, a transparent surface dumps its full ambient fill on
	// top of whatever you see through it, and that fill compounds through the refraction
	// recursion (a bright bloom in dense glass). Placing it here leaves opaque and purely
	// reflective scenes unchanged. (Use THIS surface's own diffuse; the shared 'diffuse'
	// scratch may have been clobbered by the recursive reflection shading above.)
	Color ambDiffuse = ( hit.Mtl().texture == NULL )
		? hit.Mtl().diffuse
		: hit.Mtl().texture->GetColorFromUV( hit.uv, hit.point );
	color += ambDiffuse * scene.ambient;

	if( Config.enable_refraction ) color = GetRefraction( hit, scene, color, max_tree_depth );

	// Highlights on glass ride on top of the transmission blend (POV-style
	// additive phong); zero for opaque surfaces, whose highlights were added
	// inside the light loop as before.
	color += deferredSpec;

	return color;
}


Color IndirectLight( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth, Color &deferred_spec )
{

	if(max_tree_depth == 0)
	{
		// if the depth is at 0 then we only look at direct light...
		return DirectLight(hit, scene, light, movedPoint, max_tree_depth, deferred_spec);
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
	
	Color color = hit.Mtl().diffuse * irad;

	return color;

}

Color DirectLight( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth, Color &deferred_spec )
{

	Color color;

	// The array of samples that will be passed to GetSamples. The light says
	// how many samples it actually produced (a Point light returns just one).
	int dl = Config.shadow_samples; if( dl < 1 ) dl = 1;
	std::vector<Sample> samples( dl * dl );

	int got = light.GetSamples(hit.point, hit.normal, samples.data(), dl);
	if( got < 1 ) return color;

	// irad is the irradance at the point P.  To find the irradiance we will
	// use a monte carlo sampling method.  Irradiance will equal the avarage
	// of the cosine of the angle between each sample and the normal times the 
	// hit function.  The hit function simply = 0 if the ray from point P to 
	// the sample has something blocking it, and = 1 if the ray is unobstructed.
	// irad = E(TwoPi*L*hit()*cos())
	// L = 1 / TwoPi 
	double irad = 0;
	
	for(int i = 0; i < got; i++)
	{
		// Direction to this light sample, and its geometric (cosine) term.
		Vec3 lightPos = samples[i].P;
		Vec3 direcToLight = Unit(lightPos - hit.point);
		double cosineOfAngle = direcToLight * hit.normal / (Length(direcToLight) * Length(hit.normal));
		if(cosineOfAngle <= 0) continue;   // sample below the horizon; contributes nothing

		Ray toLight;
		toLight.direction = direcToLight;
		toLight.origin = movedPoint;

		// Visibility. 'transmittance' is the fraction of the sample's light that
		// reaches the surface: 1 = clear, 0 = fully blocked, in-between when
		// transparent shadows are on and the path crosses refractive (glass) blockers.
		double transmittance = 1.0;
		if(Config.enable_transparent_shadows)
		{
			// March toward the sample, accumulating transmittance through refractive
			// blockers instead of a hard yes/no occlusion (glass casts a lighter
			// shadow). Mirrors the point-light path in PointLightShade(). No caustics.
			for(int guard = 0; guard < 32; guard++)
			{
				HitInfo info;
				info.ignore = &light;                               // don't shadow on the light itself
				info.distance = Length(lightPos - toLight.origin);  // only occluders before the sample
				if(!Cast(toLight, scene, info)) break;              // clear path to the light
				if(info.Mtl().Emitter()) break;          // reached an emitter
				double refr = info.Mtl().refractivity;
				if(refr <= 0.0) { transmittance = 0.0; break; }     // opaque -> full shadow
				transmittance *= refr;                              // glass -> partial pass
				toLight.origin = info.point + direcToLight * 0.001; // step past this surface
			}
		}
		else
		{
			// Original hard shadow: any non-light blocker fully occludes the
			// sample.  Bound the ray at the sample point -- with Infinity,
			// geometry BEYOND the light also "blocked" it, which put enclosed
			// scenes (e.g. a Cornell box, ceiling right behind the panel)
			// entirely in shadow.
			HitInfo junkInfo;
			junkInfo.ignore = &light;
			junkInfo.distance = Length(lightPos - toLight.origin);
			if(Cast(toLight, scene, junkInfo)) transmittance = 0.0;
		}

		irad += cosineOfAngle * transmittance;
	}
	
	// at this point irad is simply a sum of all the cosines to each
	// unobstructed sample we need to devide it by the total number
	// of samples to make it an avarage (and to get irradance)
	irad = irad / (double)got;

	// Texture-aware diffuse (same rule as the point-light path).
	Color diff = ( hit.Mtl().texture == NULL )
		? hit.Mtl().diffuse
		: hit.Mtl().texture->GetColorFromUV( hit.uv, hit.point );
	color = diff * irad;

	// Phong specular highlight, mirroring PointLightShade() so highlights
	// don't vanish just because a light has area: aim at the light's center.
	if( Config.enable_specular && hit.ray.type != inside_ray
	    && hit.Mtl().Phong_exp != 0 )
	{
		Vec3 direcToCenter = Unit( light.GetBounds().Center() - hit.point );
		Vec3 viewDirec     = Unit( hit.point - hit.ray.origin );
		Vec3 reflectDirec  = Unit( viewDirec - ( 2 * hit.normal ) * ( viewDirec * hit.normal ) );
		float RL = (float)( reflectDirec * direcToCenter );
		if( RL > 0 )
		{
			// Additive Phong: the highlight rides on top of the shading.
			// (The old lerp toward the specular color *replaced* the diffuse
			// under the lobe, so a specular DIMMER than the surface -- e.g.
			// an mtl with Ns set but Ks black -- darkened it into a fake
			// round "shadow" instead of a highlight.)
			float specWeight = pow( RL, hit.Mtl().Phong_exp );
			if( Config.enable_refraction && hit.Mtl().refractivity > 0 )
				deferred_spec += specWeight * hit.Mtl().specular;
			else
				color += specWeight * hit.Mtl().specular;
		}
	}

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
			// stratified samples (rand(a,b) is the thread-safe generator)
			double rand1 = ( i + rand( 0.0, 1.0 ) ) / n;
			double rand2 = ( j + rand( 0.0, 1.0 ) ) / n;

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

Color PointLightShade( const HitInfo &hit, const Scene &scene, const Object &light, const Vec3 &movedPoint, int max_tree_depth, Color &deferred_spec )
{
	Color color;
	Color black(0,0,0);
	
	if( hit.Mtl().texture == NULL )
	{
		diffuse = hit.Mtl().diffuse;
	}
	else
	{
		diffuse = hit.Mtl().texture->GetColorFromUV( hit.uv, hit.point );
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
			Ray toLight;
			toLight.direction = direcToLight;					// Direction to the light source.
			toLight.origin = movedPoint;						// lifted off the surface

			if(Config.enable_transparent_shadows)
			{
				// March toward the light, accumulating transmittance through refractive
				// blockers instead of a hard yes/no occlusion. Opaque blockers still fully
				// shadow; glass lets its 'refractivity' fraction of light pass at each
				// surface crossing. (This lightens shadows under glass; it does NOT make
				// caustics -- the light isn't actually focused, that needs photon mapping.)
				double transmittance = 1.0;
				for( int guard = 0; guard < 32; guard++ )
				{
					HitInfo info;
					info.ignore = NULL;
					info.distance = Length(lightPos - toLight.origin);
					if( !Cast(toLight, scene, info) ) break;			// clear path to light
					if( info.Mtl().Emitter() ) break;		// reached the light
					double refr = info.Mtl().refractivity;
					if( refr <= 0.0 ) { transmittance = 0.0; break; }	// opaque -> full shadow
					transmittance *= refr;								// glass -> partial pass
					toLight.origin = info.point + direcToLight * 0.001;	// step past this surface
				}
				color = color * transmittance;
			}
			else
			{
				// Original hard shadow: any non-emitter blocker -> fully in shadow.
				HitInfo info;
				info.ignore = NULL;
				info.distance = Length(lightPos - hit.point);
				if( Cast(toLight, scene, info) )
					if( !info.Mtl().Emitter() )	color = black;
			}
		}
	
		if(Config.enable_specular)
		{
			// here we find the specular highlight
			if(hit.Mtl().Phong_exp != 0)
			{
				// R is the reflection direction and L is the direction of the light source
				// RL is R dot L (normalized)
				float RL = (float)((reflectDirec * direcToLight) / (Length(direcToLight) * Length(reflectDirec))); 
				if(RL > 0)
				{
					// specWeight is how much we should weight the specular color of the object
					float specWeight = pow(RL, hit.Mtl().Phong_exp);

					// Additive Phong (see DirectLight): the old lerp replaced the
					// diffuse under the lobe and turned dim/black speculars into
					// dark blotches.
					if( Config.enable_refraction && hit.Mtl().refractivity > 0 )
						deferred_spec += specWeight * hit.Mtl().specular;
					else
						color += specWeight * hit.Mtl().specular;
				}
			}		
		}	
	}
	/*
		if(Config.enable_reflection)
		{
			// here we add reflection
			if( hit.Mtl().reflectivity > 0 )
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
					color = ( hit.Mtl().reflectivity  * reflectColor ) + (( 1 - hit.Mtl().reflectivity )* color);
				}
			}
		}
	*/

	/*
	if( Config.enable_refraction)
	{
		if( hit.Mtl().refractivity > 0 )
		{
			float n;
			Vec3 normal;
			Ray refractRay;
			Color refractColor;
			if( hit.ray.type != inside_ray )	
			{
				n = 1 / hit.Mtl().index_refract;
				refractRay.origin = hit.point + (hit.normal * -0.001);
				refractRay.type = inside_ray;
				normal = hit.normal;
			}
			else
			{
				n = hit.Mtl().index_refract;
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
			color = ( hit.Mtl().refractivity * refractColor ) + ( ( 1 - hit.Mtl().refractivity ) * color );
		}
	}
	*/
	return color;
}

Color GetSpecular( const HitInfo &hit, const Scene &scene, const Object &light, const Color origColor )
{
	Color color;

	if(hit.Mtl().Phong_exp != 0)
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
			float specWeight = pow(RL, hit.Mtl().Phong_exp);
	
			// here we linearly interpolate between the specular color and the current color		
			color =  (( specWeight * hit.Mtl().specular) + ((1 - specWeight) * origColor ));	
		}
	}
	else return origColor;

	return color;
    
}

Color GetRefraction( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth )
{
	Color color = origColor;

	if( hit.Mtl().refractivity > 0 )
	{
		float n;
		Vec3 normal;
		Ray refractRay;
		Color refractColor;
		if( hit.ray.type != inside_ray )	
		{
			n = 1 / hit.Mtl().index_refract;
			refractRay.origin = hit.point + (hit.normal * -0.001);
			refractRay.type = inside_ray;
			normal = hit.normal;
		}
		else
		{
			n = hit.Mtl().index_refract;
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
			color = ( hit.Mtl().refractivity * refractColor ) + ( ( 1 - hit.Mtl().refractivity ) * color );
		}
		else
		{
			refractRay.origin = hit.point + ( hit.normal * -0.001 );
			refractRay.direction = Unit(incident - ( 2 * -hit.normal) * (incident * -hit.normal));
			refractRay.type = inside_ray;
			refractColor = Trace( refractRay, scene, max_tree_depth );
			if( hit.ray.type == inside_ray ) return refractColor;
			color = ( hit.Mtl().refractivity * refractColor ) + ( ( 1 - hit.Mtl().refractivity ) * color );
		}
	}
	return color;
}

Color GetReflection( const HitInfo &hit, const Scene &scene, const Color origColor, const int max_tree_depth )
{
	Color color;

	// here we add reflection
	if( hit.Mtl().reflectivity > 0 )
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
			color = ( hit.Mtl().reflectivity  * reflectColor ) + ( ( 1 - hit.Mtl().reflectivity ) * origColor );
		}
	}
	else return origColor;

	return color;
}