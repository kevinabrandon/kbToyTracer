#!/usr/bin/env python3
"""Section 1: the class progression, as one animated WebP. The renderer is built up
one feature at a time -- exactly the order it was implemented in EECS 204: a flat
sphere from the given ray tracer, then the box intersector, diffuse shading, shadows,
specular highlights, reflection, and finally an affine transform.

The two "flat" stages have no lighting model, so their ambient is boosted to 1.0 to
show the raw material color (the scene's real ambient is 0.2, used once shading is on)."""
import os, time, subprocess, re
from PIL import Image, ImageDraw, ImageFont
from gallery_lib import write_section, build_index, fmt_time, BIN, SCENES, GALLERY, THREADS

SIZE = 640
try:
    FONT = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24)
except Exception:
    FONT = ImageFont.load_default()

# label, scene, config toggles (cumulative), ambient override (None = scene default)
STAGES = [
  ("1 · flat sphere — the given ray tracer", "scene1-sphereonly.sdf",
   dict(enable_shading=0, enable_shadows=0, enable_specular=0, enable_reflection=0), 1.0),
  ("2 · box intersector added", "scene1-reflection.sdf",
   dict(enable_shading=0, enable_shadows=0, enable_specular=0, enable_reflection=0), 1.0),
  ("3 · diffuse shading", "scene1-reflection.sdf",
   dict(enable_shading=1, enable_shadows=0, enable_specular=0, enable_reflection=0), None),
  ("4 · shadows", "scene1-reflection.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=0, enable_reflection=0), None),
  ("5 · specular highlights", "scene1-reflection.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=0), None),
  ("6 · reflection", "scene1-reflection.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1), None),
  ("7 · affine transform — sphere → ellipsoid", "scene2-matrix.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1), None),
]

def render_stage(scene, over, ambient, tag):
    text = open(os.path.join(SCENES, scene)).read().replace("\r", "")
    if ambient is not None:
        text = re.sub(r'amblight\s+\[[^\]]*\]', f'amblight [{ambient}, {ambient}, {ambient}]', text)
    open(f"/tmp/class_{tag}.sdf", "w").write(text)
    cfg = dict(width=SIZE, height=SIZE, aa_samples=3,
               max_bounces=6, enable_refraction=0)
    cfg.update(over)
    open(f"/tmp/class_{tag}.cfg", "w").write("".join(f"{k} {v}\n" for k, v in cfg.items()))
    subprocess.run([BIN, f"/tmp/class_{tag}.sdf", f"/tmp/class_{tag}.ppm", f"/tmp/class_{tag}.cfg"],
                   env={**os.environ, "OMP_NUM_THREADS": THREADS},
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return Image.open(f"/tmp/class_{tag}.ppm").convert("RGB")

def caption(img, text):
    d = ImageDraw.Draw(img, "RGBA")
    d.rectangle([0, img.height - 40, img.width, img.height], fill=(0, 0, 0, 160))
    d.text((14, img.height - 32), text, font=FONT, fill=(240, 240, 240))
    return img

if __name__ == "__main__":
    frames = []; t0 = time.time()
    for i, (lab, scene, over, amb) in enumerate(STAGES):
        frames.append(caption(render_stage(scene, over, amb, i), lab))
        print(f"  stage {i+1}/{len(STAGES)}: {lab}  ({time.time()-t0:.0f}s)", flush=True)
    secs = time.time() - t0
    dur = [1600] * len(frames); dur[-1] = 3200          # linger on the finished scene
    frames[0].save(os.path.join(GALLERY, "01-class-progression.webp"), save_all=True,
                   append_images=frames[1:], duration=dur, loop=0, quality=90, method=6)
    card = dict(file="01-class-progression.webp",
        title="Building the renderer, feature by feature",
        desc="The order it was implemented in class: a flat sphere from the given ray tracer, then "
             "the box intersector, diffuse shading, shadows, specular highlights, reflection, and "
             "finally an affine transform turning the sphere into an ellipsoid.",
        meta=f"{SIZE}x{SIZE}  -  {len(frames)} frames (animated WebP)  -  3x3 AA  -  "
             f"{fmt_time(secs)}, {secs/len(frames):.1f} s/frame")
    write_section(1, "class", "Class Scenes",
                  "The class scene built up one feature at a time - the progression each student "
                  "worked through in EECS 204, as a single loop.", [card])
    build_index()
