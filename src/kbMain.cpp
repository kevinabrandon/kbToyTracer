/***************************************************************************
* kbMain.cpp                                                               *
*                                                                          *
* Standalone command-line entry point for kbToyTracer. Renders a single    *
* scene description (.sdf) to a PNG or PPM image.                          *
*                                                                          *
*   kbtoytracer <scene.sdf> [output.png|.ppm] [config.cfg] [options]       *
*                                                                          *
* If the output is omitted it is derived from the scene name (.png).       *
* Render settings (resolution, sample counts, shading toggles) come from   *
* the config file -- an explicit third argument, else "kbtoytracer.cfg"    *
* in the working directory if present, else defaults -- and can be         *
* overridden per run with --set key=value.  --preview renders a fast,      *
* low-quality draft regardless of the config.                              *
*                                                                          *
* History:                                                                 *
*   2026        Refactored to a portable standalone CLI (K. Brandon).      *
*   10/22/2004  Original Win32 version.                                    *
***************************************************************************/
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "toytracer.h"
#include "kbConfig.h"

static std::string deriveOutput( const char *scene )
{
    std::string s( scene );
    size_t dot = s.find_last_of( '.' );
    if( dot != std::string::npos && s.substr( dot ) == ".sdf" ) s = s.substr( 0, dot );
    return s + ".png";
}

static void usage( const char *prog )
{
    fprintf( stderr,
        "kbToyTracer - a Monte Carlo ray tracer\n\n"
        "usage: %s <scene.sdf> [output.png|.ppm] [config.cfg] [options]\n\n"
        "  scene.sdf      scene description to render (required)\n"
        "  output         image to write; .png or .ppm by extension\n"
        "                 (default: derived from the scene name, as .png)\n"
        "  config.cfg     render settings (default: ./kbtoytracer.cfg if present)\n\n"
        "options:\n"
        "  --set key=value   override any config setting (repeatable),\n"
        "                    e.g. --set width=1920 --set aa_samples=4\n"
        "  --preview         fast draft: 256x256, 1 AA sample, hard shadows,\n"
        "                    minimal DoF, 3 bounces (applied before --set)\n"
        "  --help            show this message\n",
        prog );
}

// Apply one "key=value" from --set.
static bool applySet( const char *arg )
{
    const char *eq = strchr( arg, '=' );
    if( eq == NULL || eq == arg ) return false;
    std::string key( arg, eq - arg );
    return Config.Set( key.c_str(), eq + 1 );
}

int main( int argc, char *argv[] )
{
    std::vector<const char*> positional;
    std::vector<const char*> sets;
    bool preview = false;

    for( int i = 1; i < argc; i++ )
    {
        if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 )
            { usage( argv[0] ); return 0; }
        else if( strcmp( argv[i], "--preview" ) == 0 )
            preview = true;
        else if( strcmp( argv[i], "--set" ) == 0 && i + 1 < argc )
            sets.push_back( argv[++i] );
        else if( strncmp( argv[i], "--set=", 6 ) == 0 )
            sets.push_back( argv[i] + 6 );
        else if( argv[i][0] == '-' && argv[i][1] == '-' )
            { fprintf( stderr, "Unknown option '%s'\n\n", argv[i] ); usage( argv[0] ); return 1; }
        else
            positional.push_back( argv[i] );
    }

    if( positional.empty() ) { usage( argv[0] ); return 1; }

    const char *sceneFile  = positional[0];
    std::string outFile    = ( positional.size() >= 2 ) ? positional[1] : deriveOutput( sceneFile );
    const char *configFile = ( positional.size() >= 3 ) ? positional[2] : "kbtoytracer.cfg";

    if( Config.Load( configFile ) )
        printf( "Loaded render settings from %s\n", configFile );
    else if( positional.size() >= 3 )
        { fprintf( stderr, "Could not open config file %s\n", configFile ); return 1; }
    else
        printf( "No config file (%s); using built-in defaults.\n", configFile );

    if( preview )
    {
        Config.width = 256;  Config.height = 256;
        Config.aa_samples = 1;  Config.shadow_samples = 1;
        Config.dof_samples = 2; Config.max_bounces = 3;
        printf( "Preview mode: fast, low-quality draft.\n" );
    }

    for( size_t i = 0; i < sets.size(); i++ )
        if( !applySet( sets[i] ) )
            { fprintf( stderr, "Bad --set '%s' (want a known key=value)\n", sets[i] ); return 1; }

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
