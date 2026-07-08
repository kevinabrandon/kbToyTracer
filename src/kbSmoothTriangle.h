/***************************************************************************
* kbSmoothTriangle.h  (primitive object)                                   *
*                                                                          *
* A triangle with per-vertex shading normals and uv coordinates, for       *
* meshes imported from obj files (and available in scene files as          *
* "smooth_triangle").  Intersection uses the same dominant-axis            *
* barycentric method as the flat Triangle; the barycentric coordinates     *
* additionally interpolate the vertex normals (Phong shading) and uvs.     *
* Passing the same normal for all three vertices shades flat.              *
*                                                                          *
* History:                                                                 *
*   07/2026  Initial coding (K. Brandon), after Arvo's triangle.C.         *
*                                                                          *
***************************************************************************/
#ifndef KB_SMOOTH_TRIANGLE_H
#define KB_SMOOTH_TRIANGLE_H

#include "toytracer.h"

struct kbSmoothTriangle : public Object
{
    kbSmoothTriangle() {}
    kbSmoothTriangle( const Vec3 &A, const Vec3 &B, const Vec3 &C,
                      const Vec3 &NA, const Vec3 &NB, const Vec3 &NC,
                      const Vec2 &TA = Vec2(), const Vec2 &TB = Vec2(),
                      const Vec2 &TC = Vec2() );
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const { return box; }
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "SmoothTriangle"; }

    Mat3x3 M;          // Inverse of barycentric coord transform.
    Vec3   N;          // Geometric normal of the triangle's plane.
    double d;          // Distance from origin to the plane.
    Box3   box;        // Bounding box.
    int    axis;       // The dominant axis.
    Vec3   A, B, C;    // Vertices.
    Vec3   NA, NB, NC; // Unit shading normals at the vertices.
    Vec2   TA, TB, TC; // Texture coordinates at the vertices.
};

#endif
