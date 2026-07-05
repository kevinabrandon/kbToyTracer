//
//	kbDefaultTex.h
//	

#ifndef _KBDEFAULTTEX_H_
#define _KBDEFAULTTEX_H_

#include "kbTexture.h"

class kbStripeTex : public kbTexture
{
	public:
	kbStripeTex( const Color c )	{ color = c; }

	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const	{ return color; }

	Color color;
};

#endif