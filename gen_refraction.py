#!/usr/bin/env python3
"""Section 4: one glass sphere, three ways - refraction and depth of field -
plus an animated sphere->lens morph.

Perfect-focus refracting sphere, the same glass flattened by an affine matrix into
a lens, then the classic depth-of-field shot. The animated entry ramps the affine
matrix from the round sphere (0.2*I) to the flatten transform. Chasing the flattened
case is what exposed the per-light reflection/refraction bug (now fixed in shade1.cpp)."""
import os, time, subprocess
import numpy as np
from PIL import Image
from gallery_lib import (render, settings_line, write_section, build_index,
                         fmt_time, BIN, SCENES, GALLERY, THREADS)

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

# Glass sphere position and the flatten (thin, camera-oriented, wide) target, matching
# scenes/refract-flat.sdf. The morph interpolates the 3x3 matrix from 0.2*I to this.
EYE    = np.array([5.0, -2.0, 2.8])
CENTER = np.array([4.059599, -1.466952, 2.195396])
R0, THIN, WIDE = 0.2, 0.07, 0.32

def build_morph(out_gif, size=480, nframes=16, samps=2, bounces=8, ms=110):
    M0 = R0 * np.eye(3)
    vhat = (CENTER - EYE); vhat /= np.linalg.norm(vhat)
    u = np.cross(vhat, [0, 0, 1]); u /= np.linalg.norm(u)
    w = np.cross(vhat, u)
    M1 = np.column_stack([THIN * vhat, WIDE * u, WIDE * w])

    base = open(os.path.join(SCENES, "refract-focus.sdf")).read()
    old = "\t\tsphere\t\t(4.059599, -1.466952, 2.195396) 0.2"
    assert old in base
    cfg = "/tmp/morph_render.cfg"
    open(cfg, "w").write(f"width {size}\nheight {size}\nenable_supersample 1\n"
                         f"numSampsLarge {samps}\nenable_camera_lens 0\nenable_reflection 1\n"
                         f"enable_refraction 1\nenable_shadows 1\nnumBounces {bounces}\n")
    frames = []
    t0 = time.time()
    for i in range(nframes):
        t = i / (nframes - 1)
        M = (1 - t) * M0 + t * M1                      # linear matrix morph
        rows = [f"{M[r,0]:.6f}, {M[r,1]:.6f}, {M[r,2]:.6f}, {CENTER[r]:.6f}" for r in range(3)]
        obj = "\t\tsphere\t\t(0,0,0) 1\n\t\tmatrix ( " + "; ".join(rows) + " )"
        sp = f"/tmp/morph_{i}.sdf"; open(sp, "w").write(base.replace(old, obj))
        subprocess.run([BIN, sp, f"/tmp/morph_{i}.ppm", cfg],
                       env={**os.environ, "OMP_NUM_THREADS": THREADS},
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        frames.append(Image.open(f"/tmp/morph_{i}.ppm").convert("RGB"))
        print(f"  morph frame {i+1}/{nframes}", flush=True)
    secs = time.time() - t0
    seq = frames + frames[-2:0:-1]                     # ping-pong loop
    seq[0].save(os.path.join(GALLERY, out_gif), save_all=True, append_images=seq[1:],
                duration=ms, loop=0, optimize=True)
    return dict(nframes=nframes, size=size, secs=secs)

if __name__ == "__main__":
    cards = []
    for out, title, scene, over, desc in ENTRIES:
        r = render(out, scene, over)
        cards.append(dict(file=out, title=title, desc=desc,
                          meta=settings_line(r["cfg"], r["secs"])))
    m = build_morph("43-glass-morph.gif")
    cards.append(dict(file="43-glass-morph.gif", title="Sphere to Lens (animated)",
        desc="Ramping the affine matrix from the round sphere (0.2*I) to the flatten "
             "transform in even steps. The lens tumbles through an edge-on view, "
             "revealing that it really is a thin disc rather than a sphere.",
        meta=f"{m['size']}x{m['size']}  -  {m['nframes']} frames, ping-pong loop"
             f"  -  rendered in {fmt_time(m['secs'])}"))
    write_section(4, "refraction", "Glass Sphere & Refraction",
                  "One glass sphere, several ways: a refracting sphere in perfect focus, the "
                  "same glass flattened into a lens, depth of field, and an animated morph "
                  "from sphere to lens.", cards)
    build_index()
