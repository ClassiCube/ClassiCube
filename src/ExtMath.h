#ifndef CC_MATH_H
#define CC_MATH_H
#include "Core.h"
/* Simple math functions and constants. Also implements a RNG algorithm, based on 
      Java's implementation from https://docs.oracle.com/javase/7/docs/api/java/util/Random.html
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define MATH_PI 3.1415926535897931f
#define MATH_DEG2RAD (MATH_PI / 180.0f)
#define MATH_RAD2DEG (180.0f / MATH_PI)
#define MATH_LARGENUM 1000000000.0f
#define MATH_POS_INF ((float)(1e+300 * 1e+300))

#define Math_Deg2Packed(x) ((uint8_t)((x) * 256.0f / 360.0f))
#define Math_Packed2Deg(x) ((x) * 360.0f / 256.0f)

float Math_AbsF(float x);
float Math_SqrtF(float x);
float Math_Mod1(float x);
Int32 Math_AbsI(Int32 x);

double Math_Sin(double x);
double Math_Cos(double x);
float Math_SinF(float x);
float Math_CosF(float x);

double Math_Log(double x);
double Math_Exp(double x);
double Math_FastTan(double x);

Int32 Math_Floor(float value);
Int32 Math_Ceil(float value);
Int32 Math_Log2(UInt32 value);
Int32 Math_CeilDiv(Int32 a, Int32 b);
Int32 Math_Sign(float value);

float Math_Lerp(float a, float b, float t);
/* Linearly interpolates between a given angle range, adjusting if necessary. */
float Math_LerpAngle(float leftAngle, float rightAngle, float t);

Int32 Math_NextPowOf2(Int32 value);
bool Math_IsPowOf2(Int32 value);
#define Math_Clamp(val, min, max) val = val < (min) ? (min) : val;  val = val > (max) ? (max) : val;

typedef uint64_t Random;
void Random_Init(Random* rnd, Int32 seed);
void Random_InitFromCurrentTime(Random* rnd);
void Random_SetSeed(Random* rnd, Int32 seed);
/* Returns integer from min inclusive to max exclusive */
Int32 Random_Range(Random* rnd, Int32 min, Int32 max);
/* Returns integer from 0 inclusive to n exclusive */
Int32 Random_Next(Random* rnd, Int32 n);
/* Returns real from 0 inclusive to 1 exclusive */
float Random_Float(Random* rnd);
#endif
