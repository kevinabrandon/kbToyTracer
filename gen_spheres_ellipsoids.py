#!/usr/bin/env python3
"""Section 3: 200 objects in a colored Cornell-style room, one material idea at a time,
as a single animated WebP (spheres -> affine ellipsoids -> reflective -> refractive glass),
followed by each stage as a full-resolution still.

The room is the blurry-scene walls (red left, green right, yellow floor); the 200 objects are
Arvo's original non-intersecting ellipsoid set. Clear glass finally has colored walls to
refract, so the refractive stage actually reads as glass instead of grey blobs."""
import os
from PIL import Image, ImageDraw, ImageFont
from gallery_lib import render, write_section, build_index, fmt_time, GALLERY

try:
    FONT = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
except Exception:
    FONT = ImageFont.load_default()

# out, scene, config overrides, progression caption. Each frame's caption also gets the
# render time this machine actually took, baked in as the loop plays.
ENTRIES = [
  ("room-spheres.png", "room-spheres.sdf",
   dict(enable_reflection=0, enable_refraction=0, numBounces=1), "1 - 200 spheres"),
  ("room-ellipsoids-plain.png", "room-ellipsoids-plain.sdf",
   dict(enable_reflection=0, enable_refraction=0, numBounces=1), "2 - affine ellipsoids"),
  ("room-ellipsoids-reflect.png", "room-ellipsoids-reflect.sdf",
   dict(enable_reflection=1, enable_refraction=0, numBounces=8), "3 - reflective (0.55)"),
  ("room-ellipsoids-refract-70.png", "room-ellipsoids-refract-70.sdf",
   dict(enable_reflection=0, enable_refraction=1, numBounces=8), "4 - refractive (0.70)"),
]

def caption(im, text, size=640):
    im = im.convert("RGB").resize((size, size))
    d = ImageDraw.Draw(im, "RGBA")
    d.rectangle([0, im.height - 40, im.width, im.height], fill=(0, 0, 0, 160))
    d.text((14, im.height - 32), text, font=FONT, fill=(240, 240, 240))
    return im

if __name__ == "__main__":
    prog, total = [], 0.0
    for out, scene, over, lab in ENTRIES:
        r = render(out, scene, over)
        total += r["secs"]
        prog.append(caption(Image.open(os.path.join(GALLERY, out)),
                            f"{lab}   -   rendered in {fmt_time(r['secs'])}"))
    dur = [2700] * len(prog); dur[-1] = 4200          # dwell ~50% longer than the first cut
    prog[0].save(os.path.join(GALLERY, "room-ellipsoid-progression.webp"), save_all=True,
                 append_images=prog[1:], duration=dur, loop=0, quality=90, method=6)
    # the stills are only frame sources for the WebP above; don't leave them in the gallery
    for out, *_ in ENTRIES:
        os.remove(os.path.join(GALLERY, out))
    head = dict(file="room-ellipsoid-progression.webp", title="The progression, as one loop",
        desc="The same 200 objects cycling through each stage - spheres, affine ellipsoids, "
             "reflective, then refractive glass. Each frame shows its own render time as it plays.",
        meta=f"1024x1024 -> 640x640  -  4 frames (animated WebP)  -  rendered in {fmt_time(total)}")
    write_section(3, "spheres_ellipsoids", "Spheres & Ellipsoids",
                  "One material idea at a time on a field of 200 objects inside a colored room: "
                  "plain spheres, the same objects transformed into ellipsoids, then made reflective, "
                  "then refractive glass.", [head])
    build_index()
