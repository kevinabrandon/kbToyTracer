"""
Shared helpers for the kbToyTracer gallery.

The gallery is split into sections (sub-galleries). Each section has its own
generator script (gen_*.py) that renders its images and writes a small JSON
manifest into gallery/sections/. build_index() then assembles every manifest,
in order, into a single gallery/index.html with section headings.

This means you can re-run just one section (e.g. tweak the spheres) without
re-rendering the slow teapot or depth-of-field frames.
"""
import os, subprocess, time, json, glob, html

REPO     = os.path.dirname(os.path.abspath(__file__))
BIN      = os.path.join(REPO, "kbtoytracer")
SCENES   = os.path.join(REPO, "scenes")
GALLERY  = os.path.join(REPO, "gallery")
SECTIONS = os.path.join(GALLERY, "sections")
SIZE     = 1024
THREADS  = "4"

# Baseline render config; each entry overrides what it needs.
# shadow_samples=0 / dof_samples=0 pin the classic point-light / no-DoF look
# for reproducibility (several gallery scenes use small sphere emitters and
# lenses, which the tracer now picks up automatically); sections that want
# soft shadows or depth of field override these explicitly.
BASE = dict(width=SIZE, height=SIZE, aa_samples=3, max_bounces=6,
            enable_shadows=1, enable_reflection=1, enable_refraction=1,
            bg_escaped_rays=1, shadow_samples=0, dof_samples=0)

os.makedirs(GALLERY, exist_ok=True)
os.makedirs(SECTIONS, exist_ok=True)


def fmt_time(s):
    return f"{s:.1f} s" if s < 60 else f"{int(s//60)} min {int(s%60)} s"


def detect_machine():
    """Describe the machine actually running the render, so a clone reports its own
    CPU / cores / OS alongside its own timings. Linux-first, with graceful fallbacks."""
    import platform
    cpu = platform.processor() or platform.machine() or "unknown CPU"
    try:                                            # Linux
        for line in open("/proc/cpuinfo"):
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip(); break
    except OSError:
        try:                                        # macOS
            cpu = subprocess.check_output(["sysctl", "-n", "machdep.cpu.brand_string"],
                                          text=True).strip()
        except Exception:
            pass
    cores = os.cpu_count() or 1
    osname = f"{platform.system()} {platform.release()}"
    try:                                            # Linux distro pretty name
        for line in open("/etc/os-release"):
            if line.startswith("PRETTY_NAME="):
                pretty = line.split("=", 1)[1].strip().strip('"')
                osname = f"{pretty} (Linux {platform.release().split('-')[0]})"; break
    except OSError:
        pass
    return f"Rendered on {cpu} · {cores} cores · {osname}"


def settings_line(cfg, seconds):
    """Human-readable summary of the render settings actually used."""
    parts = [f"{cfg['width']}x{cfg['height']}"]
    if cfg.get("dof_samples"):
        n = cfg["dof_samples"]; parts.append(f"{n}x{n} lens samples (depth of field)")
    elif cfg.get("aa_samples", 1) > 1:
        n = cfg["aa_samples"]; parts.append(f"{n}x{n} anti-aliasing ({n*n} rays/pixel)")
    else:
        parts.append("1 ray/pixel")
    if cfg.get("shadow_samples"):
        n = cfg["shadow_samples"]; parts.append(f"{n}x{n} = {n*n} area-light samples")
    if cfg.get("enable_reflection") or cfg.get("enable_refraction"):
        parts.append(f"up to {cfg['max_bounces']} reflections/refractions")
    parts.append(f"rendered in {fmt_time(seconds)}")
    return "  -  ".join(parts)


def ensure_scene(scene):
    """Return the path to a scene, transparently decompressing scenes/foo.sdf.gz
    to scenes/foo.sdf on first use. Large scenes ship gzipped to keep the repo small."""
    path = os.path.join(SCENES, scene)
    if not os.path.exists(path) and os.path.exists(path + ".gz"):
        import gzip, shutil
        print(f"  decompressing {scene}.gz ...", flush=True)
        with gzip.open(path + ".gz", "rb") as fin, open(path, "wb") as fout:
            shutil.copyfileobj(fin, fout)
    return path


