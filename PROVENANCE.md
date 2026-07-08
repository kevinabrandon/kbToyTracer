# Provenance

The tracer began as James Arvo's **toytracer** teaching kernel (EECS 204, UC Irvine,
Fall 2004), archived faithfully at
[arvo-toytracer-archive](https://github.com/kevinabrandon/arvo-toytracer-archive).
Everything else is Kevin Brandon's coursework and later hobby code.

The naming convention in `src/`: **files prefixed `kb` are Kevin's; unprefixed files
are Arvo's originals** (with, at most, small marked changes). Files that are truly a
mix carry a `Provenance:` note in their header comment.

## Arvo's files (unprefixed)

Byte-faithful to the archive except for C++17 portability fixes
(`<iostream.h>` → `<iostream>`, `std::` qualification):

| File | Notes |
|---|---|
| `Color.h`, `Mat3x3.h`, `Mat3x4.h`, `Vec2.h`, `Vec3.h` | untouched |
| `point.cpp` | untouched |
| `triangle.cpp` | one marked change: normal flipped to match kbToyTracer's winding convention |
| `quad.cpp` | marked changes: normal flipped, `uv` defaulted |
| `util.cpp` | marked change: `rand()` rewritten thread-local so OpenMP rendering is race-free (2026) |
| `box.cpp` | KB restructuring (declarations moved to `kbBox.h`, Arvo's handout ray-box intersector merged in); one marked change: planar face `uv` coords for texturing (2026) |

## Mixed files (Arvo skeleton, KB extensions — see each header's `Provenance:` note)

| File | KB additions |
|---|---|
| `toytracer.h` | `Lens` (depth of field), `Material` texture/refractivity fields, `HitInfo.uv`, `inside_ray` |
| `Image.h` | PPM-reading constructor and `kbGetPixel` (for image textures) |
| `sphere.cpp` | spherical uv coords; `GetSamples` solid-angle sampling of sphere lights (soft shadows) |
| `list.cpp` | per-child affine transforms; declarations moved to `kbList.h` |

## Kevin's files (`kb` prefix)

`kbMain.cpp` (CLI, 2026), `kbTrace.cpp` (render loop: supersampling, adaptive/stochastic AA,
thin-lens DoF, OpenMP), `kbShade.cpp` (shader: refraction, area lights, indirect light),
`kbReader.cpp` (scene parser with textures/refraction), `kbConfig.*` (runtime settings, 2026),
`kbRandom.*` (seedable per-thread RNG, 2026), `kbPng.*` (dependency-free PNG writer, 2026),
`kbBVH.*`, `kbSpatialSub.*`, `kbPlane.cpp`, `kbList.h`, `kbBox.h`,
`kbTexture.h` + `kbCheckerTex.h`, `kbStripeTex.h`, `kbDefaultTex.h`, `kbMarbleTex.*`,
`kbImageTex.*`, and `kbSolidNoise.*` (Perlin-style noise after Shirley & Morley).

The three heavily-extended files were named `shade1.cpp`, `reader1.cpp`, and
`toytracer1.cpp` in the 2004/5 sources; they were renamed `kbShade.cpp`, `kbReader.cpp`,
and `kbTrace.cpp` in 2026. `legacy/` holds their superseded Win32-era class versions,
unmodified.

`kbSpatialSub` (the `begin SSub` aggregate) is a 2004 homework solution kept as a
historical exhibit: of the acceleration structures offered in the assignment it
sounded like the easiest to program, and it rendered the required 200-ellipsoid
scene. 2026 benchmarks show it does real work on big scenes -- the 229k-triangle
teapot renders ~38x faster through SSub than through the brute-force List (5 s
vs 3.2 min at 64x64) -- but its fixed overhead (a 12^3 grid tested box-by-box
per ray, no DDA traversal, no mailboxing) means that on scenes of a few hundred
objects it actually loses to the linear scan, and the BVH beats it everywhere
(~65x on the teapot). So it is frozen as-is: it does not support `matrix`
transforms (it warns if you try) and new features are not added to it. Use
`begin BVH`.

## Third-party

`src/stb_image_write.h` — Sean Barrett's single-header image writer
(github.com/nothings/stb), dual-licensed public domain / MIT (see the file's
footer). Used by `kbPng.cpp` for compressed PNG output when present; without
it the build falls back to kbPng's own uncompressed encoder.
