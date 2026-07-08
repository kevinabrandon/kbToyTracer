//
//	kbRandom.cpp
//
//	See kbRandom.h.  The rand(a,b) interface is Arvo's (toytracer.h);
//	this per-thread, seedable implementation replaces the libc-rand
//	version that originally lived in util.cpp.
//

#include "toytracer.h"
#include "kbRandom.h"
#include "kbConfig.h"
#include <random>

static thread_local std::mt19937 gen( std::random_device{}() );

void kbReseed( unsigned stream )
{
    std::seed_seq seq{ Config.seed, stream };
    gen.seed( seq );
}

double rand( double a, double b )
{
    std::uniform_real_distribution<double> dist( a, b );
    return dist( gen );
}
