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
#include <map>
#include <string>
#include "kbObjImport.h"
#include "kbSmoothTriangle.h"
#include "kbBVH.h"

static bool ObjEndsWith( const std::string &s, const char *suffix )
{
    size_t n = strlen( suffix );
    return s.size() >= n && s.compare( s.size() - n, n, suffix ) == 0;
}

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

// Minimal .mtl reader: Kd (diffuse), Ks (specular), Ns (Phong exponent) and
// Ke (emission -- the surface will render as a glowing emitter, but it can
// NOT light the scene: lights are registered at scene-parse time, so add a
// point light in the scene instead).  Everything else is ignored.

static void LoadMtl( const std::string &path,
                     std::map<std::string, Material> &mats,
                     const Material &base )
{
    FILE *fp = fopen( path.c_str(), "r" );
    if( fp == NULL )
    {
        std::cerr << "Warning: could not open material library " << path
                  << " (faces keep the default material)" << std::endl;
        return;
    }
    char line[1024], name[256];
    Material *cur = NULL;
    double r, g, b;
    while( fgets( line, sizeof(line), fp ) != NULL )
    {
        if( sscanf( line, " newmtl %255s", name ) == 1 )
        {
            mats[ name ] = base;
            cur = &mats[ name ];
        }
        else if( cur == NULL ) continue;
        else if( sscanf( line, " Kd %lf %lf %lf", &r, &g, &b ) == 3 ) cur->diffuse  = Color( r, g, b );
        else if( sscanf( line, " Ks %lf %lf %lf", &r, &g, &b ) == 3 ) cur->specular = Color( r, g, b );
        else if( sscanf( line, " Ke %lf %lf %lf", &r, &g, &b ) == 3 ) cur->emission = Color( r, g, b );
        else if( sscanf( line, " Ns %lf", &r ) == 1 )                 cur->Phong_exp = (float)r;
    }
    fclose( fp );
}

Object *kbImportObj( const char *path_c )
{
    std::string path( path_c );

    // Make sure the file exists before handing it to gzip, so the error is ours.
    FILE *probe = fopen( path_c, "r" );
    if( probe == NULL )
    {
        std::cerr << "Error: Could not open obj file " << path << std::endl;
        return NULL;
    }
    fclose( probe );

    bool piped = ObjEndsWith( path, ".gz" );
    FILE *fp = piped ? popen( ( "gzip -dc '" + path + "'" ).c_str(), "r" )
                     : fopen( path_c, "r" );
    if( fp == NULL )
    {
        std::cerr << "Error: Could not open obj file " << path << std::endl;
        return NULL;
    }

    std::vector<Vec3> vs;   // v  lines
    std::vector<Vec3> vns;  // vn lines
    std::vector<Vec2> vts;  // vt lines
    long triangles = 0, skipped = 0;

    // The model gets a neutral default material; an .mtl library (if the file
    // names one) refines it per face.  An instance without material overrides
    // inherits these per-face materials; one WITH overrides repaints uniformly.
    BVH *bvh = new BVH();
    bvh->material.diffuse = Color( 0.8, 0.8, 0.8 );

    std::map<std::string, Material> mats;   // from mtllib
    Material curMat = bvh->material;        // material for faces being read

    // Directory of the obj file, for resolving a relative mtllib path.
    std::string dir;
    {
        size_t slash = path.rfind( '/' );
        if( slash != std::string::npos ) dir = path.substr( 0, slash + 1 );
    }

    char line[1024], name[256];
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
        else if( sscanf( line, " mtllib %255s", name ) == 1 )
        {
            LoadMtl( dir + name, mats, bvh->material );
        }
        else if( sscanf( line, " usemtl %255s", name ) == 1 )
        {
            std::map<std::string, Material>::iterator it = mats.find( name );
            if( it != mats.end() ) curMat = it->second;
            else                   curMat = bvh->material;
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
                tri->material = curMat;
                bvh->AddChild( tri );
                triangles++;
            }
        }
        // Everything else (o, g, s, comments) is ignored.
    }
    piped ? pclose( fp ) : fclose( fp );

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
