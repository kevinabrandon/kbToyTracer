/***************************************************************************
* box.C    (primitive object)                                              *
*                                                                          *
* The "Box" object defines an axis-aligned box.  Its constructor takes     *
* two vectors, which are taken to be the "min" and "max" corners of the    *
* box (i.e. the three min coords, and the three max coords).               *
*                                                                          *
* History:                                                                 *
*   10/10/2004  Broken out of objects.C file.                              *
*   10/22/2004  Kevin Brandon - moved declarations to kbBox.h and          *
*               merged in Arvo's handout intersector, so all code          *
*               in this file is James Arvo's.                              *
*   07/07/2026  Kevin Brandon - planar face uv coords (marked below), so   *
*               2D textures can be applied to boxes.                       *
*                                                                          *
***************************************************************************/

#include "kbBox.h"

Box::Box( const Vec3 &Min_, const Vec3 &Max_ )
    {
    Min  = Min_;
    Max  = Max_;
    }

Object *Box::ReadString( const char *params ) // Read params from string.
    {
    float a, b, c, d, e, f;
    if( sscanf( params, "box (%f,%f,%f) (%f,%f,%f)", &a, &b, &c, &d, &e, &f ) == 6 )
        return new Box( Vec3( a, b, c ), Vec3( d, e, f ) );
    return NULL;
    }

Box3 Box::GetBounds() const // Returns a bounding box (which is just the box itself).
    {
    Box3 box;
    box.X = Interval( Min.x, Max.x );
    box.Y = Interval( Min.y, Max.y );
    box.Z = Interval( Min.z, Max.z );
    return box;
    }

// Jim Arvo, EECS204, Fall 2004

// Intersect a ray with an axis-aligned box.  Feel free to use
// this itersector in your toy tracer for assignments #2 and
// beyond.
bool Box::Intersect( const Ray &ray, HitInfo &hitinfo ) const
    {
    Vec3 R = ray.direction;
    Vec3 Q = ray.origin;
    Vec3 N;

    // Maintain an interval (min, max) that is inside the box as we process
    // successive slabs.  Start with min=0 and max=hitinfo.distance, to
    // represent the entire available portion of the ray.

    double min = 0.0;
    double max = hitinfo.distance;

    // For each component of the ray direction, check to see which way it is
    // pointed and see if it is directed toward or away from the slab.  If away,
    // return immediately, reporting a miss.  Each time min is updated set the
    // normal N as well.  The normal will then be set according to the face that
    // is most distant (which is the face of the actual box, the intersection
    // of three slabs).

    if( R.x >= 0 ) // Looking down positive X-axis.
        {
        if( Q.x > Max.x ) return false;
        double s = ( Min.x - Q.x ) / R.x;
        double t = ( Max.x - Q.x ) / R.x;
        if( s > min ) { min = s; N = Vec3( -1, 0, 0 ); }
        if( t < max ) { max = t;                       }
        }
    else // Looking down negative X-axis.
        {
        if( Q.x < Min.x ) return false;
        double s = ( Max.x - Q.x ) / R.x;
        double t = ( Min.x - Q.x ) / R.x;
        if( s > min ) { min = s; N = Vec3( 1, 0, 0 ); }
        if( t < max ) { max = t;                      }
        }
    if( min > max ) return false; // Degenerate interval.

    if( R.y >= 0 ) // Looking down positive Y-axis.
        {
        if( Q.y > Max.y ) return false;
        double s = ( Min.y - Q.y ) / R.y;
        double t = ( Max.y - Q.y ) / R.y;
        if( s > min ) { min = s; N = Vec3( 0, -1, 0 ); }
        if( t < max ) { max = t;                       }
        }
    else // Looking down negative Y-axis.
        {
        if( Q.y < Min.y ) return false;
        double s = ( Max.y - Q.y ) / R.y;
        double t = ( Min.y - Q.y ) / R.y;
        if( s > min ) { min = s; N = Vec3( 0, 1, 0 ); }
        if( t < max ) { max = t;                      }
        }
    if( min > max ) return false; // Degenerate interval.

    if( R.z >= 0 ) // Looking down positive Z-axis.
        {
        if( Q.z > Max.z ) return false;
        double s = ( Min.z - Q.z ) / R.z;
        double t = ( Max.z - Q.z ) / R.z;
        if( s > min ) { min = s; N = Vec3( 0, 0, -1 ); }
        if( t < max ) { max = t;                       }
        }
    else // Looking down negative Z-axis.
        {
        if( Q.z < Min.z ) return false;
        double s = ( Max.z - Q.z ) / R.z;
        double t = ( Min.z - Q.z ) / R.z;
        if( s > min ) { min = s; N = Vec3( 0, 0, 1 ); }
        if( t < max ) { max = t;                      }
        }
    if( min > max ) return false; // Degenerate interval.

    // If the ray originates inside the box there is no entry face: no slab
    // ever raised "min" above its starting value, so N was never assigned.
    // Reporting that as a hit would place it at t=0 with a zero normal,
    // which turns into NaN in the shader (and a black pixel).  Treat it as
    // a miss instead -- a solid box has no visible interior.  (If boxes are
    // ever made refractive, this must return the exit face with an inward
    // normal instead.)
    if( min <= 0.0 ) return false;

    // We have an actual hit.  Fill in all the geometric information so
    // that the shader can shade this point.  Compute the point of
    // intersection using the ray and the distance to the point along
    // the ray.

    hitinfo.distance = min;
    hitinfo.point    = ray.origin + min * R;
    hitinfo.normal   = N;
    hitinfo.ray      = ray;
    hitinfo.object   = this;
    hitinfo.material = &material;

    // (KB: planar uv for 2D textures -- the hit point's world coordinates in
    // the plane of the face, so a texture's frequency is in world units and
    // the pattern is continuous across coplanar faces of adjacent boxes.)
    if( N.x != 0 )      hitinfo.uv = Vec2( hitinfo.point.y, hitinfo.point.z );
    else if( N.y != 0 ) hitinfo.uv = Vec2( hitinfo.point.x, hitinfo.point.z );
    else                hitinfo.uv = Vec2( hitinfo.point.x, hitinfo.point.y );
    return true;
    }
int Box::GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const
    {
    // To be filled in later.
    return 0;
    }
