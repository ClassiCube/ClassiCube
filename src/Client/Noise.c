#include "Noise.h"

void ImprovedNoise_Init(UInt8* p, Random* rnd) {
	/* shuffle randomly using fisher-yates */
	Int32 i;
	for (i = 0; i < 256; i++) {
		p[i] = (UInt8)i;
	}

	for (i = 0; i < 256; i++) {
		Int32 j = Random_Range(rnd, i, 256);
		UInt8 temp = p[i]; p[i] = p[j]; p[j] = temp;
	}

	for (i = 0; i < 256; i++) {
		p[i + 256] = p[i];
	}
}

Real32 ImprovedNoise_Calc(UInt8* p, Real32 x, Real32 y) {
	Int32 xFloor = x >= 0 ? (Int32)x : (Int32)x - 1;
	Int32 yFloor = y >= 0 ? (Int32)y : (Int32)y - 1;
	Int32 X = xFloor & 0xFF, Y = yFloor & 0xFF;
	x -= xFloor; y -= yFloor;

	Real32 u = x * x * x * (x * (x * 6 - 15) + 10); /* Fade(x) */
	Real32 v = y * y * y * (y * (y * 6 - 15) + 10); /* Fade(y) */
	Int32 A = p[X] + Y, B = p[X + 1] + Y;

	/* Normally, calculating Grad involves a function call. However, we can directly pack this table
	 (since each value indicates either -1, 0 1) into a set of bit flags. This way we avoid needing 
	 to call another function that performs branching */
	#define xFlags 0x46552222
	#define yFlags 0x2222550A

	Int32 hash = (p[p[A]] & 0xF) << 1;
	Real32 g22 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * y; /* Grad(p[p[A], x, y) */
	hash = (p[p[B]] & 0xF) << 1;
	Real32 g12 = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * y; /* Grad(p[p[B], x - 1, y) */
	Real32 c1 = g22 + u * (g12 - g22);

	hash = (p[p[A + 1]] & 0xF) << 1;
	Real32 g21 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * (y - 1); /* Grad(p[p[A + 1], x, y - 1) */
	hash = (p[p[B + 1]] & 0xF) << 1;
	Real32 g11 = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * (y - 1); /* Grad(p[p[B + 1], x - 1, y - 1) */
	Real32 c2 = g21 + u * (g11 - g21);

	return c1 + v * (c2 - c1);
}


void OctaveNoise_Init(OctaveNoise* n, Random* rnd, Int32 octaves) {
	n->octaves = octaves;
	Int32 i;
	for (i = 0; i < octaves; i++) {
		ImprovedNoise_Init(n->p[i], rnd);
	}
}

Real32 OctaveNoise_Calc(OctaveNoise* n, Real32 x, Real32 y) {
	Real32 amplitude = 1, freq = 1;
	Real32 sum = 0;
	Int32 i;

	for (i = 0; i < n->octaves; i++) {
		sum += ImprovedNoise_Calc(n->p[i], x * freq, y * freq) * amplitude;
		amplitude *= 2.0;
		freq *= 0.5;
	}
	return sum;
}


void CombinedNoise_Init(CombinedNoise* n, Random* rnd, Int32 octaves1, Int32 octaves2) {
	OctaveNoise_Init(&n->noise1, rnd, octaves1);
	OctaveNoise_Init(&n->noise2, rnd, octaves2);
}

Real32 CombinedNoise_Calc(CombinedNoise* n, Real32 x, Real32 y) {
	Real32 offset = OctaveNoise_Calc(&n->noise2, x, y);
	return OctaveNoise_Calc(&n->noise1, x + offset, y);
}