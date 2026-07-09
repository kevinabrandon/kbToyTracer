#!/usr/bin/env python3
"""Section 1: the feature progression, as one animated WebP. The renderer is built up
one feature at a time. Stages 1-8 are the class work, in exactly the order it was
implemented in EECS 204: a flat sphere from the given ray tracer, then the box and
quad intersectors (the blocks and the colored room go up together), diffuse shading,
shadows, specular highlights, reflection, an affine transform, and a spherical area
light with soft shadows. Stages 9 on were added later (2026): 70% refractive glass,
checkerboards on the floor (40% reflective) and the box, and finally everything
re-ground in marble -- pink ellipsoid, purple box, olive floor (ellipsoid and floor
at a subtle 5% reflectivity). Stage 12 shows off import/instance: a gold Stanford
dragon sits up front, the imported Utah teapot sits behind it in the ellipsoid's
pink marble (polished up to 30% reflective, its belly catching a distorted
reflection of the dragon), Lucy stands in the back corner as a weathered white-marble
statue with soft cloudy gray mottling, and the floor goes back to its stage-10
checkerboard at half the reflectivity.

(scenes/scene7-dof.sdf adds a thin lens on top of the final stage; it's parked
outside the progression until the scene has an object near the viewport to sell
the blur.)

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
  ("2 · boxes & quads — the room goes up", "scene1-reflection-room.sdf",
   dict(enable_shading=0, enable_shadows=0, enable_specular=0, enable_reflection=0), 1.0),
  ("3 · diffuse shading", "scene1-reflection-room.sdf",
   dict(enable_shading=1, enable_shadows=0, enable_specular=0, enable_reflection=0), None),
  ("4 · shadows", "scene1-reflection-room.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=0, enable_reflection=0), None),
  ("5 · specular highlights", "scene1-reflection-room.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=0), None),
  ("6 · reflection", "scene1-reflection-room.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1), None),
  ("7 · affine transform — sphere → ellipsoid", "scene2-matrix-room.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1), None),
  ("8 · spherical light — soft shadows", "scene3-spherelight.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1,
        shadow_samples=4), None),
  ("9 · refraction — the ellipsoid goes 70% glass", "scene4-refraction.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1,
        enable_refraction=1, shadow_samples=4), None),
  ("10 · checkered floor (40% reflective) & box", "scene5-checker-floor.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1,
        enable_refraction=1, shadow_samples=4), None),
  ("11 · marble ellipsoid, box & floor", "scene6-marble.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1,
        enable_refraction=1, shadow_samples=4), None),
  ("12 · import & instance — the models move in", "scene8-imports.sdf",
   dict(enable_shading=1, enable_shadows=1, enable_specular=1, enable_reflection=1,
        enable_refraction=1, shadow_samples=4), None),
]

def render_stage(scene, over, ambient, tag):
    # Render the scene in place so relative imports resolve; only the
    # ambient-boosted stages get a rewritten copy (written next to the
    # scene, for the same reason).
    scene_path = os.path.join(SCENES, scene)
    tmp_scene = None
    if ambient is not None:
        text = open(scene_path).read().replace("\r", "")
        text = re.sub(r'amblight\s+\[[^\]]*\]', f'amblight [{ambient}, {ambient}, {ambient}]', text)
        tmp_scene = scene_path + f".class_{tag}.tmp.sdf"
        open(tmp_scene, "w").write(text)
        scene_path = tmp_scene
    cfg = dict(width=SIZE, height=SIZE, aa_samples=3, max_bounces=6,
               enable_refraction=0, shadow_samples=0, dof_samples=0, seed=42)
    cfg.update(over)
    open(f"/tmp/class_{tag}.cfg", "w").write("".join(f"{k} {v}\n" for k, v in cfg.items()))
    subprocess.run([BIN, scene_path, f"/tmp/class_{tag}.ppm", f"/tmp/class_{tag}.cfg"],
                   env={**os.environ, "OMP_NUM_THREADS": THREADS},
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if tmp_scene is not None:
        os.remove(tmp_scene)
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
    dur = [1600] * len(frames)
    dur[7] = 2400   # pause where the class work ends (stage 8) ...
    dur[-1] = 3200  # ... and linger on the finished scene
    frames[0].save(os.path.join(GALLERY, "01-class-progression.webp"), save_all=True,
                   append_images=frames[1:], duration=dur, loop=0, quality=90, method=6)
    card = dict(file="01-class-progression.webp",
        title="Building the renderer, feature by feature",
        desc="Stages 1-8 are the class work, in the order it was implemented: a flat sphere from "
             "the given ray tracer, then the box and quad intersectors (raising the blocks and a "
             "colored room: red in back, green right, cyan left, yellow behind the camera), diffuse "
             "shading, shadows, specular highlights, reflection, an affine transform turning the "
             "sphere into an ellipsoid, and a spherical light with soft shadows. Stages 9 and up "
             "were added later: the ellipsoid re-ground as 70% glass, checkerboards on the floor "
             "(40% reflective) and the box, and finally marble everywhere: pink on the ellipsoid, "
             "purple on the box, and olive on the floor (ellipsoid and floor at a subtle 5% "
             "reflectivity). The last stage shows off import/instance: a gold Stanford dragon "
             "(871k triangles) sits up front, the imported Utah teapot (229k) sits behind it in "
             "the ellipsoid's pink marble, polished up to 30% reflective so its belly catches a "
             "distorted reflection of the dragon, Lucy (100k) stands in the back corner as a "
             "weathered white-marble statue with soft cloudy gray mottling, and the floor goes "
             "back to its stage-10 checkerboard at half the reflectivity -- each mesh imported "
             "once and placed with per-instance transforms and materials.",
        meta=f"{SIZE}x{SIZE}  -  {len(frames)} frames (animated WebP)  -  3x3 AA  -  "
             f"4x4 soft-shadow samples on the later frames  -  "
             f"{fmt_time(secs)}, {secs/len(frames):.1f} s/frame")
    write_section(1, "class", "Feature Progression",
                  "The renderer built up one feature at a time, as a single loop: stages 1-8 are "
                  "the progression worked through in EECS 204; stages 9 and beyond were added "
                  "later.", [card])
    build_index()
