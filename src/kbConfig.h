/***************************************************************************
* kbConfig.h                                                               *
*                                                                          *
* Runtime render settings, loaded from a simple key/value config file.     *
* Historically these were compile-time constants scattered through         *
* kbTrace.cpp and kbShade.cpp; they are now gathered here so a render      *
* can be tuned without recompiling.                                        *
*                                                                          *
* (The direct/indirect light sample counts MAX_DL_SAMP / N_IDL_SAMP remain *
*  compile-time in kbShade.cpp because they size fixed arrays.)            *
***************************************************************************/
#ifndef KB_CONFIG_H
#define KB_CONFIG_H

struct kbConfig
{
    // Image resolution.
    int  width  = 320;
    int  height = 240;

    // Anti-aliasing (kbTrace.cpp).
    bool enable_supersample      = true;
    bool enable_stochastic_super = false;   // jittered supersampling
    bool enable_adaptive_super   = false;
    int  numSampsLarge           = 2;       // NxN samples per pixel
    int  numSampsSmall           = 2;

    // Depth of field (thin lens); aperture/focus come from the scene file.
    bool enable_camera_lens     = false;
    int  lensSamps              = 6;        // NxN rays through the aperture

    // Ray tree.
    int  numBounces             = 6;        // max reflection/refraction depth

    // When a secondary (reflection/refraction) ray escapes the scene, return the
    // background color instead of black. The class-era shader did this; the later
    // GI work switched to black. true reproduces the original 2004/5 look.
    bool bg_escaped_rays        = false;

    // Area-light Monte-Carlo sampling: dlSamp x dlSamp shadow samples per light
    // (1,4,9,16,... total). Only used when enable_area_light is on. Max 8 (see MAX_DL_SAMP).
    int  dlSamp                 = 3;

    // Shading toggles (kbShade.cpp).
    bool enable_shading         = true;
    bool enable_specular        = true;
    bool enable_shadows         = true;
    // When on, shadow rays accumulate transmittance through refractive blockers
    // (glass casts a lighter shadow) instead of a hard yes/no occlusion. No caustics.
    bool enable_transparent_shadows = false;
    bool enable_reflection      = true;
    bool enable_refraction      = true;
    bool enable_indirect_light  = false;
    bool enable_stratify_light  = false;
    bool enable_area_light      = false;

    // Console output (render progress etc.).
    bool display_messages       = true;

    // Load overrides from a config file. Returns false if it can't be opened
    // (in which case the defaults above are used).
    bool Load( const char *path );
};

extern kbConfig Config;

#endif
