//
//	kbTexture.h
//

#ifndef _KBTEXTURE_H_
#define _KBTEXTURE_H_

#include "Vec2.h"
#include "Vec3.h"
#include "Color.h"

class kbTexture
{
public:
	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const = 0;
};

#endif

