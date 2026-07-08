/***************************************************************************
* Mat3x4.h                                                                 *
*                                                                          *
* Mat3x4 is a 3x4 matrix class, with associated operators.  This object    *
* behaves exactly like the 4x4 matrix formed by appending the last row of  *
* the 4x4 identity matrix to it, and appending a fourth element of 1 to    *
* each vector it is multiplied with.                                       *
*                                                                          *
* History:                                                                 *
*   10/10/2004  Added Inverse function.                                    *
*   10/06/2004  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#ifndef _MAT3x4_H_
#define _MAT3x4_H_

#include "Mat3x3.h"
#include "Vec3.h"

struct Mat3x4 {
    inline Mat3x4() { mat = Mat3x3::Identity(); }
    inline Mat3x4( const Mat3x3 &m                ) { mat = m;          }
    inline Mat3x4( const Mat3x3 &m, const Vec3 &v ) { mat = m; vec = v; }
    inline Mat3x4( const Mat3x4 &m ) { *this = m; }
    inline ~Mat3x4() {}
    inline static Mat3x4 Identity();
    Mat3x3 mat;
    Vec3   vec;
    };

inline Mat3x4 operator+( const Mat3x4 &A, const Mat3x4 &B )
    {
    return Mat3x4( A.mat + B.mat, A.vec + B.vec );
    }

inline Mat3x4 operator-( const Mat3x4 &A, const Mat3x4 &B )
    {
    return Mat3x4( A.mat - B.mat, A.vec - B.vec );
    }

inline Mat3x4 operator*( double c, const Mat3x4 &M )
    {
    return Mat3x4( c * M.mat, c * M.vec );
    }

inline Mat3x4 operator*( const Mat3x4 &M, double c )
    {
    return c * M;
    }

inline Vec3 operator*( const Mat3x4 &M, const Vec3 &A )
    {
    return Vec3( M.mat * A + M.vec );
    }

inline Mat3x4 operator*( const Mat3x4 &A, const Mat3x4 &B )
    {
    return Mat3x4( A.mat * B.mat, A.mat * B.vec + A.vec );
    }

inline Mat3x4 operator*( const Mat3x3 &A, const Mat3x4 &B )
    {
    // Assume translation of the A matrix is zero.
    return Mat3x4( A * B.mat, A * B.vec  );
    }

inline Mat3x4 operator*( const Mat3x4 &A, const Mat3x3 &B )
    {
    // Assume translation of the B matrix is zero.
    return Mat3x4( A.mat * B, A.vec );
    }

inline Mat3x4 operator/( const Mat3x4 &M, double c )
    {
    return (1/c) * M;
    }

inline Mat3x4 Inverse( const Mat3x4 &M )
    {
    Mat3x3 W( Inverse( M.mat ) );
    return Mat3x4( W, -( W * M.vec ) );
    }

inline Mat3x4 Mat3x4::Identity()
    {
    Mat3x4 I;
    I.mat(0,0) = 1.0;
    I.mat(1,1) = 1.0;
    I.mat(2,2) = 1.0;
    return I;
    }

inline Mat3x4 Translate( double x, double y, double z )
    {
    return Mat3x4( Mat3x3::Identity(), Vec3( x, y, z ) );
    }

inline std::ostream &operator<<( std::ostream &out, const Mat3x4 &M )
    {
    // Write out a 3x4 matrix in the same format that the toytracer parser can read.
    out << "\n( "; 
    out << M.mat(0,0) << ", " << M.mat(0,1) << ", " << M.mat(0,2) << ", " << M.vec.x << "; ";
    out << M.mat(1,0) << ", " << M.mat(1,1) << ", " << M.mat(1,2) << ", " << M.vec.y << "; ";
    out << M.mat(2,0) << ", " << M.mat(2,1) << ", " << M.mat(2,2) << ", " << M.vec.z;
    out << " )" << std::endl;
    return out;
    }

#endif
