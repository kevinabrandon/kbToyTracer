#!/usr/bin/env python3
"""Section 2: soft shadows - the SphereLight scene at increasing sample counts.

A point light gives hard shadows; an area light gives soft shadows, approximated by
Monte-Carlo sampling the light. More samples -> less noise. This sweeps shadow_samples,
i.e. 1, 4, 9, 16, 25, 36, 49 shadow samples per point.
"""
from gallery_lib import render, settings_line, write_section, build_index

SAMPLE_GRID = [1, 3, 7]   # shadow_samples; total samples = 1, 9, 49

CFG = dict(aa_samples=1, enable_reflection=0, enable_refraction=0, max_bounces=2)

if __name__ == "__main__":
    cards = []
    for n in SAMPLE_GRID:
        total = n * n
        out = f"20-spherelight-{total:02d}.png"
        r = render(out, "spherelight.sdf", dict(CFG, shadow_samples=n))
        cards.append(dict(
            file=out,
            title=f"Soft Shadows - {total} sample{'s' if total != 1 else ''}",
            desc=(f"A spherical light softly illuminating boxes and a ball, sampled with "
                  f"{n}x{n} = {total} Monte-Carlo shadow samples per point. "
                  + ("Very noisy with a single sample." if total == 1 else
                     "Smooth soft shadows." if total >= 25 else
                     "Softening as samples increase.")),
            meta=settings_line(r["cfg"], r["secs"])))
    write_section(2, "soft_shadows", "Soft Shadows",
                  "Soft shadows from a spherical area light, approximated by Monte-Carlo sampling. "
                  "A point light gives a hard-edged shadow; sampling an area light softens it. "
                  "Watch the shadow noise vanish as the sample count climbs from 1 to 49.", cards)
    build_index()
