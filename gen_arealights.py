#!/usr/bin/env python3
"""Section 3: area light sources - the SphereLight scene at increasing sample counts.

A point light gives hard shadows; an area light gives soft shadows, approximated by
Monte-Carlo sampling the light. More samples -> less noise. This sweeps dlSamp = 1..7,
i.e. 1, 4, 9, 16, 25, 36, 49 shadow samples per point.
"""
from gallery_lib import render, settings_line, write_section, build_index

SAMPLE_GRID = [1, 2, 3, 4, 5, 6, 7]   # dlSamp; total samples = n*n

CFG = dict(enable_supersample=0, enable_camera_lens=0, enable_area_light=1,
           enable_reflection=0, enable_refraction=0, numBounces=2)

if __name__ == "__main__":
    cards = []
    for n in SAMPLE_GRID:
        total = n * n
        out = f"20-spherelight-{total:02d}.png"
        r = render(out, "spherelight.sdf", dict(CFG, dlSamp=n))
        cards.append(dict(
            file=out,
            title=f"Area Light - {total} sample{'s' if total != 1 else ''}",
            desc=(f"A spherical light softly illuminating boxes and a ball, sampled with "
                  f"{n}x{n} = {total} Monte-Carlo shadow samples per point. "
                  + ("Very noisy with a single sample." if total == 1 else
                     "Smooth soft shadows." if total >= 25 else
                     "Softening as samples increase.")),
            meta=settings_line(r["cfg"], r["secs"])))
    write_section(3, "arealights", "Area Light Sources",
                  "Soft shadows from a spherical light, approximated by Monte-Carlo sampling. "
                  "Watch the shadow noise vanish as the sample count climbs from 1 to 49.", cards)
    build_index()
