//
//	Box.h
//
//	10/22/2004	Kevin Brandon	-	Broken out of box.cpp
//
//
#ifndef _BOX_H_

#define _BOX_H_

#include "toytracer.h"
#include <stdio.h>

struct Box : public Object
{
    Box() {}
    Box( const Vec3 &Min, const Vec3 &Max );
    virtual bool Intersect( const Ray &ray, HitInfo &hitinfo ) const;
    virtual Box3 GetBounds() const;
    virtual int GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Box"; }
    Vec3 Min; // Minimum coordinates along each axis.
    Vec3 Max; // Maximum coordinates along each axis.
};



// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( Box );

#endif