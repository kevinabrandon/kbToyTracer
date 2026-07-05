#!/usr/bin/env python3
"""Section 2: 200 spheres, then affine ellipsoids, then reflective ellipsoids."""
from gallery_lib import render, settings_line, write_section, build_index

ENTRIES = [
  ("10-spheres.png", "Spheres", "spheres.sdf",
   dict(enable_reflection=0, numBounces=1),
   "200 diffuse spheres with soft self-shadowing - the starting point."),
  ("11-ellipsoids-plain.png", "Affine Ellipsoids", "ellipsoids-plain.sdf",
   dict(enable_reflection=0, numBounces=1),
   "The same 200 spheres, each squashed into an ellipsoid by its own affine matrix. No reflection."),
  ("12-ellipsoids-reflective.png", "Reflective Ellipsoids", "ellipsoids.sdf", {},
   "The same 200 ellipsoids, now reflective - each mirrors its neighbors."),
]

if __name__ == "__main__":
    cards = []
    for out, title, scene, over, desc in ENTRIES:
        r = render(out, scene, over)
        cards.append(dict(file=out, title=title, desc=desc,
                          meta=settings_line(r["cfg"], r["secs"])))
    write_section(2, "spheres_ellipsoids", "Spheres & Ellipsoids",
                  "Building up one feature at a time on a field of 200 objects: plain spheres, "
                  "then the same objects transformed into ellipsoids, then made reflective.", cards)
    build_index()
