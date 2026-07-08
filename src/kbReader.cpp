/***************************************************************************
* kbReader.cpp                                                             *
*                                                                          *
* This file defines the ReadSceneDescription function, which reads a       *
* simple text description of a scene and the camera.                       *
*                                                                          *
* History:																   *
*   07/2026     KAB - Reentrant SceneReader (groundwork for imports).      *
*	07/09/2005	KAB - Added refraction                                     *
*   10/23/2004  Changed handling of default colors.                        *
*   10/16/2004  Minor updates (e.g. added more error messages).            *
*   10/06/2004  Added 3x4 matrix reader & support for aggregate objects.   *
*   04/04/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <stdlib.h>
#include "toytracer.h"
#include "kbStripeTex.h"
#include "kbCheckerTex.h"
#include "kbMarbleTex.h"
#include "kbImageTex.h"
#include "kbInstance.h"
#include "kbObjImport.h"
#include "Mat3x4.h"

static char format[256];

Object *RegisteredObjects = NULL;  // A list of objects recognized by the toytracer.

bool RegisterObject( Object *obj ) // Adds each object that is registered.
    {
    obj->next = RegisteredObjects;
    RegisteredObjects = obj;
    return true;
    }

// The following "Get" functions attempt to read information in a specific
// format from a line that is read from a file.  They return "true" if and
// only if all fields have been read successfully.

bool Get( const char *line, const char *name )
    {
    // Read the given string.
    char buff[64];
    sscanf( line, "%s", buff );
    return strcmp( buff, name ) == 0;
    }

bool Get( const char *line, const char *name, const char *arg )
    {
    // Read the given string.
    char buff1[64], buff2[64];
    sscanf( line, "%s %s", buff1, buff2 );
    return ( strcmp( buff1, name ) == 0 ) && ( strcmp( buff2, arg ) == 0 );
    }

bool Get( const char *line, const char *name, Vec3 &coord )
    {
    // Read the given string followed by 3D coordinates in parens.
    sprintf( format, "%s (%%lf,%%lf,%%lf)", name );
    return sscanf( line, format, &coord.x, &coord.y, &coord.z ) == 3;
    }

bool Get( const char *line, const char *name, Color &color )
    {
    // Read the given string followed by an RGB triple in square brackets.
    sprintf( format, "%s [%%lf,%%lf,%%lf]", name );
    return sscanf( line, format, &color.red, &color.green, &color.blue ) == 3;
    }
bool Get( const char *line, const char *name, char *string )
{
	sprintf( format, "%s %%s", name );
	return sscanf( line, format, string ) == 1;
}
bool Get( const char *line, const char *name, Color &color1, Color &color2, float &value )
    {
    // Read the given string followed by an RGB triple in square brackets.
    sprintf( format, "%s [%%lf,%%lf,%%lf] [%%lf,%%lf,%%lf] %%f", name );
	return sscanf( line, format, &color1.red, &color1.green, &color1.blue, &color2.red, &color2.green, &color2.blue, &value ) == 7;
    }
bool Get( const char *line, const char *name, Color &c1, Color &c2, Color &c3, float &s1, float &s2, float &s3 )
{
	sprintf( format, "%s [%%lf,%%lf,%%lf] [%%lf,%%lf,%%lf] [%%lf,%%lf,%%lf] %%f %%f %%f", name );
	return sscanf( line, format, &c1.red, &c1.green, &c1.blue, &c2.red, &c2.green, &c2.blue,
		&c3.red, &c3.green, &c3.blue, &s1, &s2, &s3 ) == 12;
}
bool Get( const char *line, const char *name, float &value )
    {
    // Read the given string followed by a single real value.
    sprintf( format, "%s %%f", name );
    return sscanf( line, format, &value ) == 1;
    }

bool Get( const char *line, const char *name, Mat3x4 &M )
    {
    // Read the given string followed by a 3x4 matrix, in row order, enclosed in parens.
    sprintf( format, "%s (%%lf,%%lf,%%lf,%%lf; %%lf,%%lf,%%lf,%%lf; %%lf,%%lf,%%lf,%%lf)", name );
    return sscanf( line, format,
        &M.mat(0,0), &M.mat(0,1), &M.mat(0,2), &M.vec.x,
        &M.mat(1,0), &M.mat(1,1), &M.mat(1,2), &M.vec.y,
        &M.mat(2,0), &M.mat(2,1), &M.mat(2,2), &M.vec.z
        ) == 12;
    }

// Read the given string followed by 3D coords in parens and a real value
// (used by "rotate (x,y,z) degrees").  Local to the reader.

static bool GetVecFloat( const char *line, const char *name, Vec3 &v, float &value )
    {
    sprintf( format, "%s (%%lf,%%lf,%%lf) %%f", name );
    return sscanf( line, format, &v.x, &v.y, &v.z, &value ) == 4;
    }

// Builders for the convenience transform lines.  "Translate" already lives
// in Mat3x4.h; scale and axis-angle rotation are reader-local.

static Mat3x4 kbScale( double sx, double sy, double sz )
    {
    Mat3x3 S;
    S(0,0) = sx;
    S(1,1) = sy;
    S(2,2) = sz;
    return Mat3x4( S );
    }

static Mat3x4 kbRotate( const Vec3 &axis, double degrees )
    {
    // Rodrigues' rotation formula about an arbitrary (unnormalized) axis.
    Vec3 u = Unit( axis );
    double a  = degrees * Pi / 180.0;
    double cs = cos( a ), sn = sin( a ), k = 1.0 - cs;
    Mat3x3 R;
    R(0,0) = cs + u.x*u.x*k;      R(0,1) = u.x*u.y*k - u.z*sn;  R(0,2) = u.x*u.z*k + u.y*sn;
    R(1,0) = u.y*u.x*k + u.z*sn;  R(1,1) = cs + u.y*u.y*k;      R(1,2) = u.y*u.z*k - u.x*sn;
    R(2,0) = u.z*u.x*k - u.y*sn;  R(2,1) = u.z*u.y*k + u.x*sn;  R(2,2) = cs + u.z*u.z*k;
    return Mat3x4( R );
    }

// Resolve an imported path relative to the directory of the importing file.

static std::string ResolveRelative( const std::string &importer, const std::string &target )
    {
    if( !target.empty() && target[0] == '/' ) return target;
    size_t slash = importer.rfind( '/' );
    if( slash == std::string::npos ) return target;
    return importer.substr( 0, slash + 1 ) + target;
    }

static bool EndsWith( const std::string &s, const char *suffix )
    {
    size_t n = strlen( suffix );
    return s.size() >= n && s.compare( s.size() - n, n, suffix ) == 0;
    }

// Determine whether a given line is to be regarded as blank.  This is
// where comments are handled; that is, lines beginning with "#".
// All other lines have leading blanks and tabs stripped off.

static void SkipBlanks( char* &line )
    {
    for(;;)
        {
        if( *line == '\n' ) { line = NULL; break; } // An empty line.
        if( *line == '#'  ) { line = NULL; break; } // This is a comment.
        if( *line != ' ' && *line != '\t' ) return; // Found a non-white char.
        line++;
        }
    }

// The SceneReader holds everything that used to be file-scope parser state,
// so that reading a scene file is reentrant: an "import" statement can
// recursively read another file mid-parse without clobbering the state of
// the file that imported it.

struct SceneReader
{
    Camera &camera;
    Lens   &lens;
    Scene  &scene;

    // Named prototypes, from "define <name>" blocks and "import ... as <name>".
    std::map<std::string, Object*> defs;

    // Include-once cache (resolved path -> that file's top aggregate) and the
    // stack of imports currently being parsed, for cycle detection.
    std::map<std::string, Object*> imported_tops;
    std::vector<std::string>       import_stack;

    SceneReader( Camera &c, Lens &l, Scene &s ) : camera(c), lens(l), scene(s) {}

    // Parse one file.  In import mode camera/lens/scene-wide lines are
    // ignored and emitters are not registered as scene lights.  On success
    // *top_out receives the file's top-most aggregate (the whole model).
    bool ReadFile( const std::string &path, bool import_mode, Object **top_out );

    // Handle one "import <path> [as <name>]" statement found in `importer`.
    bool ImportFile( const std::string &importer, const std::string &target,
                     const char *alias, int line_num );
};

// AddObject handles the insertion of objects into aggregates by calling "AddChild" of
// the current aggregate.  If the new object is also an aggregate, then subsequent objects
// should be added to it (until it is explicitly ended).

static bool AddObject( Object* &agg, Object *obj, int line_num, const char *input_line )
    {
    obj->parent = agg;
    if( agg == NULL )
        {
        if( obj->IsAggregate() ) agg = obj; // Make obj the current aggregate.
        else
            {
				std::cerr << "Error: First object must be an aggregate, line "
                 << line_num << ": "
				 << input_line << std::endl;
            return false;
            }
        }
    else
        {
        // Add obj to the current aggregate.  Once added, if obj is itself
        // an aggregate object, then make it the current aggregate.
        agg->AddChild( obj );
        obj->material = agg->material; // Start with the accumulated color of current aggregate.
        if( obj->IsAggregate() ) agg = obj;
        }
    return true;
    }

// This is a very minimal scene description reader.  It assumes that
// each line contains a complete entity: an object definition, or
// a camera parameter, or a material parameter, etc.  (Blank lines, and
// lines that begin with "#" are also allowed.)  It creates object instances,
// links them together, and fills in the fields of the scene and camera as it
// reads successive lines of the file.  Lines may be padded with blanks or
// tabs to increase readability.

bool SceneReader::ReadFile( const std::string &path, bool import_mode, Object **top_out )
    {
    char buff[512];
    char *input_line;
    int line_num = 0;
    Object *cur = NULL;  // The "current" object, which was the last object created.
    Object *agg = NULL;  // Current aggregate object, to which all objects are now added.
    Object *top = NULL;  // The top-most aggregate object.

    std::string pending_define;      // Name from a "define" line awaiting its aggregate.
    Object *define_root = NULL;      // The aggregate being defined (NULL when not in a define).
    Mat3x4  xform;                   // Accumulated transform for the current target node:
                                     // successive matrix/translate/rotate/scale lines compose.
    bool warned_scene_lines = false; // One warning per imported file about ignored camera lines.

    // Scene files may be gzipped (e.g. big triangle-soup models); read those
    // through gzip -dc.  Check readability first so the error is ours, not gzip's.
    FILE *probe = fopen( path.c_str(), "r" );
    if( probe == NULL )
        {
			std::cout << "Error: Could not open file " << path << std::endl;
        return false;
        }
    fclose( probe );

    bool piped = EndsWith( path, ".gz" );
    FILE *fp = piped ? popen( ( "gzip -dc '" + path + "'" ).c_str(), "r" )
                     : fopen( path.c_str(), "r" );
    if( fp == NULL )
        {
			std::cout << "Error: Could not open file " << path << std::endl;
        return false;
        }
    #define KB_CLOSE_INPUT() ( piped ? pclose( fp ) : fclose( fp ) )

    // Begin looping over all the lines of the file.
    // Keep processing lines until the end of file is reached.
    // Print a warning for all lines that are unrecognizable.

    while( (input_line = fgets( buff, 512, fp )) != NULL )
        {
        Color c;
		float s;
		Color c1, c2, c3;
		float s1, s2, s3;
		char str[ 256 ];

        Mat3x4 matrix;
        line_num++;
        SkipBlanks( input_line );
        if( input_line == NULL ) continue;

        // Ask each registered object if it recognizes the line.  If it does, it will
        // create a new instance of the object and return it as the function value.

        bool found_an_object = false;
        for( Object *type = RegisteredObjects; type != NULL; type = type->next )
            {
            Object *obj = type->ReadString( input_line );
            if( obj != NULL )
                {
                bool at_top = ( agg == NULL );
                cur = obj;
                if( !AddObject( agg, obj, line_num, input_line ) ) { KB_CLOSE_INPUT(); return false; }
                // A top-level aggregate opened right after a "define" line is
                // the body of that define, not part of the scene.
                if( at_top && !pending_define.empty() ) define_root = obj;
                xform = Mat3x4();  // transform lines now target this new object
                found_an_object = true;
                break;
                }
            }
        if( found_an_object ) continue;

        // Now look for all the other stuff...  materials, camera, lights, etc.
        // An imported file is only a source of geometry: its camera, lens and
        // scene-wide settings belong to whatever scene it was authored in, so
        // they are skipped here (with a single warning per file).

        if( import_mode )
            {
            Vec3 dv; float df; Color dc;
            if( Get( input_line, "eye", dv ) || Get( input_line, "lookat", dv )
             || Get( input_line, "up", dv )  || Get( input_line, "vpdist", df )
             || Get( input_line, "lensRadius", df ) || Get( input_line, "focalDist", df )
             || Get( input_line, "amblight", dc )   || Get( input_line, "bgcolor", dc ) )
                {
                if( !warned_scene_lines )
                    std::cerr << "Warning: ignoring camera/scene settings in imported file "
                              << path << std::endl;
                warned_scene_lines = true;
                continue;
                }
            }
        else
            {
            if( Get( input_line, "eye"        , camera.eye      ) ) continue;
            if( Get( input_line, "lookat"     , camera.lookat   ) ) continue;
            if( Get( input_line, "up"         , camera.up       ) ) continue;
            if( Get( input_line, "vpdist"     , camera.vpdist   ) ) continue;
            if( Get( input_line, "lensRadius" , lens.lensRadius ) ) continue;
            if( Get( input_line, "focalDist"  , lens.focalDist  ) ) continue;
            if( Get( input_line, "amblight"   , scene.ambient   ) ) continue;
            if( Get( input_line, "bgcolor"    , scene.bgcolor   ) ) continue;
            }

        // "define <name>": the next top-level aggregate becomes a named
        // prototype (stored in the symbol table, NOT added to the scene).

        if( Get( input_line, "define", str ) )
            {
            if( agg != NULL || !pending_define.empty() )
                {
                std::cerr << "Error: define must appear at top level, outside any aggregate.  Line "
                          << line_num << ": " << input_line << std::endl;
                KB_CLOSE_INPUT(); return false;
                }
            if( defs.count( str ) )
                {
                std::cerr << "Error: '" << str << "' is already defined.  Line "
                          << line_num << ": " << input_line << std::endl;
                KB_CLOSE_INPUT(); return false;
                }
            pending_define = str;
            continue;
            }

        // "instance <name>": place a shared prototype in the scene.  Follow
        // with transform lines and/or material lines; an untouched instance
        // keeps its prototype's material.

        if( Get( input_line, "instance", str ) )
            {
            std::map<std::string, Object*>::iterator it = defs.find( str );
            if( it == defs.end() )
                {
                std::cerr << "Error: instance of unknown object '" << str
                          << "' (define or import it first).  Line "
                          << line_num << ": " << input_line << std::endl;
                KB_CLOSE_INPUT(); return false;
                }
            if( agg == NULL )
                {
                std::cerr << "Error: instance must appear inside an aggregate.  Line "
                          << line_num << ": " << input_line << std::endl;
                KB_CLOSE_INPUT(); return false;
                }
            kbInstance *inst = new kbInstance( it->second );
            if( !AddObject( agg, inst, line_num, input_line ) ) { KB_CLOSE_INPUT(); return false; }
            inst->material = inst->proto->material; // AddObject stamped the aggregate's
                                                    // material; instances instead default
                                                    // to their prototype's look.
            cur = inst;
            xform = Mat3x4();
            continue;
            }

        // "import <path> [as <name>]": read another sdf (optionally .gz) or
        // obj file.  With "as", its top aggregate becomes a named prototype;
        // either way any defines it contains are merged in.

            {
            char p1[256], p2[256];
            int nread = sscanf( input_line, "import %255s as %255s", p1, p2 );
            if( nread >= 1 )
                {
                if( !ImportFile( path, p1, nread == 2 ? p2 : NULL, line_num ) )
                    { KB_CLOSE_INPUT(); return false; }
                continue;
                }
            }

        // If no object is defined at this point, it's an error.

        if( cur == NULL )
            {
				std::cerr << "Error reading scene file; No object defined at line "
                 << line_num << ": "
				 << input_line << std::endl;
            KB_CLOSE_INPUT();
            return false;
            }

        if( Get( input_line, "diffuse"      , c ) ) { cur->material.diffuse       = agg->material.diffuse       = c; continue; }
        if( Get( input_line, "specular"     , c ) ) { cur->material.specular      = agg->material.specular      = c; continue; }
        if( Get( input_line, "reflectivity" , s ) ) { cur->material.reflectivity  = agg->material.reflectivity  = s; continue; }
        if( Get( input_line, "Phong_exp"    , s ) ) { cur->material.Phong_exp     = agg->material.Phong_exp     = s; continue; }
		if( Get( input_line, "refractivity" , s ) ) { cur->material.refractivity  = agg->material.reflectivity  = s; continue; }
		if( Get( input_line, "index_refract", s ) ) { cur->material.index_refract = agg->material.index_refract = s; continue; }
		if( Get( input_line, "stripeTex", c1, c2, s ) )
		{
			cur->material.texture = agg->material.texture = new kbStripeTex( c1, c2, s );
			continue;
		}
		if( Get( input_line, "imageTex", str ) )
		{
			cur->material.texture = agg->material.texture = new kbImageTex( str );
			continue;
		}
		if( Get( input_line, "noise" ) )
		{
			cur->material.texture = agg->material.texture = new kbMarbleTexture( 0.3f, 5.0f, 8) ;
			continue;
		}
		if( Get( input_line, "marble", c1, c2, c3, s1, s2, s3 ) )
		{
			cur->material.texture = agg->material.texture = new kbMarbleTexture( c1, c2, c3, s1, s2, ( int ) s3 );
			continue;
		}
		if( Get( input_line, "checkerTex", c1, c2, s ) )
		{
			cur->material.texture = agg->material.texture = new kbCheckerTex( c1, c2, s );
			continue;
		}
        if( Get( input_line, "emission", c ) && c != 0.0 )
            {
            // Lights are registered with the scene at parse time, so they
            // cannot live inside a define (each instance would need its own
            // transformed copy) and imported files may not add lights.
            if( define_root != NULL )
                {
					std::cerr << "Error: Emitters are not allowed inside a define.  Line "
                     << line_num << ": "
					 << input_line << std::endl;
                KB_CLOSE_INPUT();
                return false;
                }
            if( import_mode )
                {
					std::cerr << "Warning: ignoring emitter in imported file " << path
                     << ", line " << line_num << " (imports cannot add lights)." << std::endl;
                continue;
                }
            if( cur->IsAggregate() )
                {
					std::cerr << "Error: An aggregate object cannot be an emitter.  Line "
                     << line_num << ": "
					 << input_line << std::endl;
                KB_CLOSE_INPUT();
                return false;
                }
            else if( scene.num_lights >= MaxLights )
                {
					std::cerr << "Error: Too many light sources.  Line "
                     << line_num << ": "
					 << input_line << std::endl;
                KB_CLOSE_INPUT();
                return false;
                }
            cur->material.emission = c;
            scene.Light[ scene.num_lights++ ] = cur;
            continue;
            }

        // Transform lines.  Each applies to the most recently added object
        // (or, right after an "end", to the aggregate just closed), and
        // successive lines compose in reading order: written "scale" then
        // "translate", the object is scaled first, then moved.

            {
            bool  got_xform = false;
            Mat3x4 X;
            Vec3  v;
            float ang;
            if(      Get( input_line, "matrix", matrix ) )              { X = matrix;                      got_xform = true; }
            else if( Get( input_line, "translate", v ) )                { X = Translate( v.x, v.y, v.z );  got_xform = true; }
            else if( GetVecFloat( input_line, "rotate", v, ang ) )      { X = kbRotate( v, ang );          got_xform = true; }
            else if( Get( input_line, "scale", v ) )                    { X = kbScale( v.x, v.y, v.z );    got_xform = true; }
            else if( Get( input_line, "scale", s ) )                    { X = kbScale( s, s, s );          got_xform = true; }
            if( got_xform )
                {
                if( agg == NULL )
                    {
                    std::cerr << "Error: transform outside an aggregate.  Line "
                              << line_num << ": " << input_line << std::endl;
                    KB_CLOSE_INPUT();
                    return false;
                    }
                xform = X * xform;
                agg->Transform( xform );
                continue;
                }
            }

       // Look for an end statement that closes the current aggregate.  This allows us to nest aggregates.

        if( Get( input_line, "end" ) )
            {
            if( agg == NULL )
                {
					std::cerr << "Error: end statement found outside an aggregate object at line "
                     << line_num << ": "
					 << input_line << std::endl;
                KB_CLOSE_INPUT();
                return false;
                }

            // If the curent aggregate has no parent, it must be the top-most
            // one -- either the scene's top aggregate, or the completed body
            // of a define.

            if( agg->parent == NULL )
                {
                if( agg == define_root )
                    {
                    defs[ pending_define ] = agg;
                    pending_define.clear();
                    define_root = NULL;
                    }
                else if( top == NULL ) top = agg;
                }

            // Go back to adding objects to the parent object (if there is one).

            agg = agg->parent;
            xform = Mat3x4();  // transform lines now target the node holding the closed aggregate
            continue;
            }

        // If nothing matched, it's an error.  Print a warning and continue processing.

			std::cerr << "Warning: Unrecognized command at line "
             << line_num << ": "
			 << input_line << std::endl;
        }

    KB_CLOSE_INPUT();
    #undef KB_CLOSE_INPUT

    if( agg != NULL )
        {
			std::cerr << "Error: Top-most aggregate object has no 'end' statement." << std::endl;
        return false;
        }
    if( !pending_define.empty() )
        {
			std::cerr << "Error: define '" << pending_define
             << "' was never completed (missing aggregate or its 'end')." << std::endl;
        return false;
        }

    if( top_out != NULL ) *top_out = top;
    return true;
    }

bool SceneReader::ImportFile( const std::string &importer, const std::string &target,
                              const char *alias, int line_num )
    {
    std::string res = ResolveRelative( importer, target );

    // An obj file is pure geometry; it becomes a prototype directly.
    if( EndsWith( res, ".obj" ) )
        {
        if( alias == NULL )
            {
            std::cerr << "Error: obj imports need a name: import " << target
                      << " as <name>.  Line " << line_num << std::endl;
            return false;
            }
        if( defs.count( alias ) )
            {
            std::cerr << "Error: '" << alias << "' is already defined.  Line "
                      << line_num << std::endl;
            return false;
            }
        Object *model = kbImportObj( res.c_str() );
        if( model == NULL ) return false;
        defs[ alias ] = model;
        return true;
        }

    // Guard against import cycles (a imports b imports a ...).
    for( size_t i = 0; i < import_stack.size(); i++ )
        if( import_stack[i] == res )
            {
            std::cerr << "Error: import cycle detected at " << res << std::endl;
            return false;
            }

    // Include-once: a file already imported is not parsed again; its cached
    // top aggregate just gets another name (its defines are already merged).
    Object *mtop = NULL;
    std::map<std::string, Object*>::iterator hit = imported_tops.find( res );
    if( hit != imported_tops.end() )
        {
        mtop = hit->second;
        }
    else
        {
        import_stack.push_back( res );
        bool ok = ReadFile( res, true, &mtop );
        import_stack.pop_back();
        if( !ok ) return false;
        imported_tops[ res ] = mtop;
        }

    if( alias != NULL )
        {
        if( mtop == NULL )
            {
            std::cerr << "Error: imported file " << res
                      << " has no top-level aggregate to name.  Line " << line_num << std::endl;
            return false;
            }
        if( defs.count( alias ) )
            {
            std::cerr << "Error: '" << alias << "' is already defined.  Line "
                      << line_num << std::endl;
            return false;
            }
        defs[ alias ] = mtop;
        }
    return true;
    }

bool ReadSceneDescription( const char *file_name, Camera &camera, Lens &lens, Scene &scene )
    {
    scene.object = NULL;
    scene.num_lights = 0;

    SceneReader reader( camera, lens, scene );
    Object *top = NULL;
    if( !reader.ReadFile( file_name, false, &top ) ) return false;

    scene.object = top; // The only object associated with the scene is the top-most aggregate.
    return true;
    }
