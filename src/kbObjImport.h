/***************************************************************************
* kbObjImport.h                                                            *
*                                                                          *
* Loads a Wavefront obj file as a single aggregate (a BVH of triangles),   *
* suitable for use as an instancing prototype: in a scene file,            *
*     import models/dragon.obj as dragon                                   *
* then place it with "instance dragon".                                    *
*                                                                          *
* Supported: v / vn / vt / f (any polygon, fan-triangulated; negative      *
* indices allowed).  Faces with vertex normals become smooth-shaded        *
* kbSmoothTriangles; faces without shade flat.  Materials (.mtl) are       *
* ignored -- give each instance its own material in the scene file.        *
*                                                                          *
* History:                                                                 *
*   07/2026  Initial coding (K. Brandon).                                  *
*                                                                          *
***************************************************************************/
#ifndef KB_OBJ_IMPORT_H
#define KB_OBJ_IMPORT_H

#include "toytracer.h"

// Returns the model as an aggregate, or NULL (with a message) on failure.
extern Object *kbImportObj( const char *path );

#endif
