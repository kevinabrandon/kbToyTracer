#ifndef MARBLE_TEXTURE_H
#define MARBLE_TEXTURE_H


#include "kbTexture.h"
#include "toytracer.h"
#include "SolidNoise.h"


class kbMarbleTexture : public kbTexture 
{
public:
   kbMarbleTexture(float stripes_per_unit, float _scale = 10.0f, int _octaves = 8) 
   {
      freq = ( float ) Pi * stripes_per_unit;
      scale = _scale;
      octaves = _octaves;
      c0 = Color(0.8,0.8,0.8);
      c1 = Color(0.4,0.2,0.1);
      c2 = Color(0.06, 0.04,0.02);
      noise = new SolidNoise(2345);
   }
   kbMarbleTexture(const Color & _c0, const Color & _c1, const Color & _c2, float stripes_per_unit, float _scale = 10.0f, int _octaves = 8)
   {
	   c0 = _c0;
	   c1 = _c1;
	   c2 = _c2;
      freq = ( float ) Pi * stripes_per_unit;
      scale = _scale;
      octaves = _octaves;
      noise = new SolidNoise(2345);
   }
      

   virtual Color GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const;

   float freq, scale;
   int octaves;
   Color c0, c1, c2;
   SolidNoise *noise;
};


#endif

