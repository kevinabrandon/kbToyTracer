/***************************************************************************
* quad.C    (primitive object)                                             *
*                                                                          *
* The quad object is defined by four (planar) vertices in R3.  This is a   *
* simple flat quad with no normal vector interpolation.                    *
*                                                                          *
* History:                                                                 *
*   10/23/2004  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include "toytracer.h"

struct Quad : public Object {
    Quad() {}
    Quad( const Vec3 &A, const Vec3 &B, const Vec3 &C, const Vec3 &D );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Quad"; }
    Vec3   N;    // Normal to plane of quad;
    double d;    // Distance from origin to plane of quad.
    double area; // Area of the quad.
    Box3   box;  // Bounding box;
    Vec3   Eab, Ebc, Ecd, Eda;
    Vec3   A, B, C, D;  
    };

// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( Quad );

Quad::Quad( const Vec3 &A_, const Vec3 &B_, const Vec3 &C_, const Vec3 &D_ )
    {
    A = A_; // Store the vertices.
    B = B_;
    C = C_;
    D = D_;

    // Compute the quad normal by summing the triangle normals.  The area of the quad
    // cal also be computed in this way; it's just 1/2 the length of the resulting vector.
   
    Vec3 O = 0.25 * (A + B + C + D);
    Vec3 V = ((A - O)^(B - O)) + ((B - O)^(C - O)) + ((C - O)^(D - O)) + ((D - O)^(A - O));
    area = 0.5 * Length( V );
    N = -Unit( V );
    d = O * N;

    // Compute unit vectors in the direction of each edge.  These will be used for the
    // in-out test, to see if a point on the plane of the quad is inside the quad or not.

    Eab = Unit( B - A );
    Ebc = Unit( C - B );
    Ecd = Unit( D - C );
    Eda = Unit( A - D );

    // The bounding box will simply be the bounding box of the three vertices.

    box.X = Interval( min( min( A.x, B.x ), min( C.x, D.x ) ), max( max( A.x, B.x ), max( C.x, D.x ) ) ); 
    box.Y = Interval( min( min( A.y, B.y ), min( C.y, D.y ) ), max( max( A.y, B.y ), max( C.y, D.y ) ) ); 
    box.Z = Interval( min( min( A.z, B.z ), min( C.z, D.z ) ), max( max( A.z, B.z ), max( C.z, D.z ) ) ); 
    }

Object *Quad::ReadString( const char *params ) // Read params from string.
    {
    float ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz;
    if( sscanf( params, "quad (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", 
        &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz ) == 12 )
        return new Quad( Vec3( ax, ay, az ), Vec3( bx, by, bz ), Vec3( cx, cy, cz ), Vec3( dx, dy, dz ) );
    return NULL;
    }

Box3 Quad::GetBounds() const // Return pre-computed box.
    {
    return box;
    }

bool Quad::Intersect( const Ray &ray, HitInfo &hitinfo ) const
    {
    // Compute the point of intersection with the plane containing the quad.
    // Report a miss if the ray does not hit this plane.

    double denom = ray.direction * N;
    if( fabs(denom) < 1.0E-4 ) return false;
    double s = ( d - ray.origin * N ) / denom;
    if( s <= 0.0 || s > hitinfo.distance ) return false;
    Vec3 P = ray.origin + s * ray.direction;

    // Compute a sequence of cross products using the quad edges.  The point P is inside
    // the quad if and only if each vector dotted with the quad normal is positive.

    if( (( P - A ) ^ Eab) * N < 0.0 ) return false; 
    if( (( P - B ) ^ Ebc) * N < 0.0 ) return false;
    if( (( P - C ) ^ Ecd) * N < 0.0 ) return false;
    if( (( P - D ) ^ Eda) * N < 0.0 ) return false;

    // We have an actual hit.  Fill in all the geometric information so
    // that the shader can shade this point.

    hitinfo.distance = s;
    hitinfo.point    = P;
    hitinfo.normal   = -N;  // (KB: flipped to match kbToyTracer's winding convention.)
    hitinfo.ray      = ray;
    hitinfo.object   = this;
    hitinfo.uv       = Vec2();  // (KB: quads carry no texture parameterization yet.)
    return true;
    }

// This function generates nxn stratified samples over the surface of the quad.
// The weight of each sample is the quad area / n^2, times the area-to-solid-angle
// conversion factor of cos(theta)/r^2, where theta is the incident angle on the
// quad, and r is the distance between the point O and the given sample P.
// (NOTE: currently this is only correct for parallelograms.  To handle arbitrary
// quads correctly, we must also compute the Jacobian for each sample point.)

int Quad::GetSamples( const Vec3 &O, const Vec3 &N1, Sample *samples, int n ) const
    {
    int k = 0;
    double darea = area / ( n * n );  // differential area.
    double delta = 1.0 / n;

    // Compute an n by n grid of stratified (i.e. jittered) samples over the quad.
    // Use bilinear interpolation to parametrize the quad.  Fill in the array of
    // samples as we loop over them, weighting each sample by the correct conversion factor.

    for( int i = 0; i < n; i++ )
    for( int j = 0; j < n; j++ )
        {
        double s = ( i + rand( 0.0, 0.999 ) ) * delta;
        double t = ( j + rand( 0.0, 0.999 ) ) * delta;
        Vec3 P = (1.0 - s) * ( (1.0 - t) * A + t * B ) + s * ( (1.0 - t) * D + t * C );
        Vec3 U = Unit( P - O );
        samples[k].P = P;
        samples[k].w = darea * fabs( N * U ) / LengthSquared( P - O );
        k++;
        }

    // Return the number of samples generated.  For quads, this will always be the
    // square of the number requested.

    return n * n;
    }



