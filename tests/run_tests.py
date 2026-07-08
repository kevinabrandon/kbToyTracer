#!/usr/bin/env python3
"""Golden-image regression tests for kbToyTracer.

    python3 tests/run_tests.py            # run all tests against tests/golden/
    python3 tests/run_tests.py --bless    # (re)generate the golden images
    python3 tests/run_tests.py aa dof     # run only tests whose name contains a term

Each case renders 128x128 with a fixed seed (so Monte-Carlo features are
deterministic) and must match its golden image pixel-for-pixel. Goldens are
machine-specific in principle (FP rounding), so bless them on the box that
runs the tests. Failures write got/diff images into tests/failures/.
"""
import os, sys, subprocess, shutil, tempfile

REPO   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN    = os.path.join(REPO, "kbtoytracer")
GOLDEN = os.path.join(REPO, "tests", "golden")
FAIL   = os.path.join(REPO, "tests", "failures")
SCENES = os.path.join(REPO, "scenes")
TSCENE = os.path.join(REPO, "tests", "scenes")

# Base config: every case starts from this, then applies its overrides.
BASE = {
    "width": 128, "height": 128, "seed": 42,
    "aa_samples": 2, "dof_samples": 2, "shadow_samples": 2,
    "max_bounces": 6, "display_messages": 0,
}

# (name, scene, {overrides}, what-it-tests).  Scene paths are relative to
# scenes/ unless they start with "tests/" or "@" (@ = generated variant,
# see make_scene).  The descriptions also feed tests/index.html.
CASES = [
    ("point-light-reflection", "scene1-reflection.sdf",        {},
     "The classic class scene through the BVH: point lights (single analytic "
     "shadow ray each), Lambertian shading, Phong highlights, hard shadows, and "
     "mirror reflection on the center sphere."),
    ("matrix-ellipsoid",       "scene2-matrix.sdf",            {},
     "Affine-transformed primitives: a sphere carrying a 3x4 matrix becomes an "
     "ellipsoid; exercises the transform/inverse-ray path in the aggregates."),
    ("area-light-soft",        "spherelight.sdf",              {},
     "Auto-detected area light (an emissive sphere): Monte-Carlo soft shadows "
     "with the 2x2 stratified sample grid, seeded so the noise is reproducible."),
    ("area-light-1samp",       "spherelight.sdf",              {"shadow_samples": 1},
     "Same area light with a single shadow sample per shading point: hard-edged "
     "but randomly jittered shadows -- the noisiest configuration."),
    ("point-path-forced",      "spherelight.sdf",              {"shadow_samples": 0},
     "shadow_samples=0 forces the analytic point-light path even for area "
     "lights (the light's center is used): crisp shadows, no noise."),
    ("dof",                    "blurry.sdf",                   {},
     "Thin-lens depth of field, activated automatically by the lens in the "
     "scene file: 2x2 seeded aperture-disk samples per AA sample."),
    ("dof-off",                "blurry.sdf",                   {"dof_samples": 0},
     "The same lens scene with dof_samples=0: the lens is ignored and "
     "everything renders pin-sharp."),
    ("reflection-room",        "room-ellipsoids-reflect.sdf",  {"shadow_samples": 0, "max_bounces": 8},
     "Mirror reflection at depth: ~200 reflective ellipsoids in a colored room, "
     "8 bounces, traced through the BVH."),
    ("refraction-room",        "room-ellipsoids-refract-70.sdf", {"shadow_samples": 0, "max_bounces": 8},
     "Snell-law refraction with total internal reflection: the same room with "
     "refractivity-0.70 glass ellipsoids."),
    ("transparent-shadows",    "room-ellipsoids-refract-70.sdf",
        {"enable_transparent_shadows": 1, "max_bounces": 8},
     "Opt-in transparent shadows: shadow rays accumulate transmittance through "
     "the glass ellipsoids, so glass casts lighter shadows (also exercises the "
     "area-light path, shadow_samples=2)."),
    ("aa-off",                 "scene1-reflection.sdf",        {"aa_samples": 1},
     "Single ray through each pixel center: the jagged no-anti-aliasing "
     "baseline every AA mode is compared against."),
    ("aa-jitter",              "scene1-reflection.sdf",        {"aa_jitter": 1},
     "Stochastic supersampling: the 2x2 AA grid offsets are jittered with the "
     "seeded RNG instead of sitting on the uniform grid."),
    ("aa-adaptive",            "scene1-reflection.sdf",        {"aa_adaptive": 1},
     "Adaptive AA: 4 corner rays per pixel first; only pixels whose corners "
     "disagree beyond aa_threshold get the full sample grid."),
    ("bg-escaped-rays",        "scene1-reflection.sdf",        {"bg_escaped_rays": 1},
     "The class-era escape behavior: reflection rays that leave the scene "
     "return the background color instead of black (tints the mirror sphere)."),
    ("textures",               "tests/textures.sdf",           {},
     "Procedural textures on spheres (uv + solid noise): checkerboard, stripes, "
     "and Perlin-noise marble, plus specular over a textured surface."),
    ("aggregate-list",         "@scene1-list",                 {},
     "scene1 rewritten to use the brute-force List aggregate: every ray tests "
     "every object. Must render identically to the BVH version."),
    ("aggregate-ssub",         "@scene1-ssub",                 {},
     "scene1 rewritten to use the uniform spatial subdivision (SSub) grid. "
     "Must render identically to the BVH version."),
    ("matrix-in-list",         "@rotated-list",                {},
     "Per-object affine transforms inside the List aggregate: a sphere squashed "
     "to an ellipsoid and a block, both rotated 10 degrees about z by matrix "
     "lines. Renders byte-identically through List and BVH."),
    ("matrix-in-bvh",          "@rotated-bvh",                 {},
     "The same transformed scene through the BVH (which delegates transforms to "
     "its internal List). Golden is identical to matrix-in-list."),
    ("nested-bvh-transformed", "tests/nested.sdf",             {},
     "A BVH nested inside a BVH, with a matrix on the inner one: scene1's two "
     "blocks live in a sub-BVH rotated 10 degrees about z as a single unit "
     "(single-instance two-level-BVH semantics). Verified to render identically "
     "when the outer aggregate is a List."),
    # NOTE: deliberately no matrix-in-ssub case. SpatialSub is frozen as a
    # historical exhibit (see PROVENANCE.md): it does not support transforms
    # and warns if a scene tries. Its supported subset is pinned by
    # aggregate-ssub above.
    ("instancing",             "tests/instancing.sdf",         {},
     "define + instance: one prototype placed four times through the BVH with "
     "composed translate/rotate/scale lines and per-instance materials; the "
     "right-most instance is untouched and must inherit the prototype's purple."),
    ("instancing-list",        "@instancing-list",             {},
     "The same instancing scene through the brute-force List aggregate: pins "
     "the material copy-back in List's transformed-child intersection path. "
     "Golden is identical to the BVH version."),
    ("import-sdf",             "tests/import-sdf.sdf",         {},
     "import <file> as <name>: the octahedral gem model (a standalone sdf with "
     "its own camera and light, both skipped with warnings) instanced three "
     "times -- untouched yellow, squashed red, stretched reflective blue."),
    ("import-obj",             "tests/import-obj.sdf",         {},
     "Wavefront obj import: a quad-faced cube (fan triangulation, flat "
     "shading) and an icosphere with vertex normals (smooth kbSmoothTriangle "
     "shading, incl. under non-uniform scale), each instanced twice."),
]

