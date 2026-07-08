//
//	kbImageTex.h
//

#ifndef KB_IMAGETEX_H
#define KB_IMAGETEX_H

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