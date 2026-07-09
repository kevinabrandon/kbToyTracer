#!/usr/bin/env python3
"""Section 6: depth of field over the imported models. A thin lens racks focus
through the stage-12 scene (scenes/dof-dragon.sdf) -- from the gold dragon up
front, onto the pink-marble teapot, then back to Lucy in the corner -- dwelling
on each. Same recipe as the glass-scene rack in gen_refraction.py (size,
samples, smoothstep easing, segment lengths, ping-pong timing); only the scene
and the focal keys differ. Frame scenes are written next to the original in
scenes/ so its relative imports resolve."""
import os, time, subprocess, re
from PIL import Image
from gallery_lib import write_section, build_index, fmt_time, BIN, SCENES, GALLERY, THREADS

def _smooth(x): return x * x * (3 - 2 * x)   # ease in/out

def render_rack(out, size=480, samps=10):
    """Rack focus dragon -> teapot -> Lucy, dwelling at each."""
    keys, seg = [5.85, 7.73, 9.66], [5, 11]   # P1 dragon, P2 teapot, P3 Lucy
    focals, keyidx = [keys[0]], [0]
    for i in range(len(keys) - 1):
        a, b = keys[i], keys[i + 1]
        for j in range(1, seg[i] + 1): focals.append(a + (b - a) * _smooth(j / seg[i]))
        keyidx.append(len(focals) - 1)
    base = open(os.path.join(SCENES, "dof-dragon.sdf")).read().replace("\r", "")
    cfg = (f"width {size}\nheight {size}\naa_samples 1\ndof_samples {samps}\nshadow_samples 0\n"
           f"enable_reflection 1\nenable_refraction 1\nenable_shadows 1\nmax_bounces 8\n")
    cfgpath = os.path.join(SCENES, "_dof_rack.cfg")
    open(cfgpath, "w").write(cfg)
    frames, t0 = [], time.time()
    for i, fd in enumerate(focals):
        sdf = os.path.join(SCENES, f"_dof_rack_{i}.sdf")
        ppm = os.path.join(SCENES, f"_dof_rack_{i}.ppm")
        open(sdf, "w").write(re.sub(r'focalDist\s+[\d.]+', f'focalDist {fd:.4f}', base))
        subprocess.run([BIN, sdf, ppm, cfgpath],
                       env={**os.environ, "OMP_NUM_THREADS": THREADS},
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        frames.append(Image.open(ppm).convert("RGB"))
        os.remove(sdf); os.remove(ppm)
        print(f"  rack {i+1}/{len(focals)} focal={fd:.2f} ({time.time()-t0:.0f}s)", flush=True)
    os.remove(cfgpath)
    secs = time.time() - t0
    N = len(frames); order = list(range(N)) + list(range(N - 2, 0, -1))   # ping-pong
    dur = [550 if order[p] in set(keyidx) else 110 for p in range(len(order))]  # dwell at P1/P2/P3
    seq = [frames[i] for i in order]
    seq[0].save(os.path.join(GALLERY, out), save_all=True, append_images=seq[1:],
                duration=dur, loop=0, quality=88, method=6)
    return dict(size=size, nframes=N, samps=samps, secs=secs)

if __name__ == "__main__":
    r = render_rack("60-dof-rack.webp")
    card = dict(file="60-dof-rack.webp", title="Focus rack: dragon, teapot, Lucy",
        desc="A thin-lens camera (lensRadius 0.21) racking focus through the imported-model "
             "scene: the gold Stanford dragon up front, the pink-marble teapot mid-field, "
             "and Lucy in the back corner, each snapping into focus in turn while the "
             "others melt into bokeh.",
        meta=f"{r['size']}x{r['size']}  -  {r['nframes']} frames (animated WebP)  -  "
             f"{r['samps']}x{r['samps']} lens samples/pixel  -  "
             f"{fmt_time(r['secs'])}, {r['secs']/r['nframes']:.1f} s/frame")
    write_section(6, "dof", "Depth of Field",
                  "The thin-lens camera on the imported-model scene: a focus rack across the "
                  "three meshes. The scene defines the lens (lensRadius, focalDist); the config "
                  "only sets how many rays sample the aperture.", [card])
    build_index()
