# kbSPD — Standard Procedural Databases, SDF edition

Eric Haines's SPD 3.14 with an added output format (`-r 20`, the default)
that emits kbToyTracer `.sdf` scenes. Each generator builds a classic
benchmark scene at arbitrary complexity:

    make                          # builds every generator
    ./teapot -s 12 > teapot.sdf   # -s controls detail; higher = more geometry
    ../../kbtoytracer teapot.sdf

Generators: `balls` (sphereflake), `gears`, `mount` (fractal mountain),
`rings`, `teapot`, `tetra` (recursive tetrahedron), `tree`, `jacks`,
`lattice`, `sample`, `shells`, `sombrero`, `nurbtst`; plus the stock
converters `readnff`, `readobj`, `readdxf`, `nff2rad`. `prog -?` lists
options; all other SPD output formats (NFF, POV-Ray, OBJ, RIB, VRML, ...)
still work via `-r`.

Generated scenes are complete (camera, lights, materials) and render
standalone. They also compose: `import <file>.sdf as <name>` in another
scene pulls one in as an instancing prototype (its camera and lights are
skipped — but note the whole scene comes along, ground plane included).
For a clean prototype, `teapot -f` and `balls -f` omit their floors (a
kbSPD addition; the other generators still emit their full scenes).

## Provenance

- Base: https://github.com/erich666/StandardProceduralDatabases
  @ 5c860e2 (CC0; see LICENSE and Readme.txt). SPD is Eric Haines's
  1987 ray-tracing benchmark suite; 3.14 (Feb 2005) is its final version.
- The `OUTPUT_SDF` format is Kevin Brandon's, written July 2005 against
  the then-brand-new 3.14 (recovered from
  `~/dev/school/eecs204/kbToyTracer/sdfCreators/`), re-applied to the
  GitHub copy in July 2026.

## Changes vs. the 2005 port

- Scenes are wrapped in `begin BVH` / `end` (was `begin List`), emitted
  lazily before the first light/surface/primitive, and every surface opens
  its own nested `begin BVH` group. The nesting is load-bearing: the reader
  applies material lines to the aggregate default AND retroactively to the
  most recent object, so without it each material block repaints the last
  primitive of the previous surface (the 2005 port nested `begin List`,
  which fixes the same problem but leaves big groups unaccelerated).
- Output streams to stdout (dropped the Windows-era `OUTPUT_TO_FILE`).
- `vpdist` is computed from SPD's field of view (`1/tan(fov/2)`); the
  2005 port wrote the hither distance (1.0 = 90°), framing everything
  too wide.
- Tessellated surfaces with vertex normals (e.g. the teapot) emit
  `smooth_triangle`; the 2005 port always emitted flat `triangle`.
- Materials honor NFF semantics: `diffuse = Kd * color`, `specular` and
  `reflectivity` from Ks (the 2005 port ignored Kd and used a fixed
  white specular).
- Polygons with more than 4 vertices are fan-triangulated (were dropped).
- Curved primitives the SDF has no analytic type for (cones, cylinders,
  discs, tori, superquadrics, boxes) tessellate like the other
  polygon-only formats; spheres stay analytic.
- MSVC-only `*_s` CRT calls (from upstream's VS2017 pass) are shimmed
  for non-Windows builds; fixed the missing VRML1 entry in the
  filename-suffix table.
