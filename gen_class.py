#!/usr/bin/env python3
"""Section 1: the two class scenes (reflection, affine transform)."""
from gallery_lib import render, settings_line, write_section, build_index

ENTRIES = [
  ("01-reflection.png", "Reflection", "scene1-reflection.sdf", {},
   "The scene shown in class: a reflective sphere and two blocks."),
  ("02-matrix.png", "Affine Transformation", "scene2-matrix.sdf", {},
   "The same scene, the sphere squashed into an ellipsoid by a 3x4 affine matrix."),
]

if __name__ == "__main__":
    cards = []
    for out, title, scene, over, desc in ENTRIES:
        r = render(out, scene, over)
        cards.append(dict(file=out, title=title, desc=desc,
                          meta=settings_line(r["cfg"], r["secs"])))
    write_section(1, "class", "Class Scenes",
                  "The two milestones from the course: implementing reflection, then affine "
                  "transformations, on the simple scene shown in class.", cards)
    build_index()
