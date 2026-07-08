/***************************************************************************
* Color.h                                                                  *
*                                                                          *
* Color is a trivial encapsulation of floating-point RGB colors.  It has   *
* the obvious operators defined as inline functions.  Note that there is   *
* no subtraction operator for colors, and that multiplication is performed *
* component-wise and results in another color.                             *
*                                                                          *
* History:                                                                 *
*   04/01/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#ifndef _COLOR_H_
#define _COLOR_H_

#include <iostream>

struct Color {
    inline Color() { red = 0; green = 0; blue = 0; }
    inline Color( double r, double g, double b ) { red = r; green = g; blue = b; }
    double red;
    double green;
    double blue;
    };

inline Color operator+( const Color &A, const Color &B )
    {
    return Color( A.red + B.red, A.green + B.green, A.blue + B.blue );
    }

inline Color operator*( double c, const Color &A )
    {
    return Color( c * A.red, c * A.green, c * A.blue );
    }

inline Color operator*( const Color &A, double c )
    {
    return Color( c * A.red, c * A.green, c * A.blue );
    }

inline Color operator*( const Color &A, const Color &B )
    {
    // Colors are multiplied component-wise, and result in another color, not
    // a scalar.  This is the most significant difference between the Vec3 class
    // and the Color class.
    return Color( A.red * B.red, A.green * B.green, A.blue * B.blue );
    }

inline Color operator/( const Color &A, double c )
    {
    return Color( A.red / c, A.green / c, A.blue / c );
    }

inline Color& operator+=( Color &A, const Color &B )
    {
    A.red   += B.red;
    A.green += B.green;
    A.blue  += B.blue;
    return A;
    }

inline Color& operator*=( Color &A, double c )
    {
    A.red   *= c;
    A.green *= c;
    A.blue  *= c;
    return A;
    }

inline Color& operator/=( Color &A, double c )
    {
    A.red   /= c;
    A.green /= c;
    A.blue  /= c;
    return A;
    }

inline bool operator==( const Color &A, const Color &B )
    {
	return (A.red == B.red) && (A.green == B.green) && (A.blue == B.blue);
    }

inline bool operator==( const Color &A, double c )
    {
	return (A.red == c) && (A.green == c) && (A.blue == c);
    }

inline bool operator!=( const Color &A, double c )
    {
	return !( A == c );
    }

inline std::ostream &operator<<( std::ostream &out, const Color &C )
    {
    out << "[ " << C.red << ", " << C.green << ", " << C.blue << " ] ";
    return out;
    }

#endif




