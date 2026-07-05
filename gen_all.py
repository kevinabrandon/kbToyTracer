#!/usr/bin/env python3
"""Regenerate the whole gallery: wipe old output, run every section, build the index.

Individual sections can also be run on their own, e.g.:
    python3 gen_refraction.py
which re-renders only that section and rebuilds the index (leaving the other,
already-rendered sections untouched).
"""
import subprocess, sys, os, glob
HERE = os.path.dirname(os.path.abspath(__file__))
SECTIONS = ["gen_class.py", "gen_spheres_ellipsoids.py", "gen_arealights.py",
            "gen_refraction.py", "gen_teapot.py"]

# Full rebuild: clear previously rendered images and section manifests first.
for f in glob.glob(os.path.join(HERE, "gallery", "*.png")) + \
         glob.glob(os.path.join(HERE, "gallery", "sections", "*.json")):
    os.remove(f)

for s in SECTIONS:
    print(f"\n=== {s} ===")
    subprocess.run([sys.executable, os.path.join(HERE, s)], check=True)
