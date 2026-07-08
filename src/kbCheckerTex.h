//
//	kbCheckerTex.h
//

#ifndef KB_CHECKERTEX_H
#define KB_CHECKERTEX_H

#include <math.h>
#include "toytracer.h"
#include "kbTexture.h"

class kbCheckerTex : public kbTexture
{
	public:
	kbCheckerTex( const Color c1, const Color c2, double freq )
	{
		color1 = c1;
		color2 = c2;
		frequency = freq;
	}

	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const
	{
		int a, b;
		if( sin( TwoPi * frequency * uv.x ) > 0 )	a = 1;
		else										a = 0;
		if( sin( TwoPi * frequency * uv.y ) > 0 )	b = 1;
		else										b = 0;
		if( ( a && b ) || ( !a && !b ) )	return color1;
		else								return color2;
	}

	Color color1, color2;
	double frequency;
};

#endif