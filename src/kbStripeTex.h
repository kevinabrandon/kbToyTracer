//
//	kbStripeTex.h
//

#ifndef KB_STRIPETEX_H
#define KB_STRIPETEX_H

#include "kbTexture.h"

class kbStripeTex : public kbTexture
{
	public:
	kbStripeTex( const Color c1, const Color c2, double freq )
	{
		color1 = c1;
		color2 = c2;
		frequency = freq;
	}

	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const
	{
		double twopi = 6.283185307179586476925286766559;
		if( sin( twopi * frequency * uv.x ) > 0 ) return color1;
		else	return color2;
	}

	Color color1, color2;
	double frequency;
};

#endif