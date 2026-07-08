/***************************************************************************
* kbConfig.h                                                               *
*                                                                          *
* Runtime render settings, loaded from a simple key/value config file      *
* and/or --set key=value command-line overrides.                           *
*                                                                          *
* Philosophy: the SCENE decides what is in the picture (lights, lens,      *
* materials); the CONFIG decides how well it is rendered (resolution,      *
* sample counts, bounce depth).  Features like depth of field and soft     *
* shadows turn on automatically when the scene contains a lens or an       *
* area light; the sample counts here control their quality (0 disables).  *
***************************************************************************/
#ifndef KB_CONFIG_H
#define KB_CONFIG_H

struct kbConfig
{
    // Image resolution.
    int  width  = 512;
    int  height = 512;

    // Anti-aliasing: aa_samples x aa_samples rays per pixel (1 = single ray).
    int  aa_samples   = 2;
    bool aa_jitter    = false;   // jitter the AA grid (stochastic supersampling)
    bool aa_adaptive  = false;   // corner-sample first, refine only busy pixels
    double aa_threshold = 0.01;  // corner-difference that triggers refinement

    // Depth of field: dof_samples x dof_samples lens rays per AA sample.
    // Active only when the scene defines a lens; 0 disables DoF entirely.
    int  dof_samples  = 3;

    // Soft shadows: shadow_samples x shadow_samples rays per area light.
    // Point lights always use a single shadow ray; 1 = hard shadows.
    int  shadow_samples = 3;

    // Ray tree.
    int  max_bounces  = 6;       // max reflection/refraction depth

    // When a secondary (reflection/refraction) ray escapes the scene, return the
    // background color instead of black. The class-era shader did this; the later
    // GI work switched to black. true reproduces the original 2004/5 look.
    bool bg_escaped_rays = false;

    // Random seed for all Monte-Carlo sampling (jitter, lens, soft shadows).
    // 0 = seed from the system (every render different); any other value gives
    // reproducible renders -- essential for flicker-free animation frames.
    unsigned seed = 0;

    // Feature kill-switches (normally all on; useful for debugging/teaching).
    bool enable_shading         = true;
    bool enable_specular        = true;
    bool enable_shadows         = true;
    // When on, shadow rays accumulate transmittance through refractive blockers
    // (glass casts a lighter shadow) instead of a hard yes/no occlusion. No caustics.
    bool enable_transparent_shadows = false;
    bool enable_reflection      = true;
    bool enable_refraction      = true;
    bool enable_indirect_light  = false;  // experimental GI
    bool enable_stratify_light  = false;  // experimental GI

    // Console output (render progress etc.).
    bool display_messages       = true;

    // Set a single "key value" setting. Returns false for unknown keys.
    bool Set( const char *key, const char *val );

    // Load settings from a config file (one "key value" per line, '#' for
    // comments). Returns false if the file can't be opened (in which case
    // the defaults above are used).
    bool Load( const char *path );
};

extern kbConfig Config;

#endif
