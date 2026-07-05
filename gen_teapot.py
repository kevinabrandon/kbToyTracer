#!/usr/bin/env python3
"""Section 5 (the finale): the reflective 229k-triangle Utah teapot."""
from gallery_lib import render, settings_line, write_section, build_index

ENTRIES = [
  ("50-teapot.png", "Reflective Teapot", "teapot-bvh-hq.sdf", {},
   "The Utah teapot at 229,921 triangles - reflective, on a reflective checkerboard "
   "floor. Only tractable because of the BVH acceleration structure; each ray finds "
   "its triangle in log time instead of scanning all quarter-million."),
]

if __name__ == "__main__":
    cards = []
    for out, title, scene, over, desc in ENTRIES:
        r = render(out, scene, over)
        cards.append(dict(file=out, title=title, desc=desc,
                          meta=settings_line(r["cfg"], r["secs"])))
    write_section(5, "teapot", "Utah Teapot",
                  "The showpiece: a quarter-million triangles carried by the acceleration "
                  "structure, reflective on a reflective floor.", cards)
    build_index()
