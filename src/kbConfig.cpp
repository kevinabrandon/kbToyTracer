/***************************************************************************
* kbConfig.cpp                                                             *
* Parses a simple "key value" config file (one setting per line, '#' for   *
* comments) into the global Config object.                                 *
***************************************************************************/
#include "kbConfig.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

kbConfig Config;   // global instance, initialized with the defaults in kbConfig.h

static bool asBool( const char *v )
{
    return v[0] == '1' || v[0] == 't' || v[0] == 'T' || v[0] == 'y' || v[0] == 'Y';
}

bool kbConfig::Load( const char *path )
{
    FILE *f = fopen( path, "r" );
    if( f == NULL ) return false;

    char line[256], key[128], val[128];
    while( fgets( line, sizeof line, f ) != NULL )
    {
        if( line[0] == '#' || line[0] == '\n' || line[0] == '\r' ) continue;
        if( sscanf( line, "%127s %127s", key, val ) != 2 ) continue;

        #define INT_OPT(name)  else if( !strcmp( key, #name ) ) name = atoi( val );
        #define BOOL_OPT(name) else if( !strcmp( key, #name ) ) name = asBool( val );
        if( 0 ) {}
        INT_OPT(width) INT_OPT(height)
        INT_OPT(numSampsLarge) INT_OPT(numSampsSmall)
        INT_OPT(lensSamps) INT_OPT(numBounces) INT_OPT(dlSamp)
        BOOL_OPT(enable_supersample) BOOL_OPT(enable_sochastic_super)
        BOOL_OPT(enable_adaptive_super) BOOL_OPT(enable_camera_lens)
        BOOL_OPT(enable_shading) BOOL_OPT(enable_specular) BOOL_OPT(enable_shadows)
        BOOL_OPT(enable_reflection) BOOL_OPT(enable_refraction)
        BOOL_OPT(enable_indirect_light) BOOL_OPT(enable_stratify_light)
        BOOL_OPT(enable_area_light) BOOL_OPT(bg_escaped_rays)
        else fprintf( stderr, "kbConfig: unknown setting '%s'\n", key );
        #undef INT_OPT
        #undef BOOL_OPT
    }
    fclose( f );
    return true;
}
