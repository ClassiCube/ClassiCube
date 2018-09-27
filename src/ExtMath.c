#include "ExtMath.h"
#include "Platform.h"
#include "Utils.h"
#include <math.h>

/* TODO: Replace with own functions that don't rely on stdlib */

float Math_AbsF(float x)  { return fabsf(x); /* MSVC intrinsic */ }
float Math_SqrtF(float x) { return sqrtf(x); /* MSVC intrinsic */ }
float Math_Mod1(float x)  { return x - (Int32)x; /* fmodf(x, 1); */ }
Int32 Math_AbsI(Int32 x)    { return abs(x); /* MSVC intrinsic */ }

double Math_Sin(double x) { return sin(x); }
double Math_Cos(double x) { return cos(x); }
double Math_Log(double x) { return log(x); }
double Math_Exp(double x) { return exp(x); }

float Math_SinF(float x) { return (float)Math_Sin(x); }
float Math_CosF(float x) { return (float)Math_Cos(x); }

Int32 Math_Floor(float value) {
	Int32 valueI = (Int32)value;
	return valueI > value ? valueI - 1 : valueI;
}

Int32 Math_Ceil(float value) {
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

Int32 Math_Sign(float value) {
	if (value > 0.0f) return +1;
	if (value < 0.0f) return -1;
	return 0;
}

float Math_Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

float Math_LerpAngle(float leftAngle, float rightAngle, float t) {
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

Int32 Math_AccumulateWheelDelta(float* accmulator, float delta) {
	/* Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2) */
	/* We must use rounding at final step, not at every intermediate step. */
	*accmulator += delta;
	Int32 steps = (Int32)*accmulator;
	*accmulator -= steps;
	return steps;
}

/* Not the most precise Tan(x), but within 10^-15 of answer, so good enough for here */
double Math_FastTan(double angle) {
	double cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	if (cosA < -0.00000001 || cosA > 0.00000001) return sinA / cosA;

	/* tan line is parallel to y axis, infinite gradient */
	Int32 sign = Math_Sign(sinA);
	if (cosA) sign *= Math_Sign(cosA);
	return sign * MATH_POS_INF;
}

double Math_FastLog(double x) {
	/* x = 2^exp * mantissa */
	/* so log(x) = log(2^exp) + log(mantissa) */
	/* so log(x) = exp*log(2) + log(mantissa) */

	/* now need to work out log(mantissa) */
}

double Math_FastExp(double x) {
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
	TimeMS now = DateTime_CurrentUTC_MS();
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

float Random_Float(Random* seed) {
	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	Int32 raw = (Int32)(*seed >> (48 - 24));
	return raw / ((float)(1 << 24));
}

