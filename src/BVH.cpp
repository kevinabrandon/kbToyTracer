//
//	BVH.cpp
//
//	Bounding Volume Hierarchy — after Shirley & Morley, "Realistic Ray Tracing".
//
//	Build: recursively bound a set of primitives, split them into two groups by
//	the median centroid along the box's longest axis, and recurse. Query: test
//	the ray against a node's box; on a miss the whole subtree is skipped.
//
//	History:
//	  2004  Scaffold (begin BVH keyword, delegated to a linear List).
//	  2026  Implemented the actual hierarchy (K. Brandon), from Shirley's book.
//

#include "BVH.h"
#include <vector>
#include <algorithm>

// A node of the tree. Interior nodes have left/right children; leaves hold a
// few primitives, kept as List_nodes so per-object transforms/materials survive.
struct BVHNode
{
    Box3                     box;
    BVHNode                 *left  = 0;
    BVHNode                 *right = 0;
    std::vector<List_node*>  items;    // non-empty  =>  leaf
};

static const int LEAF_MAX = 2;         // primitives per leaf

// World-space bounding box of one child (apply its transform if it has one).
static Box3 worldBox( const List_node *n )
{
    Box3 b = n->object->GetBounds();
    if( n->transformed ) b = n->matrix * b;
    return b;
}

// Slab test: does the ray enter box b within [0, tmax]?  (Shirley's slab method.)
static bool hitBox( const Ray &ray, const Box3 &b, double tmax )
{
    double t0 = 0.0, t1 = tmax, inv, tn, tf, s;

    inv = 1.0 / ray.direction.x;
    tn = ( b.X.min - ray.origin.x ) * inv;  tf = ( b.X.max - ray.origin.x ) * inv;
    if( inv < 0.0 ) { s = tn; tn = tf; tf = s; }
    if( tn > t0 ) t0 = tn;   if( tf < t1 ) t1 = tf;   if( t1 < t0 ) return false;

    inv = 1.0 / ray.direction.y;
    tn = ( b.Y.min - ray.origin.y ) * inv;  tf = ( b.Y.max - ray.origin.y ) * inv;
    if( inv < 0.0 ) { s = tn; tn = tf; tf = s; }
    if( tn > t0 ) t0 = tn;   if( tf < t1 ) t1 = tf;   if( t1 < t0 ) return false;

    inv = 1.0 / ray.direction.z;
    tn = ( b.Z.min - ray.origin.z ) * inv;  tf = ( b.Z.max - ray.origin.z ) * inv;
    if( inv < 0.0 ) { s = tn; tn = tf; tf = s; }
    if( tn > t0 ) t0 = tn;   if( tf < t1 ) t1 = tf;   if( t1 < t0 ) return false;

    return true;
}

// Intersect one child, mirroring List's per-node transform handling.
static bool hitChild( List_node *node, const Ray &ray, HitInfo &hitinfo )
{
    if( node->object == hitinfo.ignore ) return false;

    if( !node->transformed )
        return node->object->Intersect( ray, hitinfo );

    // Map the ray into the object's local frame, intersect, then map the hit back.
    Ray newRay;
    newRay.direction = Unit( node->inverse.mat * ray.direction );
    newRay.origin    = node->inverse * ray.origin;

    HitInfo temp;
    temp.distance = Infinity;
    temp.ignore   = NULL;
    if( !node->object->Intersect( newRay, temp ) ) return false;

    Vec3   p = node->matrix * temp.point;
    double d = Length( p - ray.origin );
    if( d >= hitinfo.distance ) return false;

    hitinfo.distance      = d;
    hitinfo.point         = p;
    hitinfo.ray.origin    = ray.origin;
    hitinfo.ray.direction = ray.direction;
    hitinfo.normal        = Unit( Inverse( Transpose( node->matrix.mat ) ) * temp.normal );
    hitinfo.object        = temp.object;
    hitinfo.uv            = temp.uv;
    return true;
}

// A primitive with its world box and centroid precomputed once (so the build
// never re-evaluates GetBounds during partitioning).
struct Prim { List_node *node; Box3 box; Vec3 c; };

static BVHNode *buildNode( std::vector<Prim> &p, int begin, int end )
{
    BVHNode *node = new BVHNode;

    Box3 b = p[begin].box;
    for( int i = begin + 1; i < end; i++ ) b += p[i].box;
    node->box = b;

    int n = end - begin;
    if( n <= LEAF_MAX )
    {
        for( int i = begin; i < end; i++ ) node->items.push_back( p[i].node );
        return node;
    }

    // Split on the median centroid along the box's longest axis.
    double dx = b.X.max - b.X.min, dy = b.Y.max - b.Y.min, dz = b.Z.max - b.Z.min;
    int axis = ( dx >= dy && dx >= dz ) ? 0 : ( dy >= dz ? 1 : 2 );
    int mid  = begin + n / 2;

    std::nth_element( p.begin() + begin, p.begin() + mid, p.begin() + end,
        [axis]( const Prim &a, const Prim &b ) {
            double va = axis == 0 ? a.c.x : ( axis == 1 ? a.c.y : a.c.z );
            double vb = axis == 0 ? b.c.x : ( axis == 1 ? b.c.y : b.c.z );
            return va < vb;
        } );

    node->left  = buildNode( p, begin, mid );
    node->right = buildNode( p, mid, end );
    return node;
}

static bool traverse( const BVHNode *node, const Ray &ray, HitInfo &hitinfo )
{
    if( node == 0 ) return false;
    if( !hitBox( ray, node->box, hitinfo.distance ) ) return false;

    if( !node->items.empty() )   // leaf
    {
        bool hit = false;
        for( size_t i = 0; i < node->items.size(); i++ )
            if( hitChild( node->items[i], ray, hitinfo ) ) hit = true;
        return hit;
    }

    bool hitL = traverse( node->left,  ray, hitinfo );
    bool hitR = traverse( node->right, ray, hitinfo );
    return hitL || hitR;
}

static void freeNode( BVHNode *node )
{
    if( node == 0 ) return;
    freeNode( node->left );
    freeNode( node->right );
    delete node;
}

// ------------------------------------------------------------------ BVH methods

BVH::BVH( )  { root = 0; built = false; }
BVH::~BVH( ) { freeNode( root ); }

void BVH::Build() const
{
    std::vector<Prim> prims;
    for( List_node *nd = list.first; nd != NULL; nd = nd->next )
    {
        if( nd->object == NULL ) continue;
        Prim pr;
        pr.node = nd;
        pr.box  = worldBox( nd );
        pr.c    = pr.box.Center();
        prims.push_back( pr );
    }
    if( !prims.empty() ) root = buildNode( prims, 0, (int) prims.size() );
    built = true;
}

bool BVH::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{
    // Build the tree exactly once, safely even if several threads arrive together.
    std::call_once( buildOnce, [this]{ Build(); } );
    return traverse( root, ray, hitinfo );
}

Box3 BVH::GetBounds( ) const
{
    return list.GetBounds( );
}

Object * BVH::ReadString( const char * params )
{
    extern bool Get( const char *line, const char *name, const char *arg );
    if( Get( params, "begin", "BVH" ) ) return new BVH( );
    return NULL;
}

void BVH::AddChild( Object *obj )
{
    list.AddChild( obj );
    built = false;   // tree is stale until (re)built
}

void BVH::Transform( const Mat3x4 &mat )
{
    list.Transform( mat );
}
