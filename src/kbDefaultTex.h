//
//	kbDefaultTex.h
//	

#ifndef KB_DEFAULTTEX_H
#define KB_DEFAULTTEX_H

#include "kbTexture.h"

class kbStripeTex : public kbTexture
{
	public:
	kbStripeTex( const Color c )	{ color = c; }

	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const	{ return color; }

	Color color;
};

#endif