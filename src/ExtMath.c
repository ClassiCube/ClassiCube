#include "ExtMath.h"
#include "Platform.h"
#include "Utils.h"
#include <math.h>
/* For abs(x) function */
#include <stdlib.h>

/* TODO: Replace with own functions that don't rely on <math> */

#ifndef __GNUC__
float Math_AbsF(float x)  { return fabsf(x); /* MSVC intrinsic */ }
float Math_SqrtF(float x) { return sqrtf(x); /* MSVC intrinsic */ }
#endif

float Math_Mod1(float x)  { return x - (int)x; /* fmodf(x, 1); */ }
int   Math_AbsI(int x)    { return abs(x); /* MSVC intrinsic */ }

double Math_Sin(double x) { return sin(x); }
double Math_Cos(double x) { return cos(x); }

float Math_SinF(float x) { return (float)Math_Sin(x); }
float Math_CosF(float x) { return (float)Math_Cos(x); }
double Math_Atan2(double x, double y) { return atan2(y, x); }

int Math_Floor(float value) {
	int valueI = (int)value;
	return valueI > value ? valueI - 1 : valueI;
}

int Math_Ceil(float value) {
	int valueI = (int)value;
	return valueI < value ? valueI + 1 : valueI;
}

int Math_Log2(cc_uint32 value) {
	cc_uint32 r = 0;
	while (value >>= 1) r++;
	return r;
}

int Math_CeilDiv(int a, int b) {
	return a / b + (a % b != 0 ? 1 : 0);
}

int Math_Sign(float value) {
	if (value > 0.0f) return +1;
	if (value < 0.0f) return -1;
	return 0;
}

float Math_Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

float Math_ClampAngle(float degrees) {
	while (degrees >= 360.0f) degrees -= 360.0f;
	while (degrees < 0.0f)    degrees += 360.0f;
	return degrees;
}

float Math_LerpAngle(float leftAngle, float rightAngle, float t) {
	/* We have to cheat a bit for angles here */
	/* Consider 350* --> 0*, we only want to travel 10* */
	/* But without adjusting for this case, we would interpolate back the whole 350* degrees */
	cc_bool invertLeft  = leftAngle  > 270.0f && rightAngle < 90.0f;
	cc_bool invertRight = rightAngle > 270.0f && leftAngle  < 90.0f;
	if (invertLeft)  leftAngle  = leftAngle  - 360.0f;
	if (invertRight) rightAngle = rightAngle - 360.0f;

	return Math_Lerp(leftAngle, rightAngle, t);
}

int Math_NextPowOf2(int value) {
	int next = 1;
	while (value > next) { next <<= 1; }
	return next;
}

cc_bool Math_IsPowOf2(int value) {
	return value != 0 && (value & (value - 1)) == 0;
}

double Math_Log(double x) {
	/* x = 2^exp * mantissa */
	/*  so log(x) = log(2^exp) + log(mantissa) */
	/*  so log(x) = exp*log(2) + log(mantissa) */
	/* now need to work out log(mantissa) */

	return log(x);
}

double Math_Exp(double x) {
	/* let x = k*log(2) + f, where k is integer */
	/*  so exp(x) = exp(k*log(2)) * exp(f) */
	/*  so exp(x) = exp(log(2^k)) * exp(f) */
	/*  so exp(x) = 2^k           * exp(f) */
	/* now need to work out exp(f) */

	return exp(x);
}


/*########################################################################################################################*
*--------------------------------------------------Random number generator------------------------------------------------*
*#########################################################################################################################*/
#define RND_VALUE (0x5DEECE66DULL)
#define RND_MASK ((1ULL << 48) - 1)

void Random_SeedFromCurrentTime(RNGState* rnd) {
	TimeMS now = DateTime_CurrentUTC_MS();
	Random_Seed(rnd, (int)now);
}

void Random_Seed(RNGState* seed, int seedInit) {
	*seed = (seedInit ^ RND_VALUE) & RND_MASK;
}

int Random_Next(RNGState* seed, int n) {
	cc_int64 raw;
	int bits, val;

	if ((n & -n) == n) { /* i.e., n is a power of 2 */
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		raw   = (cc_int64)(*seed >> (48 - 31));
		return (int)((n * raw) >> 31);
	}

	do {
		*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
		bits  = (int)(*seed >> (48 - 31));
		val   = bits % n;
	} while (bits - val + (n - 1) < 0);
	return val;
}

float Random_Float(RNGState* seed) {
	int raw;

	*seed = (*seed * RND_VALUE + 0xBLL) & RND_MASK;
	raw   = (int)(*seed >> (48 - 24));
	return raw / ((float)(1 << 24));
}
