/***************************************************************************
* reader.cpp                                                               *
*                                                                          *
* This file defines the ReadSceneDescription function, which reads a       *
* simple text description of a scene and the camera.                       *
*                                                                          *
* History:																   *
*	07/09/2005	KAB - Added refraction                                     *
*   10/23/2004  Changed handling of default colors.                        *
*   10/16/2004  Minor updates (e.g. added more error messages).            *
*   10/06/2004  Added 3x4 matrix reader & support for aggregate objects.   *
*   04/04/2003  Initial coding.                                            *
*                                                                          *
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "toytracer.h"
#include "kbStripeTex.h"
#include "kbCheckerTex.h"
#include "kbMarbleTex.h"
#include "kbImageTex.h"
#include "Mat3x4.h"

static char format[256];
static int line_num;
static char *input_line;

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

// AddObject handles the insertion of objects into aggregates by calling "AddChild" of
// the current aggregate.  If the new object is also an aggregate, then subsequent objects
// should be added to it (until it is explicitly ended).  

static bool AddObject( Object* &agg, Object *obj )
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

bool ReadSceneDescription( const char *file_name, Camera &camera, Lens &lens, Scene &scene )
    {
    static char buff[512];
    Object *cur = NULL;  // The "current" object, which was the last object created.
    Object *agg = NULL;  // Current aggregate object, to which all objects are now added.
    Object *top = NULL;  // The top-most aggregate object.

    scene.object = NULL;
    scene.num_lights = 0;
    line_num = 0;

    FILE *fp = fopen( file_name, "r" );
    if( fp == NULL )
        {
			std::cout << "Error: Could not open file " << file_name << std::endl;
        return false;
        }

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
                cur = obj;
                if( !AddObject( agg, obj ) ) return false;
                found_an_object = true;
                break;
                }
            }
        if( found_an_object ) continue;

        // Now look for all the other stuff...  materials, camera, lights, etc.
 
        if( Get( input_line, "eye"        , camera.eye      ) ) continue;
        if( Get( input_line, "lookat"     , camera.lookat   ) ) continue;            
        if( Get( input_line, "up"         , camera.up       ) ) continue;            
        if( Get( input_line, "vpdist"     , camera.vpdist   ) ) continue;     
		if( Get( input_line, "lensRadius" , lens.lensRadius ) ) continue;
		if( Get( input_line, "focalDist"  , lens.focalDist  ) ) continue;
        if( Get( input_line, "amblight"   , scene.ambient   ) ) continue;            
        if( Get( input_line, "bgcolor"    , scene.bgcolor   ) ) continue;

        
        // If no object is defined at this point, it's an error.

        if( cur == NULL )
            {
				std::cerr << "Error reading scene file; No object defined at line "
                 << line_num << ": "
				 << input_line << std::endl;
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
        if( Get( input_line, "emission", cur->material.emission ) && cur->material.Emitter() )
            {
            if( cur->IsAggregate() )
                {
					std::cerr << "Error: An aggregate object cannot be an emitter.  Line "
                     << line_num << ": "
					 << input_line << std::endl;
                return false;
                }
            else if( scene.num_lights >= MaxLights )
                {
					std::cerr << "Error: Too many light sources.  Line "
                     << line_num << ": "
					 << input_line << std::endl;
                return false;
                }
            scene.Light[ scene.num_lights++ ] = cur;
            continue;
            }

        if( Get( input_line, "matrix", matrix ) )
            {
            agg->Transform( matrix );
            continue;
            }
        
       // Look for an end statement that closes the current aggregate.  This allows us to nest aggregates.

        if( Get( input_line, "end" ) )
            { 
            if( agg == NULL )
                {
					std::cerr << "Error: end statement found outside an aggregate object at line "
                     << line_num << ": "
					 << input_line << std::endl;
                return false;
                }

            // If the curent aggregate has no parent, it must be the top-most one.
            
            if( agg->parent == NULL && top == NULL ) top = agg;
            
            // Go back to adding objects to the parent object (if there is one).
            
            agg = agg->parent; 
            continue;
            }
        
        // If nothing matched, it's an error.  Print a warning and continue processing.

			std::cerr << "Warning: Unrecognized command at line "
             << line_num << ": "
			 << input_line << std::endl;
        }

    if( agg != NULL )
        {
			std::cerr << "Error: Top-most aggregate object has no 'end' statement." << std::endl;
        return false;
        }

    scene.object = top; // The only object associated with the scene is the top-most aggregate.
    return true;
    }
