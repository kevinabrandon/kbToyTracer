//
//	kbTexture.h
//

#ifndef KB_TEXTURE_H
#define KB_TEXTURE_H

#include "Vec2.h"
#include "Vec3.h"
#include "Color.h"

class kbTexture
{
public:
	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const = 0;
};

#endif

