# Model library

Meshes for `import ... as <name>` (see the main README's scene-format notes).
Use them in a scene with:

    import models/dragon.obj.gz as dragon
    instance dragon
    ...

| File | What | Source & license |
|---|---|---|
| `CornellBox-Original.obj/.mtl` | The Cornell box (36 triangles after fan triangulation), per-face materials incl. the red/green walls | Guedis Cardenas & Morgan McGuire, Williams College 2011, via the [McGuire Computer Graphics Archive](https://casual-effects.com/data/); released into the **public domain** |
| `dragon.obj.gz` | The Stanford dragon, 871k triangles with vertex normals (smooth shading) | Scan by the [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/), OBJ conversion via the McGuire Computer Graphics Archive; Stanford scan terms: free with attribution ("Stanford Computer Graphics Laboratory"), no commercial use of the model itself |
| `lucy.obj.gz` | Lucy, decimated to ~100k triangles; area-weighted vertex normals baked in here (the source file had none) | Scan by the Stanford 3D Scanning Repository (full scan is 28M triangles); decimated OBJ via [alecjacobson/common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models); same Stanford attribution terms |

Notes:

- The `.obj.gz` files are read directly by the importer; no need to decompress.
- The Cornell box's emissive ceiling panel (`Ke` in the .mtl) renders as a
  glowing surface but does not light the scene -- lights are scene objects in
  this tracer, so scenes add a point light inside the box
  (see `scenes/cornell-box.sdf`).
- Orientation: the Cornell box and dragon are y-up (`rotate (1,0,0) 90` makes
  them z-up); Lucy is z-up but in raw scanner millimetres, off-center -- see
  `scenes/lucy.sdf` for the recentering transform.
