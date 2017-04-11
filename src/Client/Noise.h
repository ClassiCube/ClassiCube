#ifndef CS_IMPROVED_NOISE_H
#define CS_IMPROVED_NOISE_H
#include "Typedefs.h"
#include "Random.h"

#define NOISE_TABLE_SIZE 512
void ImprovedNoise_Init(UInt8* p, Random* rnd);
Real32 ImprovedNoise_Compute(UInt8* p, Real32 x, Real32 y);


/* since we need structure to be a fixed size */
#define MAX_OCTAVES 8
typedef struct {
	UInt8 p[MAX_OCTAVES][NOISE_TABLE_SIZE];
	Int32 octaves;
} OctaveNoise;
void OctaveNoise_Init(OctaveNoise* n, Random* rnd, Int32 octaves);
Real32 OctaveNoise_Compute(OctaveNoise* n, Real32 x, Real32 y);


typedef struct {
	OctaveNoise noise1;
	OctaveNoise noise2;
} CombinedNoise;
void CombinedNoise_Init(CombinedNoise* n, Random* rnd, Int32 octaves1, Int32 octaves2);
Real32 CombinedNoise_Compute(CombinedNoise* n, Real32 x, Real32 y);

#endif