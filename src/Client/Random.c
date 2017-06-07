#include "Random.h"
#define value (0x5DEECE66DLL)
#define mask ((1LL << 48) - 1)

void Random_Init(Random* seed, Int32 seedInit) {
	*seed = (seedInit ^ value) & mask;
}

Int32 Random_Range(Random* seed, Int32 min, Int32 max) {
	return min + Random_Next(seed, max - min);
}

Int32 Random_Next(Random* seed, Int32 n) {
	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * value + 0xBLL) & mask;
		Int64 raw = (Int64)((UInt64)*seed >> (48 - 31));
		return (Int32)((n * raw) >> 31);
	}

	Int32 bits, val;
	do {
		*seed = (*seed * value + 0xBLL) & mask;
		bits = (Int32)((UInt64)*seed >> (48 - 31));
		val = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

Real32 Random_Float(Random* seed) {
	*seed = (*seed * value + 0xBLL) & mask;
	Int32 raw = (Int32)((UInt64)*seed >> (48 - 24));
	return raw / ((Real32)(1 << 24));
}