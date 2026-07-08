/***************************************************************************
* toytracer.h                                                              *
*                                                                          *
* This is the header file for a "Toy" ray tracer.  This ray tracer is very *
* minimal, and is intended simply for learning the basics of ray tracing.  *
* Here is a partial list of things that are missing from this program that *
* a serious ray tracer would have:                                         *
*                                                                          *
* 1) At least one technique for speeding up ray-object intersection when   *
*    there are a large number of object (e.g. thousands).  In contrast,    *
*    this toy ray tracer uses "brute force" or "naive" ray intersection.   *
*    That is, it simply tests all objects and keeps the closest hit.       *
*                                                                          *
* 2) Some method of anti-aliasing; that is, a method for removing the      *
*    jagged edges, probably by casting multiple rays per pixel.            *
*                                                                          *
* Provenance:                                                              *
* Original: James Arvo (EECS 204, UC Irvine).  Extended by Kevin           *
* Brandon: Lens (depth of field), Material texture/refractivity,           *
* HitInfo uv coords, inside_ray type, kb texture hook.                     *
*                                                                          *
* History:                                                                 *
*   10/10/2004  Added Aggregate sub-class & REGISTER_OBJECT macro.         *
*   10/06/2004  Added type to ray & ray to HitInfo.  Removed HitGeom.      *
*   09/29/2004  Updated for Fall 2004 class.                               *
*   04/10/2003  Added GetSamples to Object class.                          *
*   04/06/2003  Added Object class.                                        *
*   04/01/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#ifndef _TOYTRACER_H_
#define _TOYTRACER_H_

#include <math.h>
#include "Vec3.h"       // Defines the Vec3 class, which are points in R3.
#include "Vec2.h"		// Defines the Vec2 class, which are points in R2.
#include "Mat3x3.h"     // Defines 3x3 matrices.
#include "Mat3x4.h"     // Defines 3x4 matrices, which encode affine transforms.
#include "Color.h"      // Defines the Color class, which holds real RGB values. 
#include "kbTexture.h"

static const int MaxLights = 10;

struct Object;          // Objects that can be ray traced, defined below.

enum ray_type {         // A flag that may affect the processing of a ray.
    generic_ray  = 0,   // Any ray; No special meaning.
    primary_ray  = 1,   // A ray cast directly from the eye.
    shadow_ray   = 2,   // A ray cast for shadow detection only.
    indirect_ray = 3,   // A ray cast from a surface to sample illumination.
    light_ray    = 4,   // A ray cast from a light source (e.g. for photon mapping).
    special_ray  = 5,   // A ray used for some other special purpose.
	inside_ray   = 6	// A ray shot inside an object (for refraction).
    };

struct Ray {            // A ray in R3.
    Vec3 origin;        // The ray originates from this point.
    Vec3 direction;     // Unit vector indicating direction of ray.
    int  type;          // Different rays may be processed differently.
    };

struct Material {       // Surface material for shading.
    // (KB: everything zeroed explicitly.  This struct used to rely on lucky
    // fresh-heap zeroes: an aggregate created mid-parse -- e.g. an imported
    // model -- could hand its children a garbage texture pointer.)
    Material() : texture(0), reflectivity(0), refractivity(0),
                 index_refract(1), Phong_exp(0), type(0) {}
    Color diffuse;      // Diffuse color.
    Color specular;     // Color of highlights.
    Color emission;     // Emitted light.
	kbTexture *texture;
    float reflectivity; // Weight given to mirror reflection, between 0 and 1.
	float refractivity; // Weight given to refraction, between 0 and 1.
	float index_refract; // index of refraction
    float Phong_exp;    // Phong exponent for specular highlights.
    int   type;         // Reserved for future use.
    bool  Emitter() const { return emission != 0.0; } 
    };

struct HitInfo {          // Records all info at ray-object intersection.
    const Object *object; // The object that was hit.
    const Object *ignore; // An object to ignore (passed in to Cast).
    double  distance;     // Distance along ray to the point of intersection.
    Vec3    point;        // The point of ray-object intersection.
    Vec3    normal;       // The surface normal at the point of intersection.
    Ray     ray;          // The ray that hit the surface.
	Vec2    uv;           // uv coordinates for 2d textures
    const Material *material = 0; // Material to shade with: the hit object's
                          // own, unless an instance wrapper (kbInstance)
                          // overrides it so shared geometry can be shaded
                          // with a per-instance material.
    const Material &Mtl() const;  // material if set, else object->material.
    };

struct Camera {         // Defines the position of the camera.
    Vec3  eye;          // Position of eye.
    Vec3  lookat;       // The point we are looking toward.
    Vec3  up;           // A vector not parallel to the direction of gaze.
    float vpdist;       // Distance to the view plane.
    };

struct Lens
{
	float lensRadius;
	float focalDist;

	Lens( ) { lensRadius = 0; focalDist = 0; }
	Lens( float rad, float dist ) { lensRadius = rad; focalDist = dist; }
};

struct Interval {       // An interval of real numbers.
    Interval() {}
    Interval( double a, double b ) { min = a; max = b; }
    double min;
    double max;
    Interval &operator+=( const Interval &A );  // Expand to include the interval A.
    Interval &operator+=( const double c ) { min += c; max += c; return *this; }
    };

struct Box3 {           // A box in R3.  Useful for bounding boxes.
    Interval X;
    Interval Y;
    Interval Z;
    Vec3 Center() const;
    Box3 &operator+=( const Box3 &A );  // Expand to include the box A;
    inline Vec3 Min() const { return Vec3( X.min, Y.min, Z.min ); }
    inline Vec3 Max() const { return Vec3( X.max, Y.max, Z.max ); }
    inline Box3 &operator+=( const Vec3 &v ) { X += v.x; Y += v.y; Z += v.z; return *this; }
    };

struct Sample {         // A point and weight returned from a sampling algorithm.
    Vec3   P;
    double w;
    };



struct Object {         // The base class for all objects that can be ray traced.
    Object() {}
    virtual ~Object() { parent = 0; }
    virtual bool Intersect( const Ray &ray, HitInfo & ) const = 0;
    virtual Box3 GetBounds() const = 0;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const { return 0; }
    virtual bool IsAggregate() const { return false; }
    virtual void Transform( const Mat3x4 & ) {} // Used by aggregates.
    virtual void AddChild( Object * ) {}        // Used by aggregates.
    virtual Object *ReadString( const char * ) = 0; // Creates instances from strings of params.
    virtual const char *MyNameIs() const { return "<undefined>"; }
    Material material;
    Object  *parent;    // Useful for nesting objects within aggregates.
    Object  *next;      // Used by toytracer to link registered objects.
    };

inline const Material &HitInfo::Mtl() const
    {
    return material != 0 ? *material : object->material;
    }

struct Scene {
    Color ambient;              // Ambient light (from all directions).
    Color bgcolor;              // Background color, if the ray does not hit anything. 
    Object *object;             // Either a single primitve or an aggregate object.
    Object *Light[ MaxLights ]; // Pointers to all the objects that are emitters.
    int    num_lights;          // The number of light-emitting objects.
    };

struct Aggregate : public Object {  // The base class for all aggregate objects.
    Aggregate() {}
    virtual ~Aggregate() {}
    virtual void AddChild( Object * ) = 0;  // Each aggregate must fill this in.
    virtual bool IsAggregate() const { return true; }
    };

// The three fundamental functions for ray tracing.

extern Color Trace(         // What color do I see looking along this ray?
    const Ray   &ray,       // Root of ray tree to recursively trace in scene.
    const Scene &scene,     // Global scene description: objects + materials.
    int   max_tree_depth    // Limit to depth of the ray tree.
    );

extern Color Shade(         // Surface shader.
    const HitInfo &hitinfo, // Geometry of ray-object hit + surface material.
    const Scene   &scene,   // Global scene description: objects + materials.
    int   max_tree_depth    // Limit to depth of the ray tree.
    );

extern bool Cast(           // Casts a single ray to see what it hits.
    const Ray   &ray,       // The ray to cast into the scene.
    const Scene &scene,     // Global scene description: objects + materials.
    HitInfo     &hitinfo    // Distance passed in, intersection info returned.
    );


// ReadSceneDescription reads a file of the given name, builds object instances
// and fills in material properties.  The MakeImage function rasterizes the image plane,
// calling Trace at least once per pixel and inserting the values into an image file of
// the given name.

extern bool ReadSceneDescription(
    const char *file_name,      // The file to read, including the extension.
    Camera &camera,             // The camera position defined in the file.
	Lens &lens,					// The lens for the camera.
    Scene &scene                // The scene description defined in the file.
    );

extern bool MakeImage(
    const Camera &camera,       // Defines the view.
	const Lens   &lens,			// The lens for the camera... if lensRadius and focalDist equal zero, it is ignored
    const Scene  &scene,        // Global scene description: objects + materials.
    int           width  = 128, // Horizontal resolution of image in pixels.
    int           height = 128, // Vertical resolution of image in pixels.
    const char   *fname  = "toytracer.ppm"
    );


extern Color AdaptiveSuper(int i, int j, Vec3 dU, Vec3 dR, Vec3 O, Ray ray, const Scene &scene, double thr,  bool first);

bool CompareColor( const Color &A, const Color &B, const double thr );


// Miscellaneous constants and utility functions.

static const double 
    Pi       = 3.14159265358979,
    TwoPi    = 2.0 * Pi,
    FourPi   = 4.0 * Pi,
    Infinity = 1.0E10;

inline double min( double x, double y )
    {
    return x <= y ? x : y;
    }

inline double max( double x, double y )
    {
    return x >= y ? x : y;
    }

extern double rand(    // Return a random number uniformly distributed in [a,b].
    double a,
    double b
    );

extern Box3 operator*( const Mat3x3 &M, const Box3 &box );  // Transforms a bounding box.

inline Box3 operator*( const Mat3x4 &M, const Box3 &box )   // Transforms a bounding box.
    {
    Box3 new_box = M.mat * box;
    return new_box += M.vec;
    }

// The following macro is used for "registering" new objects in the toytracer.  Because of this
// macro, objects can be added to the toytracer without modifying any of the existing code.  Simply
// define a new object in a separate source file and invoke this macro using the class name of the
// new primitive or aggregate object.  Linking the new module into the toytracer will automatically
// add the new object into the list of "registered" objects at run time.  (This macro makes use of
// the fact that global variables are initialized before "main" is invoked.)

#define REGISTER_OBJECT( class_name ) \
    extern bool RegisterObject( Object * ); \
    static bool dummy_variable_##class_name = RegisterObject( new class_name );

 
#endif
