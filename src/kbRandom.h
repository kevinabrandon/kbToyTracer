//
//	kbRandom.h
//
//	Random numbers for all Monte-Carlo sampling (AA jitter, lens rays,
//	soft shadows).  Each thread owns its own generator, so parallel
//	rendering is race-free.  When Config.seed is nonzero, kbReseed()
//	re-seeds deterministically (the render loop calls it per row), making
//	renders reproducible run-to-run -- essential for animation frames.
//

#ifndef KB_RANDOM_H
#define KB_RANDOM_H

// Re-seed the calling thread's generator from (Config.seed, stream).
// Use a distinct stream id per unit of work (e.g. the row index).
extern void kbReseed( unsigned stream );

// rand( a, b ) itself is declared in toytracer.h (it is part of the
// original Arvo interface); kbRandom.cpp provides the implementation.

#endif
