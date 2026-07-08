/***************************************************************************
* util.C                                                                   *
*                                                                          *
* Trivial utilities.                                                       *
*                                                                          *
* History:                                                                 *
*   10/16/2004  Fixed bug in Mat3x3 * Box3 operator.                       *
*   10/10/2004  Broken out from torytracer.C.                              *
*                                                                          *
***************************************************************************/
#include "toytracer.h"
#include <random>

// (KB: rewritten with a per-thread generator so parallel OpenMP rendering
//  is race-free and each thread is decorrelated.)
double rand( double a, double b )
    {
    static thread_local std::mt19937 gen( std::random_device{}() );
    std::uniform_real_distribution<double> dist( a, b );
    return dist( gen );
    }

Interval &Interval::operator+=( const Interval &A )
    {
    if( A.min < min ) min = A.min;
    if( A.max > max ) max = A.max;
    return *this;
    };

Vec3 Box3::Center() const
    {
    return 0.5 * ( Min() + Max() );
    }

Box3 &Box3::operator+=( const Box3 &A )
    { 
    X += A.X;
    Y += A.Y;
    Z += A.Z;
    return *this;
    };

// This operator allows a 3x3 matrix to transform a bounding box.
// The result is another axis-aligned box guaranteed to contain all
// of the the original vertices transformed by the given matrix.

Box3 operator*( const Mat3x3 &M, const Box3 &box )
    {
    double new_min[] = { 0, 0, 0 };
    double new_max[] = { 0, 0, 0 };
    double old_min[] = { box.X.min, box.Y.min, box.Z.min };
    double old_max[] = { box.X.max, box.Y.max, box.Z.max }; 

    // Find the extreme points by considering the product
    // of the min and max with each component of M.
                     
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ )
        {
        double a = M(i,j) * old_min[j];
        double b = M(i,j) * old_max[j];
        new_min[i] += min( a, b ); 
        new_max[i] += max( a, b );
        }

    // Return the smallest bounding box enclosing the extremal points.
    Box3 b;
    b.X = Interval( new_min[0], new_max[0] );
    b.Y = Interval( new_min[1], new_max[1] );
    b.Z = Interval( new_min[2], new_max[2] );
    return b;
    }


