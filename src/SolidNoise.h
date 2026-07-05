#ifndef SOLID_NOISE_H
#define SOLID_NOISE_H

#include <stdlib.h>
#include <math.h>
#include "Vec3.h"

class SolidNoise {
public:
    SolidNoise();
    SolidNoise(int seed);
    float noise(const Vec3&) const;
    Vec3 vectorNoise(const Vec3&) const;
    Vec3 vectorTurbulence( const Vec3&, int) const ;
    float turbulence(const Vec3&, int) const;
    float dturbulence(const Vec3&, int, float) const;
    float omega(float) const;
    Vec3 gamma(int, int, int) const;
    int intGamma(int, int) const;
    float knot(int, int, int, Vec3&) const;
    Vec3 vectorKnot(int, int, int, Vec3&) const;
    
    Vec3 grad[16];
    int phi[16];
};


inline float SolidNoise::omega(float t) const {
   t = (t > 0.0f)? t : -t;
   return (t < 1.0f)?  (-6.0f*t*t*t*t*t + 15.0f*t*t*t*t -10.0f*t*t*t + 1) : 0.0f;
}

inline Vec3 SolidNoise::gamma(int i, int j, int k) const
{
   int idx;
   idx = phi[abs(k)%16];
   idx = phi[abs(j + idx) % 16];
   idx = phi[abs(i + idx) % 16];
   return grad[idx];
}

inline float SolidNoise::knot(int i, int j, int k, Vec3& v) const {
  return ( omega( ( float ) v.x) * omega( ( float ) v.y) * omega( ( float ) v.z) * ( float ) ( v * gamma(i,j,k)) );
}

inline Vec3 SolidNoise::vectorKnot( int i, int j, int k, Vec3& v)
const {
    return ( omega(( float ) v.x) * omega( ( float ) v.y) * omega( ( float ) v.z) * gamma(i,j,k) );
}

inline int SolidNoise::intGamma(int i, int j) const {
   int idx;
   idx = phi[abs(j)%16];
   idx = phi[abs(i + idx) % 16];
   return idx;
}


#endif

