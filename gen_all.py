#!/usr/bin/env python3
"""Regenerate the whole gallery: run every section, then build the index.

Individual sections can also be run on their own, e.g.:
    python3 gen_spheres_ellipsoids.py
which re-renders only that section and rebuilds the index (leaving the slow
teapot / depth-of-field frames untouched).
"""
import subprocess, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
SECTIONS = ["gen_class.py", "gen_spheres_ellipsoids.py", "gen_arealights.py", "gen_teapot_dof.py"]
for s in SECTIONS:
    print(f"\n=== {s} ===")
    subprocess.run([sys.executable, os.path.join(HERE, s)], check=True)
