#!/usr/bin/env python3
"""Section 4 (the finale): the reflective 229k-triangle teapot and the depth-of-field frame."""
from gallery_lib import render, settings_line, write_section, build_index

ENTRIES = [
  ("30-teapot.png", "Reflective Teapot", "teapot-bvh-hq.sdf", {},
   "The Utah teapot at 229,921 triangles - reflective, on a reflective checkerboard floor. "
   "Only tractable because of the BVH acceleration structure."),
  ("31-depth-of-field.png", "Depth of Field + Refraction", "blurry.sdf",
   dict(enable_supersample=0, enable_camera_lens=1, lensSamps=10),
   "A glass sphere (refraction) in a Cornell-style room, shot through a lens focused "
   "mid-scene - the background blurs while the spheres stay sharp."),
]

if __name__ == "__main__":
    cards = []
    for out, title, scene, over, desc in ENTRIES:
        r = render(out, scene, over)
        cards.append(dict(file=out, title=title, desc=desc,
                          meta=settings_line(r["cfg"], r["secs"])))
    write_section(4, "teapot_dof", "Teapot & Depth of Field",
                  "The showpieces: the acceleration structure carrying a quarter-million "
                  "triangles, and the lens model producing depth of field.", cards)
    build_index()
