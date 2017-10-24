#ifndef CC_IMPROVED_NOISE_H
#define CC_IMPROVED_NOISE_H
#include "Typedefs.h"
#include "Random.h"
/* Implements perlin noise generation.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define NOISE_TABLE_SIZE 512
/* Initalises an ImprovedNoise instance. */
void ImprovedNoise_Init(UInt8* p, Random* rnd);
/* Calculates a noise value at the given coordinates. */
Real32 ImprovedNoise_Calc(UInt8* p, Real32 x, Real32 y);

/* since we need structure to be a fixed size */
#define MAX_OCTAVES 8
typedef struct OctaveNoise_ {
	UInt8 p[MAX_OCTAVES][NOISE_TABLE_SIZE];
	Int32 octaves;
} OctaveNoise;

/* Initalises an OctaveNoise instance. */
void OctaveNoise_Init(OctaveNoise* n, Random* rnd, Int32 octaves);
/* Calculates a noise value at the given coordinates. */
Real32 OctaveNoise_Calc(OctaveNoise* n, Real32 x, Real32 y);

typedef struct CombinedNoise_ {
	OctaveNoise noise1;
	OctaveNoise noise2;
} CombinedNoise;

/* Initalises a CombinedNoise instance. */
void CombinedNoise_Init(CombinedNoise* n, Random* rnd, Int32 octaves1, Int32 octaves2);
/* Calculates a noise value at the given coordinates. */
Real32 CombinedNoise_Calc(CombinedNoise* n, Real32 x, Real32 y);
#endif