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
| `box.cpp` | all Arvo code; KB restructuring only (declarations moved to `kbBox.h`, Arvo's handout ray-box intersector merged in) |

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
`kbBVH.*`, `kbSpatialSub.*`, `kbPlane.cpp`, `kbList.h`, `kbBox.h`,
`kbTexture.h` + `kbCheckerTex.h`, `kbStripeTex.h`, `kbDefaultTex.h`, `kbMarbleTex.*`,
`kbImageTex.*`, and `kbSolidNoise.*` (Perlin-style noise after Shirley & Morley).

The three heavily-extended files were named `shade1.cpp`, `reader1.cpp`, and
`toytracer1.cpp` in the 2004/5 sources; they were renamed `kbShade.cpp`, `kbReader.cpp`,
and `kbTrace.cpp` in 2026. `legacy/` holds their superseded Win32-era class versions,
unmodified.
