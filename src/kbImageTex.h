//
//	kbImageTex.h
//

#ifndef _KBIMAGETEX_H_
#define _KBIMAGETEX_H_

#include "kbTexture.h"
#include "Image.h"

class kbImageTex : public kbTexture
{
public:
	kbImageTex( const char *file_name );
	~kbImageTex( );
	virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const;

private:
	Image *image;

};


#endif