def make_scene(tag, tmpdir):
    """Generated scene variants: a base scene with a different aggregate."""
    base, have, agg = {
        "@scene1-list":  ("scene1-reflection.sdf", "BVH",  "List"),
        "@scene1-ssub":  ("scene1-reflection.sdf", "BVH",  "SSub"),
        "@rotated-list": ("tests/rotated.sdf",     "List", "List"),
        "@rotated-bvh":  ("tests/rotated.sdf",     "List", "BVH"),
        "@rotated-ssub": ("tests/rotated.sdf",     "List", "SSub"),
        "@instancing-list": ("tests/instancing.sdf", "BVH", "List"),
    }[tag]
    src = open(scene_path(base, tmpdir)).read()
    path = os.path.join(tmpdir, tag[1:] + ".sdf")
    open(path, "w").write(src.replace("begin " + have, "begin " + agg))
    return path

def scene_path(scene, tmpdir):
    if scene.startswith("@"):      return make_scene(scene, tmpdir)
    if scene.startswith("tests/"): return os.path.join(TSCENE, scene[6:])
    return os.path.join(SCENES, scene)

def render(name, scene, over, outdir, tmpdir):
    cfg = dict(BASE); cfg.update(over)
    cfgpath = os.path.join(tmpdir, name + ".cfg")
    with open(cfgpath, "w") as fh:
        for k, v in cfg.items(): fh.write(f"{k} {v}\n")
    out = os.path.join(outdir, name + ".png")
    r = subprocess.run([BIN, scene_path(scene, tmpdir), out, cfgpath],
                       capture_output=True, text=True)
    if r.returncode != 0 or not os.path.exists(out):
        sys.exit(f"FAIL {name}: tracer exited {r.returncode}\n{r.stderr}")
    return out

