/***************************************************************************
* triangle.C    (primitive object)                                         *
*                                                                          *
* The triangle object is defined by three vertices in R3.  This is a       *
* simple flat triangle with no normal vector interpolation.  The           *
* triangle structure is defined to accommodate the barycentric coord       *
* method of intersecting a ray with a triangle.                            *
*                                                                          *
* History:                                                                 *
*   10/23/2004  Fixed bug in bounding box computation.                     *
*   10/10/2004  Broken out of objects.C file.                              *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include "toytracer.h"

struct Triangle : public Object {
    Triangle() {}
    Triangle( const Vec3 &A, const Vec3 &B, const Vec3 &C );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Triangle"; }
    Mat3x3 M;    // Inverse of barycentric coord transform.
    Vec3   N;    // Normal to plane of triangle;
    double d;    // Distance from origin to plane of triangle.
    Box3   box;  // Bounding box;
    int    axis; // The dominant axis;
    Vec3   A;
    Vec3   B;
    Vec3   C;
    };

// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( Triangle );

Triangle::Triangle( const Vec3 &A_, const Vec3 &B_, const Vec3 &C_ )
    {
    A = A_; // Store the vertices.
    B = B_;
    C = C_;

    N = -Unit( (A - B) ^ (C - B) ); // Compute the normal vector.
    d = A * N;

    // Determine which axis is the "dominant" axis of the triangle.  That is, which
    // coordinate axis is closest to being orthogonal to the triangle.

    if( fabs(N.x) >= fabs(N.y) && fabs(N.x) >= fabs(N.z) ) axis = 0;
    if( fabs(N.y) >= fabs(N.x) && fabs(N.y) >= fabs(N.z) ) axis = 1;
    if( fabs(N.z) >= fabs(N.x) && fabs(N.z) >= fabs(N.y) ) axis = 2;

   // Set up the matrix for finding the barycentric coordinates.

    Mat3x3 W;
    W(0,0) = A.x; W(0,1) = B.x; W(0,2) = C.x;
    W(1,0) = A.y; W(1,1) = B.y; W(1,2) = C.y;
    W(2,0) = A.z; W(2,1) = B.z; W(2,2) = C.z;
    W(axis,0) = 1.0;
    W(axis,1) = 1.0;
    W(axis,2) = 1.0;

    // Store the inverse of this matrix.  It will provide an efficient means of
    // finding the barycentric coordinates of the point of intersection with the
    // plane containing the triangle.

    M = Inverse( W );

    // The bounding box will simply be the bounding box of the three vertices.

    box.X = Interval( min( A.x, min( B.x, C.x ) ), max( A.x, max( B.x, C.x ) ) ); 
    box.Y = Interval( min( A.y, min( B.y, C.y ) ), max( A.y, max( B.y, C.y ) ) ); 
    box.Z = Interval( min( A.z, min( B.z, C.z ) ), max( A.z, max( B.z, C.z ) ) ); 
    }

Object *Triangle::ReadString( const char *params ) // Read params from string.
    {
    float ax, ay, az, bx, by, bz, cx, cy, cz;
    if( sscanf( params, "triangle (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", 
        &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz ) == 9 )
        return new Triangle( Vec3( ax, ay, az ), Vec3( bx, by, bz ), Vec3( cx, cy, cz ) );
    return NULL;
    }

Box3 Triangle::GetBounds() const // Return pre-computed box.
    {
    return box;
    }

bool Triangle::Intersect( const Ray &ray, HitInfo &hitinfo ) const
    {
    // Compute the point of intersection with the plane containing the triangle.
    // Report a miss if the ray does not hit this plane.

    double denom = ray.direction * N;
    if( fabs(denom) < 1.0E-4 ) return false;
    double s = ( d - ray.origin * N ) / denom;
    if( s <= 0.0 || s > hitinfo.distance ) return false;
    Vec3 P = ray.origin + s * ray.direction;

    // Create a new vector that is a copy of P, but with one coordinate (corresponding
    // to the dominant exis) set to 1.  This is the right-hand-side of the equation for
    // the barycentric coordinates.

    Vec3 Q = P;
    switch( axis )
        {
        case 0: Q.x = 1.0; break;
        case 1: Q.y = 1.0; break;
        case 2: Q.z = 1.0; break;
        }

    // Solve for the barycentric coordinates and check their sign.  The ray hits the triangle
    // if and only of all the barycentric coordinates are positive.

    if( M(0,0) * Q.x + M(0,1) * Q.y + M(0,2) * Q.z < 0.0 ) return false;
    if( M(1,0) * Q.x + M(1,1) * Q.y + M(1,2) * Q.z < 0.0 ) return false;
    if( M(2,0) * Q.x + M(2,1) * Q.y + M(2,2) * Q.z < 0.0 ) return false;

    // We have an actual hit.  Fill in all the geometric information so
    // that the shader can shade this point.

    hitinfo.distance = s;
    hitinfo.point    = P; 
    hitinfo.normal   = N;
    hitinfo.ray      = ray;
    hitinfo.object   = this;
    return true;
    }

int Triangle::GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const
    {
    int count = 0;
    Vec3    W = (A - B) ^ (C - B);
    double dA = Length( W ) / ( 2.0 * n * n );
    W = Unit( W );
    for( int i = 0; i < n; i++ )
    for( int j = 0; j < n; j++ )
        {
        double s = sqrt( ( i + rand(0,1) ) / n );
        double t =       ( j + rand(0,1) ) / n;
        Vec3 Q = (1.0 - s) * A + (s - s * t) * B + ( s * t ) * C;
        Vec3 R = Q - P;
        double d = Length( R );
        double c = fabs( R * W / d );
        samples[ count ].P = Q;
        samples[ count ].w = dA * c / ( d * d );
        count++;
        }
    return n * n;
    }



