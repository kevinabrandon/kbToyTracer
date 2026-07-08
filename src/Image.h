/***************************************************************************
* Image.h                                                                  *
*                                                                          *
* The image class defines a trivial encoding for images known as PPM       *
* format; it simply consists of an array or RGB triples, with one byte     *
* per component, preceeded by a simple header giving the size of the       *
* image.                                                                   *
*                                                                          *
* History:                                                                 *
*   04/01/2003  Initial coding.                                            *
* Provenance:                                                              *
* Original: James Arvo (EECS 204, UC Irvine).  Extended by Kevin           *
* Brandon: PPM-reading constructor and kbGetPixel, used by the             *
* image-map texture (kbImageTex).                                          *
*                                                                          *
***************************************************************************/
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <stdio.h>
#include "Color.h"

typedef unsigned char channel;  

struct Pixel {
    Pixel() { r = 0; g = 0; b = 0; }
    Pixel( channel r_, channel g_, channel b_ ) { r = r_; g = g_; b = b_; }
    channel r;
    channel g;
    channel b;
    };

struct Image {
    inline Image( int x_res, int y_res );
	inline Image( const char *file_name );
    inline ~Image() 
	{ 
		delete[] pixels; 
	}
    inline bool Write( const char *file_name );
    inline Pixel &operator()( int i, int j ) { return *( pixels + ( i * width + j ) ); }  
	inline Color kbGetPixel( int x, int y );
    Pixel *pixels;
    int    width;
    int    height;
    };

inline Image::Image( int x_res, int y_res )
    {
    width  = x_res;
    height = y_res;
    pixels = new Pixel[ width * height ];
    Pixel *p = pixels;
    for( int i = 0; i < width * height; i++ ) *p++ = Pixel(0,0,0);
    }

inline Image::Image( const char *file_name )
{
	FILE *fp = fopen( file_name, "rb" );
	if( fp == NULL ) 
	{	
		printf( "error couldn't open file\n" );
		return;
	}

	fscanf( fp, "P6\n" );
	
	long position = ftell( fp );

	if( fgetc( fp ) == '#' )	while( fgetc( fp ) != '\n' );	
	else						fseek( fp, position, 0 );
		
	int value = fscanf( fp, "%d %d\n255\n", &width, &height );
	if( value != 2 )
	{
		printf( "error reading image file: %s\n", file_name );
		return;
	}
    
	if( width <= 0 || height <= 0 )		return;
	Pixel *p = pixels = new Pixel[ width * height ];
	for( int i = 0; i < width * height; i++ )
	{
		if( value = fscanf( fp, "%c%c%c", &p->r, &p->g, &p->b ) != 3 )
		{
			printf( "error reading ppm image: %i, %i\n", i, value );
			break;
		}
		p++;
	}
	fclose( fp );
}

inline Color Image::kbGetPixel( int x, int y )
{

	if( x < 0 || x > width ) return Color( 0, 0, 0 );
	if( y < 0 || y > height ) return Color( 1, 1, 1 );	

	double r, g, b;
	int index = x + width * y;

	r = pixels[ index ].r / 255.0;
	g = pixels[ index ].g / 255.0;
	b = pixels[ index ].b / 255.0;

	return Color( r, g, b );
}

inline bool Image::Write( const char *file_name )
    {
    Pixel *p = pixels;
    FILE  *fp = fopen( file_name, "w+b" );
    if( fp == NULL ) return false; 
    fprintf( fp, "P6\n%d %d\n255\n", width, height );
    for( int i = 0; i < width * height; i++ )
        {
        fprintf( fp, "%c%c%c", p->r, p->g, p->b );
        p++;
        }
    fclose( fp );
    return true;
    }

#endif