def pixels(path):
    from PIL import Image
    import numpy as np
    return np.asarray(Image.open(path).convert("RGB")).astype(int)

def write_html():
    """Regenerate tests/index.html from the case table."""
    import html as H
    cards = []
    for name, scene, over, desc in CASES:
        cfg = dict(BASE); cfg.update(over)
        knobs = "  ".join(f"{k}={v}" for k, v in sorted(over.items())) or "base settings"
        cards.append(f"""<div class="card">
 <img src="golden/{name}.png" alt="{name}" width="256" height="256">
 <h2>{H.escape(name)}</h2>
 <p class="desc">{H.escape(desc)}</p>
 <p class="meta">scene: {H.escape(scene)}<br>overrides: {H.escape(knobs)}</p>
</div>""")
    base = "  ".join(f"{k}={v}" for k, v in BASE.items())
    page = f"""<!doctype html><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>kbToyTracer golden tests</title>
<style>
 body{{background:#111;color:#ddd;font:15px system-ui;margin:20px auto;max-width:1200px;padding:0 16px}}
 h1{{font-size:22px}} h2{{font-size:15px;margin:8px 0 4px}}
 .lead{{color:#8a8f98;max-width:80ch}}
 .grid{{display:grid;grid-template-columns:repeat(auto-fill,minmax(280px,1fr));gap:18px}}
 .card{{background:#1a1c1f;border-radius:8px;padding:12px}}
 img{{width:100%;height:auto;image-rendering:pixelated;border-radius:4px;background:#000}}
 .desc{{font-size:13px;color:#c8ccd4;line-height:1.45;margin:4px 0}}
 .meta{{font-size:12px;color:#777;font-family:monospace;margin:4px 0 0}}
</style>
<h1>kbToyTracer golden-image tests</h1>
<p class="lead">Each card is a blessed reference render: 128x128, fixed seed, compared
pixel-for-pixel by <code>make test</code> (tests/run_tests.py). Base config:
<code>{H.escape(base)}</code>; each case lists only what it overrides.
This page is generated by <code>run_tests.py --html</code> -- don't edit by hand.</p>
<div class="grid">
{chr(10).join(cards)}
</div>
"""
    out = os.path.join(REPO, "tests", "index.html")
    with open(out, "w") as fh: fh.write(page)
    return out

def main():
    args = [a for a in sys.argv[1:]]
    bless = "--bless" in args
    if "--html" in args:
        print("wrote", write_html()); return
    terms = [a for a in args if not a.startswith("--")]
    cases = [c for c in CASES if not terms or any(t in c[0] for t in terms)]
    if not cases: sys.exit("no matching tests")
    if not os.path.exists(BIN): sys.exit("build first: make")

    os.makedirs(GOLDEN, exist_ok=True)
    shutil.rmtree(FAIL, ignore_errors=True)

    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name, scene, over, _desc in cases:
            if bless:
                # Render twice; a golden that isn't reproducible is useless.
                a = render(name, scene, over, GOLDEN, tmp)
                b = render(name + ".check", scene, over, tmp, tmp)
                import numpy as np
                if not (pixels(a) == pixels(b)).all():
                    sys.exit(f"BLESS FAILED {name}: not deterministic across runs")
                print(f"  blessed {name}")
                continue

            gold = os.path.join(GOLDEN, name + ".png")
            if not os.path.exists(gold):
                print(f"  MISSING golden for {name} (run --bless)"); failures += 1; continue
            got = render(name, scene, over, tmp, tmp)
            import numpy as np
            a, b = pixels(gold), pixels(got)
            if a.shape == b.shape and (a == b).all():
                print(f"  ok   {name}")
            else:
                failures += 1
                os.makedirs(FAIL, exist_ok=True)
                shutil.copyfile(got, os.path.join(FAIL, name + ".got.png"))
                if a.shape == b.shape:
                    d = np.abs(a - b)
                    from PIL import Image
                    Image.fromarray((d.sum(2).clip(0, 255)).astype("uint8")) \
                         .save(os.path.join(FAIL, name + ".diff.png"))
                    print(f"  FAIL {name}: {(d.sum(2) > 0).sum()} px differ (max {d.max()})")
                else:
                    print(f"  FAIL {name}: size {b.shape} vs golden {a.shape}")

    if bless:
        write_html()
        print(f"blessed {len(cases)} goldens in tests/golden/ (and refreshed index.html)")
    elif failures:
        sys.exit(f"{failures}/{len(cases)} tests FAILED (see tests/failures/)")
    else:
        print(f"all {len(cases)} tests passed")

if __name__ == "__main__":
    main()
