/***************************************************************************
* kbSmoothTriangle.cpp  (primitive object)                                 *
*                                                                          *
* See kbSmoothTriangle.h.  The plane intersection and barycentric test     *
* mirror Arvo's flat triangle; the same barycentric coordinates then       *
* interpolate the vertex normals and uvs.                                  *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include "kbSmoothTriangle.h"

REGISTER_OBJECT( kbSmoothTriangle );

kbSmoothTriangle::kbSmoothTriangle(
    const Vec3 &A_, const Vec3 &B_, const Vec3 &C_,
    const Vec3 &NA_, const Vec3 &NB_, const Vec3 &NC_,
    const Vec2 &TA_, const Vec2 &TB_, const Vec2 &TC_ )
{
    A = A_;  B = B_;  C = C_;
    NA = Unit( NA_ );  NB = Unit( NB_ );  NC = Unit( NC_ );
    TA = TA_;  TB = TB_;  TC = TC_;

    N = -Unit( (A - B) ^ (C - B) ); // Geometric normal, kbToyTracer winding convention.
    d = A * N;

    // The coordinate axis closest to orthogonal to the triangle.
    axis = 0;
    if( fabs(N.y) >= fabs(N.x) && fabs(N.y) >= fabs(N.z) ) axis = 1;
    if( fabs(N.z) >= fabs(N.x) && fabs(N.z) >= fabs(N.y) ) axis = 2;

    // Matrix whose inverse yields barycentric coordinates (see triangle.C).
    Mat3x3 W;
    W(0,0) = A.x; W(0,1) = B.x; W(0,2) = C.x;
    W(1,0) = A.y; W(1,1) = B.y; W(1,2) = C.y;
    W(2,0) = A.z; W(2,1) = B.z; W(2,2) = C.z;
    W(axis,0) = 1.0;
    W(axis,1) = 1.0;
    W(axis,2) = 1.0;
    M = Inverse( W );

    box.X = Interval( min( A.x, min( B.x, C.x ) ), max( A.x, max( B.x, C.x ) ) );
    box.Y = Interval( min( A.y, min( B.y, C.y ) ), max( A.y, max( B.y, C.y ) ) );
    box.Z = Interval( min( A.z, min( B.z, C.z ) ), max( A.z, max( B.z, C.z ) ) );
}

bool kbSmoothTriangle::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{
    // Intersect the plane of the triangle.
    double denom = ray.direction * N;
    if( fabs(denom) < 1.0E-4 ) return false;
    double s = ( d - ray.origin * N ) / denom;
    if( s <= 0.0 || s > hitinfo.distance ) return false;
    Vec3 P = ray.origin + s * ray.direction;

    // Right-hand side for the barycentric solve: P with the dominant-axis
    // coordinate replaced by 1.
    Vec3 Q = P;
    switch( axis )
    {
        case 0: Q.x = 1.0; break;
        case 1: Q.y = 1.0; break;
        case 2: Q.z = 1.0; break;
    }

    // Barycentric coordinates; the ray hits iff all three are non-negative.
    double b0 = M(0,0) * Q.x + M(0,1) * Q.y + M(0,2) * Q.z;
    double b1 = M(1,0) * Q.x + M(1,1) * Q.y + M(1,2) * Q.z;
    double b2 = M(2,0) * Q.x + M(2,1) * Q.y + M(2,2) * Q.z;
    if( b0 < 0.0 || b1 < 0.0 || b2 < 0.0 ) return false;

    hitinfo.distance = s;
    hitinfo.point    = P;
    hitinfo.normal   = Unit( b0 * NA + b1 * NB + b2 * NC );
    hitinfo.uv       = Vec2( b0 * TA.x + b1 * TB.x + b2 * TC.x,
                             b0 * TA.y + b1 * TB.y + b2 * TC.y );
    hitinfo.ray      = ray;
    hitinfo.object   = this;
    hitinfo.material = &material;
    return true;
}

Object *kbSmoothTriangle::ReadString( const char *params )
{
    float ax,ay,az, bx,by,bz, cx,cy,cz, nax,nay,naz, nbx,nby,nbz, ncx,ncy,ncz;
    if( sscanf( params,
        "smooth_triangle (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
        &ax,&ay,&az, &bx,&by,&bz, &cx,&cy,&cz,
        &nax,&nay,&naz, &nbx,&nby,&nbz, &ncx,&ncy,&ncz ) == 18 )
        return new kbSmoothTriangle(
            Vec3( ax,ay,az ), Vec3( bx,by,bz ), Vec3( cx,cy,cz ),
            Vec3( nax,nay,naz ), Vec3( nbx,nby,nbz ), Vec3( ncx,ncy,ncz ) );
    return NULL;
}
