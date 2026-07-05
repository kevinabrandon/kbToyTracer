/***************************************************************************
* main.cpp                                                                 *
*                                                                          *
* Standalone command-line entry point for kbToyTracer. Renders a single    *
* scene description (.sdf) to a PPM image.                                  *
*                                                                          *
*   kbtoytracer <scene.sdf> [output.ppm] [config.cfg]                       *
*                                                                          *
* If output.ppm is omitted it is derived from the scene name. Render        *
* settings (resolution, anti-aliasing, depth of field, shading toggles)    *
* come from the config file: an explicit third argument, else a            *
* "kbtoytracer.cfg" in the working directory if present, else defaults.    *
*                                                                          *
* History:                                                                 *
*   2026        Refactored to a portable standalone CLI (K. Brandon).      *
*   10/22/2004  Original Win32 version.                                    *
***************************************************************************/
#include <cstdio>
#include <cstring>
#include <string>
#include "toytracer.h"
#include "kbConfig.h"

bool bDisplayMessages = true;   // referenced by toytracer1.cpp / reader1.cpp

static std::string deriveOutput( const char *scene )
{
    std::string s( scene );
    size_t dot = s.find_last_of( '.' );
    if( dot != std::string::npos && s.substr( dot ) == ".sdf" ) s = s.substr( 0, dot );
    return s + ".ppm";
}

int main( int argc, char *argv[] )
{
    if( argc < 2 )
    {
        fprintf( stderr,
            "kbToyTracer - a Monte Carlo ray tracer\n\n"
            "usage: %s <scene.sdf> [output.ppm] [config.cfg]\n\n"
            "  scene.sdf    scene description to render (required)\n"
            "  output.ppm   output image (default: derived from scene name)\n"
            "  config.cfg   render settings (default: ./kbtoytracer.cfg if present)\n",
            argv[0] );
        return 1;
    }

    const char *sceneFile  = argv[1];
    std::string outFile    = ( argc >= 3 ) ? argv[2] : deriveOutput( sceneFile );
    const char *configFile = ( argc >= 4 ) ? argv[3] : "kbtoytracer.cfg";

    if( Config.Load( configFile ) )
        printf( "Loaded render settings from %s\n", configFile );
    else if( argc >= 4 )
        { fprintf( stderr, "Could not open config file %s\n", configFile ); return 1; }
    else
        printf( "No config file (%s); using built-in defaults.\n", configFile );

    printf( "Rendering %s -> %s  (%dx%d)\n\n",
            sceneFile, outFile.c_str(), Config.width, Config.height );

    Scene  scene;
    Camera camera;
    Lens   lens;

    if( !ReadSceneDescription( sceneFile, camera, lens, scene ) )
    {
        fprintf( stderr, "Error: could not read scene '%s'\n", sceneFile );
        return 1;
    }

    if( !MakeImage( camera, lens, scene, Config.width, Config.height, outFile.c_str() ) )
    {
        fprintf( stderr, "Error: rendering failed.\n" );
        return 2;
    }

    printf( "Wrote %s\n", outFile.c_str() );
    return 0;
}
