//
//	kbPlane.cpp    (primitive object)
//
//	An infinite plane:  "plane (nx,ny,nz) d"  with unit normal N and
//	plane equation N*P = d.
//
//	Kevin Brandon
//
#include <stdio.h>
#include "toytracer.h"

static const bool 
		enable_strat	= true,
		myMethod		= false;

struct Plane : public Object 
{
    Plane() {}
    Plane( const Vec3 &normal, float dist );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Plane"; }
    Vec3  m_normal;
    float m_distance;
};

// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( Plane );

Plane::Plane( const Vec3 &normal, float dist )
{
    m_normal = normal;
    m_distance = dist;
}

Object *Plane::ReadString( const char *params ) // Reads params from a string.
{
    float x, y, z, d;
    if( sscanf( params, "plane (%f,%f,%f) %f", &x, &y, &z, &d ) == 4 )
        return new Plane( Vec3( x, y, z ), d );
    return NULL;
}

Box3 Plane::GetBounds() const 
{
    Box3 box;
	box.X.max = 10.001;
	box.X.min = -10.001;
    box.Y.max = 10.001;
	box.Y.min = -10.001;
	box.Z.max = 10.001;
	box.Z.min = -10.001;
    return box;
}

bool Plane::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{

	double d = m_normal * ray.direction;
	
	if (d != 0)
	{
		double dist = -( ( m_normal * ray.origin ) + m_distance ) / d;
		if (dist > 0)
		{
			//if (dist < a_Dist) 
			if( dist < hitinfo.distance )
			{
				hitinfo.distance = dist;
				hitinfo.point = ray.origin + dist * ray.direction;
				hitinfo.normal = m_normal;
				hitinfo.ray = ray;
				hitinfo.object = this;
				return true;
			}
		}
	}
	return false;
	
}

int Plane::GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const
{
	return 0;
}