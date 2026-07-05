//
//	kbImageTex.cpp
//

#include "kbImageTex.h"

kbImageTex::kbImageTex( const char *file_name )
{
	image = new Image( file_name );
}

kbImageTex::~kbImageTex( )
{
	delete image;
}

Color kbImageTex::GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const 
{
   // u & v *= dimensions minus the slop(2) and the zero base difference (1)
   // for a total of 3
	
   float u = ( float ) uv.x - ( int ) ( uv.x );
   float v = ( 1 - ( float ) uv.y - ( int ) ( uv.y ) ); 

  u *= ( image->width - 3 );
  v *= ( image->height - 3 );

   int iu = ( int ) u;
   int iv = ( int ) v;

   float tu = u - iu;
   float tv = v - iv;
   
   Color c = image->kbGetPixel(iu,iv)*(1-tu)*(1-tv)+
	   image->kbGetPixel(iu+1,iv)*tu*(1-tv)+
	   image->kbGetPixel(iu,iv+1)*(1-tu)*tv+
	   image->kbGetPixel(iu+1,iv+1)*tu*tv;
	

//   Color c = image->kbGetPixel( uv.x*image->width, uv.y * image->height );
   return c;
}
