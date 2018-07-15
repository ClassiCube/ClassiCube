#ifndef CC_IMPROVED_NOISE_H
#define CC_IMPROVED_NOISE_H
#include "Random.h"
/* Implements perlin noise generation.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define NOISE_TABLE_SIZE 512
void ImprovedNoise_Init(UInt8* p, Random* rnd);
Real32 ImprovedNoise_Calc(UInt8* p, Real32 x, Real32 y);

/* since we need structure to be a fixed size */
#define MAX_OCTAVES 8
struct OctaveNoise {
	UInt8 p[MAX_OCTAVES][NOISE_TABLE_SIZE];
	Int32 octaves;
};

void OctaveNoise_Init(struct OctaveNoise* n, Random* rnd, Int32 octaves);
Real32 OctaveNoise_Calc(struct OctaveNoise* n, Real32 x, Real32 y);

struct CombinedNoise {
	struct OctaveNoise noise1;
	struct OctaveNoise noise2;
};

void CombinedNoise_Init(struct CombinedNoise* n, Random* rnd, Int32 octaves1, Int32 octaves2);
Real32 CombinedNoise_Calc(struct CombinedNoise* n, Real32 x, Real32 y);
#endif
