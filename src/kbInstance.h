/***************************************************************************
* kbInstance.h                                                             *
*                                                                          *
* An instance shares one prototype object (typically a named aggregate     *
* created by a "define" or an "import" statement) among many placements    *
* in a scene.  The placement transform itself is NOT stored here: it       *
* rides on the enclosing aggregate's List_node, exactly like a plain       *
* "matrix" line on any other child, so the existing inverse-ray transform  *
* path in List/BVH does all the geometric work.  The wrapper's job is the  *
* material: after a hit anywhere inside the (shared) prototype, the hit    *
* is re-stamped with this instance's material, so every instance can be    *
* shaded differently.  Note this repaints the WHOLE prototype uniformly;   *
* per-part materials inside a prototype are flattened (v1 semantics).      *
*                                                                          *
* Instances are created by the scene reader (kbReader.cpp), never from a   *
* registered-object line, so this class is not REGISTER_OBJECT'ed.         *
*                                                                          *
* History:                                                                 *
*   07/2026  Initial coding (K. Brandon).                                  *
*                                                                          *
***************************************************************************/
#ifndef KB_INSTANCE_H
#define KB_INSTANCE_H

#include "toytracer.h"

struct kbInstance : public Object
{
    kbInstance( Object *p ) : proto( p ) { material = p->material; }

    virtual bool Intersect( const Ray &ray, HitInfo &hitinfo ) const
    {
        // The prototype only writes hitinfo when it found a strictly closer
        // hit, so a "true" here means the closest hit so far is inside this
        // instance -- safe to claim its shading.
        if( !proto->Intersect( ray, hitinfo ) ) return false;
        hitinfo.material = &material;
        return true;
    }

    virtual Box3 GetBounds() const { return proto->GetBounds(); }
    virtual Object *ReadString( const char * ) { return NULL; }
    virtual const char *MyNameIs() const { return "Instance"; }

    Object *proto;   // The shared geometry; owned by the reader's symbol table.
};

#endif
