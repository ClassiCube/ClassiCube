#include "ExtMath.h"
#include "Platform.h"
#include "Utils.h"
#include <math.h>

/* TODO: Replace with own functions that don't rely on stdlib */

Real32 Math_AbsF(Real32 x)  { return fabsf(x); /* MSVC intrinsic */ }
Real32 Math_SqrtF(Real32 x) { return sqrtf(x); /* MSVC intrinsic */ }
Real32 Math_Mod1(Real32 x)  { return x - (Int32)x; /* fmodf(x, 1); */ }
Int32 Math_AbsI(Int32 x)    { return abs(x); /* MSVC intrinsic */ }

Real64 Math_Sin(Real64 x) { return sin(x); }
Real64 Math_Cos(Real64 x) { return cos(x); }
Real64 Math_Log(Real64 x) { return log(x); }
Real64 Math_Exp(Real64 x) { return exp(x); }

Real32 Math_SinF(Real32 x) { return (Real32)Math_Sin(x); }
Real32 Math_CosF(Real32 x) { return (Real32)Math_Cos(x); }

Int32 Math_Floor(Real32 value) {
	Int32 valueI = (Int32)value;
	return valueI > value ? valueI - 1 : valueI;
}

Int32 Math_Ceil(Real32 value) {
	Int32 valueI = (Int32)value;
	return valueI < value ? valueI + 1 : valueI;
}

Int32 Math_Log2(UInt32 value) {
	UInt32 r = 0;
	while (value >>= 1) r++;
	return r;
}

Int32 Math_CeilDiv(Int32 a, Int32 b) {
	return a / b + (a % b != 0 ? 1 : 0);
}

Int32 Math_Sign(Real32 value) {
	if (value > 0.0f) return +1;
	if (value < 0.0f) return -1;
	return 0;
}

Real32 Math_Lerp(Real32 a, Real32 b, Real32 t) {
	return a + (b - a) * t;
}

Real32 Math_LerpAngle(Real32 leftAngle, Real32 rightAngle, Real32 t) {
	/* We have to cheat a bit for angles here */
	/* Consider 350* --> 0*, we only want to travel 10* */
	/* But without adjusting for this case, we would interpolate back the whole 350* degrees */
	bool invertLeft  = leftAngle  > 270.0f && rightAngle < 90.0f;
	bool invertRight = rightAngle > 270.0f && leftAngle  < 90.0f;
	if (invertLeft)  leftAngle  = leftAngle  - 360.0f;
	if (invertRight) rightAngle = rightAngle - 360.0f;

	return Math_Lerp(leftAngle, rightAngle, t);
}

Int32 Math_NextPowOf2(Int32 value) {
	Int32 next = 1;
	while (value > next) { next <<= 1; }
	return next;
}

bool Math_IsPowOf2(Int32 value) {
	return value != 0 && (value & (value - 1)) == 0;
}

Int32 Math_AccumulateWheelDelta(Real32* accmulator, Real32 delta) {
	/* Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2) */
	/* We must use rounding at final step, not at every intermediate step. */
	*accmulator += delta;
	Int32 steps = (Int32)*accmulator;
	*accmulator -= steps;
	return steps;
}

/* Not the most precise Tan(x), but within 10^-15 of answer, so good enough for here */
Real64 Math_FastTan(Real64 angle) {
	Real64 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	if (cosA < -0.00000001 || cosA > 0.00000001) return sinA / cosA;

	/* tan line is parallel to y axis, infinite gradient */
	Int32 sign = Math_Sign(sinA);
	if (cosA) sign *= Math_Sign(cosA);
	return sign * MATH_POS_INF;
}

Real64 Math_FastLog(Real64 x) {
	/* x = 2^exp * mantissa */
	/* so log(x) = log(2^exp) + log(mantissa) */
	/* so log(x) = exp*log(2) + log(mantissa) */

	/* now need to work out log(mantissa) */
}

Real64 Math_FastExp(Real64 x) {
	/* let x = k*log(2) + f, where k is integer */
	/* so exp(x) = exp(k*log(2)) * exp(f) */
	/* so exp(x) = exp(log(2^k)) * exp(f) */
	/* so exp(x) = 2^k           * exp(f) */

	/* now need to work out exp(f) */
}


/*########################################################################################################################*
*--------------------------------------------------Random number generator------------------------------------------------*
*#########################################################################################################################*/
#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_Init(Random* seed, Int32 seedInit) { Random_SetSeed(seed, seedInit); }
void Random_InitFromCurrentTime(Random* rnd) {
	UInt64 now = DateTime_CurrentUTC_MS();
	Random_Init(rnd, (Int32)now);
}

void Random_SetSeed(Random* seed, Int32 seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

Int32 Random_Range(Random* seed, Int32 min, Int32 max) {
	return min + Random_Next(seed, max - min);
}

Int32 Random_Next(Random* seed, Int32 n) {
	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		Int64 raw = (Int64)(*seed >> (48 - 31));
		return (Int32)((n * raw) >> 31);
	}

	Int32 bits, val;
	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits = (Int32)(*seed >> (48 - 31));
		val = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

Real32 Random_Float(Random* seed) {
	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	Int32 raw = (Int32)(*seed >> (48 - 24));
	return raw / ((Real32)(1 << 24));
}

