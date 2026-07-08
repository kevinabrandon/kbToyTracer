//
//	kbPng.h
//
//	Dependency-free PNG output.  The encoder emits a valid PNG using
//	zlib "stored" (uncompressed) deflate blocks -- about the same size
//	as a PPM, but viewable everywhere.
//

#ifndef KB_PNG_H
#define KB_PNG_H

#include "Image.h"

// Write the image as PNG.
extern bool kbWritePNG( const Image &img, const char *file_name );

// Write by extension: ".png" -> kbWritePNG, anything else -> PPM (Image::Write).
extern bool kbWriteImage( Image &img, const char *file_name );

#endif