def render(out, scene, cfg_over):
    """Render one image into gallery/, returning a card dict."""
    from PIL import Image
    ensure_scene(scene)
    cfg = dict(BASE); cfg.update(cfg_over)
    cfgpath = "/tmp/gallery_render.cfg"
    with open(cfgpath, "w") as fh:
        for k, v in cfg.items(): fh.write(f"{k} {v}\n")
    ppm = f"/tmp/gallery_{out}.ppm"
    print(f"  rendering {out} ({scene}) ...", flush=True)
    t0 = time.time()
    subprocess.run([BIN, os.path.join(SCENES, scene), ppm, cfgpath],
                   env={**os.environ, "OMP_NUM_THREADS": THREADS},
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    secs = time.time() - t0
    Image.open(ppm).save(os.path.join(GALLERY, out))
    print(f"     {fmt_time(secs)}", flush=True)
    return dict(cfg=cfg, secs=secs)


def write_section(order, name, title, blurb, cards):
    """Persist a section's cards to gallery/sections/NN_name.json."""
    path = os.path.join(SECTIONS, f"{order:02d}_{name}.json")
    json.dump(dict(order=order, title=title, blurb=blurb, cards=cards),
              open(path, "w"), indent=1)
    print(f"  wrote section '{title}' ({len(cards)} images)")


def build_index():
    """Assemble every section manifest into gallery/index.html."""
    sections = [json.load(open(p)) for p in sorted(glob.glob(os.path.join(SECTIONS, "*.json")))]
    sections.sort(key=lambda s: s["order"])
    out = ["""<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>kbToyTracer - render gallery</title>
<style>
 body{background:#15161a;color:#e6e6e6;font-family:system-ui,sans-serif;margin:0;padding:24px}
 h1{font-weight:600;margin:0 0 4px}
 .lead{color:#8a8f98;margin:0 0 10px;max-width:74ch}
 .repo{margin:0 0 28px;font-size:14px}
 .repo a{color:#6cf;text-decoration:none}
 .repo a:hover{text-decoration:underline}
 h2{font-weight:600;font-size:20px;margin:34px 0 2px;border-bottom:1px solid #2a2d36;padding-bottom:6px}
 .blurb{color:#8a8f98;margin:0 0 6px;max-width:74ch;font-size:14px}
 .machine{color:#6b7078;margin:0 0 16px;font-size:12px;font-family:ui-monospace,monospace}
 .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(320px,1fr));gap:20px}
 .card{background:#1e2027;border-radius:10px;overflow:hidden;box-shadow:0 2px 10px #0006}
 .card img{width:100%;display:block;background:#000;cursor:zoom-in;aspect-ratio:1/1}
 .body{padding:11px 13px}
 .title{font-size:15px;font-weight:600;margin:0 0 4px}
 .desc{font-size:13px;color:#c8ccd4;line-height:1.45;margin:0 0 7px}
 .meta{font-size:11.5px;color:#8a8f98;font-family:ui-monospace,monospace;line-height:1.5}
 a{color:inherit;text-decoration:none}
</style></head><body>
<h1>kbToyTracer - render gallery</h1>
<p class="lead">Rendered by James Arvo's toytracer (UC Irvine, Fall 2004), extended by
Kevin Brandon and resurrected in 2026. Organized by feature; every image is rendered fresh
by the section scripts. Click any image for full size.</p>
<p class="repo"><a href="https://github.com/kevinabrandon/kbToyTracer">Source, scene files &amp; render scripts on GitHub &rarr;</a></p>
"""]
    machine = detect_machine()
    for s in sections:
        out.append(f'<h2>{html.escape(s["title"])}</h2>')
        if s.get("blurb"): out.append(f'<p class="blurb">{html.escape(s["blurb"])}</p>')
        out.append(f'<p class="machine">{html.escape(machine)}</p>')
        out.append('<div class="grid">')
        for c in s["cards"]:
            out.append(f'<div class="card"><a href="{c["file"]}" target="_blank">'
                       f'<img src="{c["file"]}" loading="lazy"></a><div class="body">'
                       f'<p class="title">{html.escape(c["title"])}</p>'
                       f'<p class="desc">{html.escape(c["desc"])}</p>'
                       f'<p class="meta">{html.escape(c["meta"])}</p></div></div>')
        out.append('</div>')
    out.append("</body></html>")
    open(os.path.join(GALLERY, "index.html"), "w").write("\n".join(out))
    total = sum(len(s["cards"]) for s in sections)
    print(f"index built: {len(sections)} sections, {total} images -> {GALLERY}/index.html")
