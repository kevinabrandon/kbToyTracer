
#include <math.h>
#include "kbMarbleTex.h"


Color kbMarbleTexture::GetColorFromUV( const Vec2 &uv, const Vec3 &hitPoint ) const
{
   float t = ( float ) (sin(freq*hitPoint.x + scale*noise->turbulence(freq*hitPoint, octaves))) +1.0f;
   //float t = 2.0f*fabs(sin(freq*p.x() + scale*noise.turbulence(freq*p, octaves)));

   if (t < 1.0f)
      return (c1*t + (1.0f - t)*c2);
   else
   {
      t -= 1.0f;
      return (c0*t + (1.0f - t)*c1);
   }
}

