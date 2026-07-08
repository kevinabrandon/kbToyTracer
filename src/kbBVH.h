//
//	kbBVH.h
//
//	Defines a Bounding Volume Hierarchy (Shirley & Morley, "Realistic Ray Tracing").
//
//	A BVH is a binary tree of axis-aligned bounding boxes. Each interior node's
//	box encloses its two children; leaves hold a few primitives. A ray that
//	misses a node's box skips the entire subtree, turning brute-force O(n) into
//	roughly O(log n) per ray.
//
//	Children are collected (with any affine transforms) into an ordinary List as
//	they are read from the scene file; the tree is built lazily on the first ray.
//

#ifndef KB_BVH_H
#define KB_BVH_H

#include "toytracer.h"
#include "kbList.h"
#include <mutex>

struct BVHNode;   // a node of the BVH tree (defined in BVH.cpp)

class BVH : public Aggregate
{
public:
    BVH( );
   ~BVH( );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds( ) const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs( ) const { return "BVH"; }
    virtual void AddChild( Object * );
    virtual void Transform( const Mat3x4 & );

    List list;                 // collects the children (and their transforms)

private:
    mutable BVHNode        *root;       // the tree (BVHNode defined in BVH.cpp), built lazily
    mutable bool            built;
    mutable std::once_flag  buildOnce;  // ensures the lazy build happens exactly once, even under threads
    void Build() const;        // construct the tree from "list"
};

REGISTER_OBJECT( BVH );

#endif // KB_BVH_H
