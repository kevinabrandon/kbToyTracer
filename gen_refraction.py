#!/usr/bin/env python3
"""Section 4: one glass sphere, three ways - refraction and depth of field.

Perfect-focus refracting sphere, the same glass flattened by an affine matrix into
a lens, then the classic depth-of-field shot. Chasing the flattened case is what
exposed the per-light reflection/refraction bug (now fixed in shade1.cpp)."""
from gallery_lib import render, settings_line, write_section, build_index

ENTRIES = [
  ("40-glass-sphere.png", "Refracting Sphere", "refract-focus.sdf",
   dict(enable_camera_lens=0, numBounces=8),
   "A glass sphere in a Cornell-style room, in perfect focus. It acts as a lens: "
   "the room appears inverted and shrunk inside it."),
  ("41-glass-ellipsoid.png", "Flattened into a Lens", "refract-flat.sdf",
   dict(enable_camera_lens=0, numBounces=8),
   "The same glass, flattened by an affine matrix into a thick disc facing the "
   "camera. Refraction through a transformed surface - a good stress test for the shader."),
  ("42-glass-depth-of-field.png", "Depth of Field", "blurry.sdf",
   dict(enable_supersample=0, enable_camera_lens=1, lensSamps=10, numBounces=8),
   "The glass sphere shot through a thin lens focused mid-scene: the background "
   "boxes and walls blur while the spheres stay sharp."),
]

if __name__ == "__main__":
    cards = []
    for out, title, scene, over, desc in ENTRIES:
        r = render(out, scene, over)
        cards.append(dict(file=out, title=title, desc=desc,
                          meta=settings_line(r["cfg"], r["secs"])))
    write_section(4, "refraction", "Glass Sphere & Refraction",
                  "One glass sphere, three ways: a refracting sphere in perfect focus, the "
                  "same glass flattened into a lens, and the classic depth-of-field shot.", cards)
    build_index()
