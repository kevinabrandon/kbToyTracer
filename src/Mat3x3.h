/***************************************************************************
* Mat3x3.h                                                                 *
*                                                                          *
* Mat3x3 is a 3x3 matrix class, with associated operators.                 *
*                                                                          *
* History:                                                                 *
*   10/16/2004  Added ^ operator for multiplying by the transpose.         *
*   10/10/2004  Added Inverse function.                                    *
*   04/07/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#ifndef _MAT3x3_H_
#define _MAT3x3_H_

#include "Vec3.h"

struct Mat3x3 {
    inline Mat3x3();
    inline Mat3x3( const Mat3x3 &M ) { *this = M; }
    inline ~Mat3x3() {}
    inline       double &operator()( int i, int j )       { return m[i][j]; }
    inline const double &operator()( int i, int j ) const { return m[i][j]; }
    inline static Mat3x3 Identity();
    inline Vec3 row( int i ) const { return Vec3( m[i][0], m[i][1], m[i][2] ); }
    inline Vec3 col( int j ) const { return Vec3( m[0][j], m[1][j], m[2][j] ); }
    double m[3][3];
    };

inline Mat3x3::Mat3x3()
    {
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ ) m[i][j] = 0.0;
    }

inline Mat3x3 operator+( const Mat3x3 &A, const Mat3x3 &B )
    {
    Mat3x3 C;
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ ) C(i,j) = A(i,j) + B(i,j);
    return C;
    }

inline Mat3x3 operator-( const Mat3x3 &A, const Mat3x3 &B )
    {
    Mat3x3 C;
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ ) C(i,j) = A(i,j) - B(i,j);
    return C;
    }

inline Mat3x3 operator*( double c, const Mat3x3 &M )
    {
    Mat3x3 A;
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ ) A(i,j) = c * M(i,j);
    return A;    
    }

inline Mat3x3 operator*( const Mat3x3 &M, double c )
    {
    return c * M;
    }

inline Vec3 operator*( const Mat3x3 &M, const Vec3 &A )
    {
    // Transform the column vector A, multiplying on the left by matrix M.
    return Vec3(
        M(0,0) * A.x + M(0,1) * A.y + M(0,2) * A.z,
        M(1,0) * A.x + M(1,1) * A.y + M(1,2) * A.z,
        M(2,0) * A.x + M(2,1) * A.y + M(2,2) * A.z
        );
    }

inline Vec3 operator^( const Mat3x3 &M, const Vec3 &A )
    {
    // Multiply the vector A on the left by the TRANSPOSE of the matrix.
    return Vec3(
        M(0,0) * A.x + M(1,0) * A.y + M(2,0) * A.z,
        M(0,1) * A.x + M(1,1) * A.y + M(2,1) * A.z,
        M(0,2) * A.x + M(1,2) * A.y + M(2,2) * A.z
        );
    }

inline Mat3x3 operator*( const Mat3x3 &A, const Mat3x3 &B )
    {
    Mat3x3 C;
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ )
        C(i,j) = A(i,0) * B(0,j) + A(i,1) * B(1,j) + A(i,2) * B(2,j);
    return C;
    }

inline Mat3x3 operator/( const Mat3x3 &M, double c )
    {
    return (1/c) * M;
    }

inline Mat3x3 Adjoint( const Mat3x3 &M )  
    {
    // The Adjoint matrix is the inverse transpose times the determinant.
    Mat3x3 A;
    A(0,0) = M(1,1) * M(2,2) - M(1,2) * M(2,1);
    A(0,1) = M(1,2) * M(2,0) - M(1,0) * M(2,2);
    A(0,2) = M(1,0) * M(2,1) - M(1,1) * M(2,0);
 
    A(1,0) = M(0,2) * M(2,1) - M(0,1) * M(2,2);
    A(1,1) = M(0,0) * M(2,2) - M(0,2) * M(2,0);
    A(1,2) = M(0,1) * M(2,0) - M(0,0) * M(2,1);

    A(2,0) = M(0,1) * M(1,2) - M(0,2) * M(1,1);
    A(2,1) = M(0,2) * M(1,0) - M(0,0) * M(1,2);
    A(2,2) = M(0,0) * M(1,1) - M(0,1) * M(1,0);
    return A;
    }

inline double det( const Mat3x3 &M )  // Determinant.
    {
    return
        M(0,0) * ( M(1,1) * M(2,2) - M(1,2) * M(2,1) )
      - M(0,1) * ( M(1,0) * M(2,2) - M(1,2) * M(2,0) )
      + M(0,2) * ( M(1,0) * M(2,1) - M(1,1) * M(2,0) );
    }

inline Mat3x3 Transpose( const Mat3x3 &M )
    {
    Mat3x3 W;
    for( int i = 0; i < 3; i++ )
    for( int j = 0; j < 3; j++ ) W(i,j) = M(j,i);
    return W;
    }

inline Mat3x3 Inverse( const Mat3x3 &M )
    {
    double d = det( M );
    return Transpose( Adjoint( M ) ) / d;
    }

inline Mat3x3 Mat3x3::Identity()
    {
    Mat3x3 I;
    I(0,0) = 1.0;
    I(1,1) = 1.0;
    I(2,2) = 1.0;
    return I;
    }

inline Mat3x3 Rotate_X( double angle )
    {
    // Rotation about the X-axis.
    Mat3x3 M = Mat3x3::Identity();
    M(1,1) = cos( angle );  M(1,2) = -sin( angle );
    M(2,1) = sin( angle );  M(2,2) =  cos( angle );
    return M;
    }

inline Mat3x3 Rotate_Y( double angle )
    {
    // Rotation about the Y-axis.
    Mat3x3 M = Mat3x3::Identity();
    M(0,0) = cos( angle );  M(0,2) = -sin( angle );
    M(2,0) = sin( angle );  M(2,2) =  cos( angle );
    return M;
    }

inline Mat3x3 Rotate_Z( double angle )
    {
    // Rotation about the Z-axis.
    Mat3x3 M = Mat3x3::Identity();
    M(0,0) = cos( angle );  M(0,1) = -sin( angle );
    M(1,0) = sin( angle );  M(1,1) =  cos( angle );
    return M;
    }

inline Mat3x3 Scale( double x, double y, double z )
    {
    // Allows non-uniform scaling along all three coordinate axes.
    Mat3x3 M;
    M(0,0) = x;
    M(1,1) = y;
    M(2,2) = z;
    return M;
    }

inline std::ostream &operator<<( std::ostream &out, const Mat3x3 &M )
    {
    out << "\n";
    out << "| " << M(0,0) << " " << M(0,1) << " " << M(0,2) << " |\n";
    out << "| " << M(1,0) << " " << M(1,1) << " " << M(1,2) << " |\n";
    out << "| " << M(2,0) << " " << M(2,1) << " " << M(2,2) << " |\n";
    out << std::endl;
    return out;
    }

#endif
