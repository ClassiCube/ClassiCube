#include "Random.h"
#include "Platform.h"
#define RND_VALUE (0x5DEECE66DLL)
#define RND_MASK ((1LL << 48) - 1)

void Random_Init(Random* seed, Int32 seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

void Random_InitFromCurrentTime(Random* rnd) {
	DateTime now = Platform_CurrentUTCTime();
	Int64 totalMS = DateTime_TotalMilliseconds(&now);
	Random_Init(rnd, (Int32)totalMS);
}

Int32 Random_Range(Random* seed, Int32 min, Int32 max) {
	return min + Random_Next(seed, max - min);
}

Int32 Random_Next(Random* seed, Int32 n) {
	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		Int64 raw = (Int64)((UInt64)*seed >> (48 - 31));
		return (Int32)((n * raw) >> 31);
	}

	Int32 bits, val;
	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits = (Int32)((UInt64)*seed >> (48 - 31));
		val = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

Real32 Random_Float(Random* seed) {
	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	Int32 raw = (Int32)((UInt64)*seed >> (48 - 24));
	return raw / ((Real32)(1 << 24));
}