/***************************************************************************
* kbObjImport.cpp                                                          *
*                                                                          *
* See kbObjImport.h.                                                       *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "kbObjImport.h"
#include "kbSmoothTriangle.h"
#include "kbBVH.h"

// Parse one face-vertex token: "v", "v/vt", "v//vn" or "v/vt/vn", with
// obj's 1-based indices (negative = relative to the end of the list so far).
// Returns false if the token has no usable vertex index.

static bool ParseFaceToken( const char *tok, int nv, int nt, int nn,
                            int &vi, int &ti, int &ni )
{
    char *e;
    vi = (int)strtol( tok, &e, 10 );
    if( e == tok ) return false;
    if( vi < 0 ) vi += nv + 1;
    ti = 0;
    ni = 0;
    if( *e == '/' )
    {
        e++;
        if( *e != '/' && *e != '\0' )
        {
            ti = (int)strtol( e, &e, 10 );
            if( ti < 0 ) ti += nt + 1;
        }
        if( *e == '/' )
        {
            e++;
            ni = (int)strtol( e, &e, 10 );
            if( ni < 0 ) ni += nn + 1;
        }
    }
    if( vi < 1 || vi > nv ) return false;
    if( ti < 0 || ti > nt ) ti = 0;
    if( ni < 0 || ni > nn ) ni = 0;
    return true;
}

Object *kbImportObj( const char *path )
{
    FILE *fp = fopen( path, "r" );
    if( fp == NULL )
    {
        std::cerr << "Error: Could not open obj file " << path << std::endl;
        return NULL;
    }

    std::vector<Vec3> vs;   // v  lines
    std::vector<Vec3> vns;  // vn lines
    std::vector<Vec2> vts;  // vt lines
    long triangles = 0, skipped = 0;

    // The model is pure geometry with a neutral default material; instances
    // are expected to repaint it from the scene file.
    BVH *bvh = new BVH();
    bvh->material.diffuse       = Color( 0.8, 0.8, 0.8 );
    bvh->material.specular      = Color( 0.0, 0.0, 0.0 );
    bvh->material.emission      = Color( 0.0, 0.0, 0.0 );
    bvh->material.texture       = NULL;
    bvh->material.reflectivity  = 0.0;
    bvh->material.refractivity  = 0.0;
    bvh->material.index_refract = 1.0;
    bvh->material.Phong_exp     = 0.0;
    bvh->material.type          = 0;

    char line[1024];
    while( fgets( line, sizeof(line), fp ) != NULL )
    {
        double x, y, z;
        if( sscanf( line, "v %lf %lf %lf", &x, &y, &z ) == 3 )
        {
            vs.push_back( Vec3( x, y, z ) );
        }
        else if( sscanf( line, "vn %lf %lf %lf", &x, &y, &z ) == 3 )
        {
            vns.push_back( Vec3( x, y, z ) );
        }
        else if( sscanf( line, "vt %lf %lf", &x, &y ) == 2 )
        {
            vts.push_back( Vec2( x, y ) );
        }
        else if( line[0] == 'f' && ( line[1] == ' ' || line[1] == '\t' ) )
        {
            // Gather the polygon's corners, then fan-triangulate.
            std::vector<int> vi, ti, ni;
            for( char *tok = strtok( line + 1, " \t\r\n" ); tok != NULL;
                       tok = strtok( NULL, " \t\r\n" ) )
            {
                int v, t, n;
                if( ParseFaceToken( tok, (int)vs.size(), (int)vts.size(),
                                    (int)vns.size(), v, t, n ) )
                {
                    vi.push_back( v );
                    ti.push_back( t );
                    ni.push_back( n );
                }
            }
            for( size_t k = 1; k + 1 < vi.size(); k++ )
            {
                const Vec3 &A = vs[ vi[0]   - 1 ];
                const Vec3 &B = vs[ vi[k]   - 1 ];
                const Vec3 &C = vs[ vi[k+1] - 1 ];

                // Skip degenerate (zero-area) triangles; their barycentric
                // matrix is singular.
                Vec3 cross = (A - B) ^ (C - B);
                if( Length( cross ) < 1.0E-12 ) { skipped++; continue; }

                // With vertex normals, shade smooth; without, use the
                // geometric normal at all three corners (flat shading).
                // Obj winding is CCW, which matches this tracer's convention.
                Vec3 NA, NB, NC;
                if( ni[0] > 0 && ni[k] > 0 && ni[k+1] > 0 )
                {
                    NA = vns[ ni[0]   - 1 ];
                    NB = vns[ ni[k]   - 1 ];
                    NC = vns[ ni[k+1] - 1 ];
                }
                else
                {
                    NA = NB = NC = -Unit( cross );
                }

                Vec2 TA, TB, TC;
                if( ti[0] > 0 )   TA = vts[ ti[0]   - 1 ];
                if( ti[k] > 0 )   TB = vts[ ti[k]   - 1 ];
                if( ti[k+1] > 0 ) TC = vts[ ti[k+1] - 1 ];

                kbSmoothTriangle *tri =
                    new kbSmoothTriangle( A, B, C, NA, NB, NC, TA, TB, TC );
                tri->material = bvh->material;
                bvh->AddChild( tri );
                triangles++;
            }
        }
        // Everything else (mtllib, usemtl, o, g, s, comments) is ignored.
    }
    fclose( fp );

    if( triangles == 0 )
    {
        std::cerr << "Error: no triangles found in obj file " << path << std::endl;
        delete bvh;
        return NULL;
    }
    std::cout << "Imported " << triangles << " triangles from " << path;
    if( skipped > 0 ) std::cout << " (" << skipped << " degenerate faces skipped)";
    std::cout << std::endl;
    return bvh;
}
