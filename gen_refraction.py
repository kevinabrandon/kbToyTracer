#!/usr/bin/env python3
"""Section 4: refraction, in motion. Two animated WebP loops:
  - a depth-of-field focus rack through the scene (blurry.sdf), and
  - a blue glass sphere morphing into a flattened lens (refract-focus.sdf).
Each is rendered frame-by-frame by the tracer and assembled into an animated WebP."""
import os, time, subprocess, re
import numpy as np
from PIL import Image
from gallery_lib import write_section, build_index, fmt_time, BIN, SCENES, GALLERY, THREADS

def _render_frames(texts, cfg_text, tag):
    """Render each scene-text with a shared config; return (PIL frames, total seconds)."""
    open(f"/tmp/{tag}.cfg", "w").write(cfg_text)
    frames, t0 = [], time.time()
    for i, txt in enumerate(texts):
        open(f"/tmp/{tag}_{i}.sdf", "w").write(txt)
        subprocess.run([BIN, f"/tmp/{tag}_{i}.sdf", f"/tmp/{tag}_{i}.ppm", f"/tmp/{tag}.cfg"],
                       env={**os.environ, "OMP_NUM_THREADS": THREADS},
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        frames.append(Image.open(f"/tmp/{tag}_{i}.ppm").convert("RGB"))
        print(f"  {tag} {i+1}/{len(texts)} ({time.time()-t0:.0f}s)", flush=True)
    return frames, time.time() - t0

def _smooth(x): return x * x * (3 - 2 * x)   # ease in/out

def render_dof_rack(out, size=480, samps=10):
    """Rack focus from halfway-to-glass -> glass sphere -> objects behind it, dwelling at each."""
    eye = np.array([5.0, -2.0, 2.8])
    d_glass = np.linalg.norm(np.array([4.059599, -1.466952, 2.195396]) - eye)
    keys, seg = [d_glass / 2, d_glass, 6.3], [5, 11]     # P1 soft, P2 glass, P3 objects behind
    focals, keyidx = [keys[0]], [0]
    for i in range(len(keys) - 1):
        a, b = keys[i], keys[i + 1]
        for j in range(1, seg[i] + 1): focals.append(a + (b - a) * _smooth(j / seg[i]))
        keyidx.append(len(focals) - 1)
    base = open(os.path.join(SCENES, "blurry.sdf")).read().replace("\r", "")
    texts = [re.sub(r'focalDist\s+[\d.]+', f'focalDist {fd:.4f}', base) for fd in focals]
    cfg = (f"width {size}\nheight {size}\naa_samples 1\ndof_samples {samps}\nshadow_samples 0\n"
           f"enable_reflection 1\nenable_refraction 1\nenable_shadows 1\nmax_bounces 8\n")
    frames, secs = _render_frames(texts, cfg, "rack")
    N = len(frames); order = list(range(N)) + list(range(N - 2, 0, -1))   # ping-pong
    dur = [550 if order[p] in set(keyidx) else 110 for p in range(len(order))]  # dwell at P1/P2/P3
    seq = [frames[i] for i in order]
    seq[0].save(os.path.join(GALLERY, out), save_all=True, append_images=seq[1:],
                duration=dur, loop=0, quality=88, method=6)
    return dict(size=size, nframes=N, samps=samps, secs=secs)

def render_morph(out, size=480, nframes=32, samps=2, bounces=8):
    """Blue glass sphere morphing into a camera-facing lens by ramping its affine matrix."""
    eye = np.array([5.0, -2.0, 2.8]); center = np.array([4.059599, -1.466952, 2.195396])
    R0, THIN, WIDE = 0.2, 0.12, 0.32
    M0 = R0 * np.eye(3); vhat = (center - eye); vhat /= np.linalg.norm(vhat)
    u = np.cross(vhat, [0, 0, 1]); u /= np.linalg.norm(u); w = np.cross(vhat, u)
    M1 = np.column_stack([THIN * vhat, WIDE * u, WIDE * w])
    base = open(os.path.join(SCENES, "refract-focus.sdf")).read()
    base = base.replace("\t\tdiffuse\t\t[0.78, 0.95, 0.96]", "\t\tdiffuse\t\t[0, 0.14, 0.6]")
    base = base.replace("\t\treflectivity\t0\n\t\trefractivity\t0.175",
                        "\t\treflectivity\t0.08\n\t\trefractivity\t0.7")
    base = base.replace("\t\tindex_refract\t1.6", "\t\tindex_refract\t1.5")
    base = base.replace("\t\tPhong_exp\t\t15", "\t\tPhong_exp\t\t40")
    old = "\t\tsphere\t\t(4.059599, -1.466952, 2.195396) 0.2"
    texts = []
    for i in range(nframes):
        t = _smooth(i / (nframes - 1)); M = (1 - t) * M0 + t * M1
        rows = [f"{M[r,0]:.6f}, {M[r,1]:.6f}, {M[r,2]:.6f}, {center[r]:.6f}" for r in range(3)]
        obj = "\t\tsphere\t\t(0,0,0) 1\n\t\tmatrix ( " + "; ".join(rows) + " )"
        texts.append(base.replace(old, obj))
    cfg = (f"width {size}\nheight {size}\naa_samples {samps}\ndof_samples 0\nshadow_samples 0\n"
           f"enable_reflection 1\nenable_refraction 1\nenable_shadows 1\nmax_bounces {bounces}\n")
    frames, secs = _render_frames(texts, cfg, "morph")
    N = len(frames); order = list(range(N)) + list(range(N - 2, 0, -1))
    dur = [900 if order[p] in (0, N - 1) else 70 for p in range(len(order))]   # hold the sphere & the lens
    seq = [frames[i] for i in order]
    seq[0].save(os.path.join(GALLERY, out), save_all=True, append_images=seq[1:],
                duration=dur, loop=0, quality=90, method=6)
    return dict(size=size, nframes=N, samps=samps, secs=secs)

if __name__ == "__main__":
    cards = []
    r = render_dof_rack("40-depth-of-field-rack.webp")
    cards.append(dict(file="40-depth-of-field-rack.webp", title="Depth of Field - focus rack",
        desc="A thin-lens camera racking focus through the scene: from a soft foreground, onto the "
             "refractive glass sphere, then onto the reflective sphere and box behind it.",
        meta=f"{r['size']}x{r['size']}  -  {r['nframes']} frames (animated WebP)  -  "
             f"{r['samps']}x{r['samps']} lens samples/pixel  -  "
             f"{fmt_time(r['secs'])}, {r['secs']/r['nframes']:.1f} s/frame"))
    m = render_morph("41-glass-morph.webp")
    cards.append(dict(file="41-glass-morph.webp", title="Sphere to Lens (animated)",
        desc="A blue glass sphere flattened into a camera-facing lens by ramping its affine matrix. "
             "Refraction through a transformed primitive, tumbling through an edge-on view.",
        meta=f"{m['size']}x{m['size']}  -  {m['nframes']} frames (animated WebP)  -  "
             f"{m['samps']}x{m['samps']} AA  -  "
             f"{fmt_time(m['secs'])}, {m['secs']/m['nframes']:.1f} s/frame"))
    write_section(4, "refraction", "Glass Sphere & Refraction",
                  "Refraction in motion: a depth-of-field focus rack through the scene, and a glass "
                  "sphere morphing into a lens. Both are animated WebP loops.", cards)
    build_index()
