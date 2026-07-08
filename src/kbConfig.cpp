/***************************************************************************
* kbConfig.cpp                                                             *
* Parses "key value" settings -- from a config file (one per line, '#'     *
* for comments) or from --set command-line overrides -- into the global    *
* Config object.                                                           *
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

bool kbConfig::Set( const char *key, const char *val )
{
    #define INT_OPT(name)   else if( !strcmp( key, #name ) ) name = atoi( val );
    #define DBL_OPT(name)   else if( !strcmp( key, #name ) ) name = atof( val );
    #define BOOL_OPT(name)  else if( !strcmp( key, #name ) ) name = asBool( val );

    if( 0 ) {}
    INT_OPT(width) INT_OPT(height)
    INT_OPT(aa_samples) BOOL_OPT(aa_jitter) BOOL_OPT(aa_adaptive) DBL_OPT(aa_threshold)
    INT_OPT(dof_samples) INT_OPT(shadow_samples) INT_OPT(max_bounces)
    INT_OPT(seed)
    BOOL_OPT(enable_shading) BOOL_OPT(enable_specular) BOOL_OPT(enable_shadows)
    BOOL_OPT(enable_transparent_shadows)
    BOOL_OPT(enable_reflection) BOOL_OPT(enable_refraction)
    BOOL_OPT(enable_indirect_light) BOOL_OPT(enable_stratify_light)
    BOOL_OPT(bg_escaped_rays) BOOL_OPT(display_messages)
    else return false;

    #undef INT_OPT
    #undef DBL_OPT
    #undef BOOL_OPT
    return true;
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
        if( !Set( key, val ) )
            fprintf( stderr, "kbConfig: unknown setting '%s'\n", key );
    }
    fclose( f );
    return true;
}
