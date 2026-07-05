/***************************************************************************
* point.C   (primitive object)                                             *
*                                                                          *
* The point object is not a real object in that a ray can never hit, by    *
* definition.  It is merely used as a way to encode point light sources    *
* in a scene in which objects are allowed to emit light.  Hence, a point   *
* object with non-zero emission is a point light source.                   *
*                                                                          *
* History:                                                                 *
*	10/22/2004	Kevin Brandon											   *
*   10/10/2004  Broken out of objects.C file.                              *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include "toytracer.h"

struct Point : public Object {     // This is used for encoding point light sources.
    Point() {}
    Point( const Vec3 &position );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Point"; }
    Vec3 position;
    };

// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( Point );

Point::Point( const Vec3 &pos )
{
    position = pos;
}

Object *Point::ReadString( const char *params ) // Reads params from a string.
{
    float x, y, z;
    if( sscanf( params, "point (%f,%f,%f)", &x, &y, &z ) == 3 )
        return new Point( Vec3( x, y, z ) );
    return NULL;
}

Box3 Point::GetBounds() const // Returns a bounding box.
{
    Box3 box;
    box.X.min = position.x;  box.X.max = position.x;
    box.Y.min = position.y;  box.Y.max = position.y;
    box.Z.min = position.z;  box.Z.max = position.z;
    return box;
}

bool Point::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{
    // Always false, by definition.
    return false;
}

int Point::GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const
{
    // Return exactly point point, with unit weight.
    samples[0].P = position;
    samples[0].w = 1.0;
    return 1;
}