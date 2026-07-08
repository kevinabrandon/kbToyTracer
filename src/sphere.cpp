/***************************************************************************
* sphere.C   (primitive object)                                            *
*                                                                          *
* A sphere of given radius and center.  One of the most basic objects      *
* to ray trace.                                                            *
*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~*         
* To intersect a ray with a sphere with the given center and radius, we    *
* solve the following equation for s: || Q + sR - C || = radius, where Q   *
* is the ray origin, R is the ray direction, and C is the center of the    *
* sphere.  This is equivalent to  ( A + sR )^T ( A + sR ) = radius^2,      *
* where A = Q - C.  Expanding, A.A + 2 s A.R + s^2 R.R = radius^2,         *
* where "." denotes the dot product.  Since R is a unit vercor, R.R = 1.   *
* Rearranging, we have s^2 + (2 A.R) s + (A.A - radius^2) = 0, which is    *
* a quadratic equation in s, the distance along the ray to the point of    *
* intersection.  If this equation has complex roots, then the ray misses   *
* the sphere.  Otherwise, we must determine whether either of the roots    *
* falls on the positive part of the ray, and if so, which is closer.       *
*                                                                          *
* History:                                                                 *
* Provenance:                                                              *
* Original: James Arvo (EECS 204, UC Irvine).  Extended by Kevin           *
* Brandon: spherical uv coords for texturing, and GetSamples               *
* (solid-angle sampling of spherical area lights for soft shadows).        *
*   10/10/2004  Broken out of objects.C file.                              *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include <math.h>
#include "toytracer.h"

static const bool 
		enable_strat	= true,
		myMethod		= false;

struct Sphere : public Object 
{
    Sphere() {}
    Sphere( const Vec3 &center, float radius );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Sphere"; }
    Vec3  center;
    float radius;
};

// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( Sphere );

Sphere::Sphere( const Vec3 &cent, float rad )
{
    center = cent;
    radius = rad;
}

Object *Sphere::ReadString( const char *params ) // Reads params from a string.
{
    float x, y, z, r;
    if( sscanf( params, "sphere (%f,%f,%f) %f", &x, &y, &z, &r ) == 4 )
        return new Sphere( Vec3( x, y, z ), r );
    return NULL;
}

Box3 Sphere::GetBounds() const // Returns a bounding box.
{
    Box3 box;
    box.X.min = center.x - radius;  box.X.max = center.x + radius;
    box.Y.min = center.y - radius;  box.Y.max = center.y + radius;
    box.Z.min = center.z - radius;  box.Z.max = center.z + radius;
    return box;
}

bool Sphere::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{

    Vec3 A = ray.origin - center;
    Vec3 R = ray.direction;
    double b = 2.0 * ( A * R );
    double discr = b * b - 4.0 * ( A * A - radius * radius );  // The discriminant.

    // If the discriminant if negative, the quadratic equation had negative
    // roots, and the ray misses the sphere.

    if( discr < 0.0 ) return false;

    discr = sqrt( discr );

    // First try the smaller of the two roots.  If this is positive, it is the
    // closest intersection.

    double s = ( -b - discr ) * 0.5;
    if( s > 0.0 )
    {
        // If the closest intersection is too far away, report a miss.
        if( s > hitinfo.distance ) return false;
    }
    else
    {
        // Now try the other root, since the smallest root puts the
        // point of intersection "behind"us.  If the larger root is now
        // positive, it means we are inside the sphere.
        s = ( -b + discr ) * 0.5;
        if( s <= 0 ) return false;
        if( s > hitinfo.distance ) return false;
    }

    // We have an actual hit.  Fill in all the geometric information so
    // that the shader can shade this point.

	Vec2 uv;
	Vec3 hitPoint = ray.origin + s * R;

	double phi = atan2( ( hitPoint.y - center.y ), ( hitPoint.x - center.x ) );
	double theta = acos( ( hitPoint.z - center.z ) / radius );
	if( phi < 0.0 ) phi += TwoPi;
	uv.x = phi / ( 2 * Pi );
	uv.y = ( Pi - theta ) / Pi;

    hitinfo.distance = s;
    hitinfo.point    = ray.origin + s * R;
    hitinfo.normal   = Unit( hitinfo.point - center );
    hitinfo.ray      = ray;
	hitinfo.uv       = uv;
    hitinfo.object   = this;
    return true;
}

//my first try for GetSamples

int Sphere::GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const
{
	// orientation information  
	// needed for professors solution to rotating samples
	Vec3 R = Unit(center - P);
	Vec3 U = Unit(OrthogonalTo(R));
	Vec3 V = Unit(R ^ U);

	// triangle information
	double distHypot = Length(center - P);								// length of hypotonuse
	double distAdj = sqrt (distHypot * distHypot - radius * radius);	// length of adjacent side
	double distOpp = radius;											// length of opposite side
	
	double sineAlpha = distOpp / distHypot;								// sin(theta)
	double cosineAlpha = distAdj / distHypot;							// cos(theta)
	double tangentAlpha = distOpp / distAdj;							// tan(theta)

	// solid angle calulation and weight information
	double height = 1 - cosineAlpha;	// height for hat-box theorem
	double solidAngle = TwoPi * height;	// solid angle
	double weight = solidAngle / (n * n);	// weight for each sample in the array of samples.
	
	int count = 0;
	for(int i = 0; i < n; i++)
	{
		for(int j = 0; j < n; j++)
		{
			// first random vriable
			double x = rand();	
			x = x / RAND_MAX;	// make x between 0 and 1
			if(enable_strat)	x = (i + x) / n;	// if enable_strat is true x will be limited to the box at (i, j)
			
			double y = rand();
			y = y / RAND_MAX;	// make y between 0 and 1
			if(enable_strat)	y = (j + y) / n;	// if enable_strat is true y will be limited to the box at (i, j)

			// for explination of names of variables see answer to #4 of Homework 3
			double z_component = x + cosineAlpha * (1 - x);
			double K = sqrt(1 - z_component * z_component);
			double x_component = K * cos(TwoPi * y);
			double y_component = K * sin(TwoPi * y);
		
			Vec3 pointOnHem = Vec3(x_component, y_component, z_component) + P;	// point on hemisphere on solid angle cented at direction (0,0,1)
			Vec3 pointOnSphere;

			// rotating point to correct place...
			if(myMethod)
			{
				// my method for rotating samples to correct place...
				// not quite correct... although very close approximation
				Vec3 UpZAxis(0, 0, 1);
				pointOnHem = pointOnHem - UpZAxis;
				pointOnSphere = pointOnHem + R;
			}
			else
			{
				// using a method stolen from the proffesor!  Thanks Proffesor Arvo!
				// reorients the point 
				pointOnSphere = pointOnHem + distHypot * (z_component * R + U * y_component + V * x_component);
			}

			samples[count].P = pointOnSphere;
			samples[count].w = weight;
			count++;
		}
	}
	return n * n;

}

