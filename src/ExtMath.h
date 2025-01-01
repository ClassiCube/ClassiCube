#ifndef CC_MATH_H
#define CC_MATH_H
#include "Core.h"
CC_BEGIN_HEADER

/* Simple math functions and constants. Also implements a RNG algorithm, based on 
      Java's implementation from https://docs.oracle.com/javase/7/docs/api/java/util/Random.html
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

#define MATH_PI 3.1415926535897931f
#define MATH_DEG2RAD (MATH_PI / 180.0f)
#define MATH_RAD2DEG (180.0f / MATH_PI)
#define MATH_LARGENUM 1000000000.0f

#define Math_Deg2Packed(x) ((cc_uint8)((x) * 256.0f / 360.0f))
#define Math_Packed2Deg(x) ((x) * 360.0f / 256.0f)

#if defined __GNUC__ && defined __APPLE__ && defined _ARCH_PPC
	/* fabsf is single intrinsic instructions in gcc/clang */
	/* (sqrtf doesn't seem to exist in 10.3 and earlier SDKs) */
	#define Math_AbsF(x)  __builtin_fabsf(x)
	#define Math_SqrtF(x) __builtin_sqrt(x)
#elif defined __GNUC__ && !defined CC_PLAT_PS1
	/* fabsf/sqrtf are single intrinsic instructions in gcc/clang */
	/* (sqrtf is only when -fno-math-errno though) */
	#define Math_AbsF(x)  __builtin_fabsf(x)
	#define Math_SqrtF(x) __builtin_sqrtf(x)
#else
	float Math_AbsF(float x);
	float Math_SqrtF(float x);
#endif

float Math_Mod1(float x);
int   Math_AbsI(int x);

static CC_INLINE float Math_SafeDiv(float a, float b) {
	if (Math_AbsF(b) < 0.000001f) return MATH_LARGENUM;
	return a / b;
}

CC_API double Math_Sin(double x);
CC_API double Math_Cos(double x);
CC_API float Math_SinF(float x);
CC_API float Math_CosF(float x);
/* Computes atan2(y, x), intended primarily for angle calculation*/
/*  Note that accuracy is only up to around 4 decimal places */
float Math_Atan2f(float x, float y);

/* Computes log2(x). Can also be used to approximate log_y(x). */
/*   e.g. for log3(x), use: log2(x)/log2(3) */
double Math_Log2(double x);
/* Computes 2^x. Can also be used to approximate y^x. */
/*   e.g. for 3^x, use: exp2(log2(3)*x) */
double Math_Exp2(double x);

int Math_Floor(float value);
int Math_Ceil(float value);
int Math_ilog2(cc_uint32 value);
int Math_CeilDiv(int a, int b);
int Math_Sign(float value);

/* Clamps the given angle so it lies between [0, 360) */
float Math_ClampAngle(float degrees);
/* Linearly interpolates between a and b */
float Math_Lerp(float a, float b, float t);
/* Linearly interpolates between a given angle range, adjusting if necessary. */
float Math_LerpAngle(float leftAngle, float rightAngle, float t);

int Math_NextPowOf2(int value);
cc_bool Math_IsPowOf2(int value);
#define Math_Clamp(val, min, max) val = val < (min) ? (min) : val;  val = val > (max) ? (max) : val;

typedef cc_uint64 RNGState;
/* Initialises RNG using seed from current UTC time. */
void Random_SeedFromCurrentTime(RNGState* rnd);
/* Initialised RNG using the given seed. */
CC_API  void Random_Seed(      RNGState* rnd, int seed);
typedef void (*FP_Random_Seed)(RNGState* rnd, int seed);

/* Returns integer from 0 inclusive to n exclusive */
CC_API  int Random_Next(      RNGState* rnd, int n);
typedef int (*FP_Random_Next)(RNGState* rnd, int n);
/* Returns real from 0 inclusive to 1 exclusive */
CC_API float Random_Float(RNGState* rnd);
/* Returns integer from min inclusive to max exclusive */
static CC_INLINE int Random_Range(RNGState* rnd, int min, int max) {
	return min + Random_Next(rnd, max - min);
}

CC_END_HEADER
#endif
