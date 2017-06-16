#ifndef CS_MATH_H
#define CS_MATH_H
#include <math.h>
#include "Typedefs.h"
/* Simple math functions.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define MATH_PI 3.1415926535897931f

#define MATH_DEG2RAD (MATH_PI / 180.0f)

#define MATH_RAD2DEG (180.0f / MATH_PI)

#define Math_AbsF(x) fabsf(x)

#define Math_AbsI(x) abs(x)

#define Math_LogE(x) logf(x)

#define Math_PowE(x) expf(x)

#define Math_Sin(x) sinf(x)

#define Math_Cos(x) cosf(x)

#define Math_Tan(x) tanf(x)

#define Math_Sqrt(x) sqrtf(x)

#define Math_Mod(x, y) fmodf(x, y)

/* Integer floor of a floating-point value. */
Int32 Math_Floor(Real32 value);

/* Log base 2 of given value. */
Int32 Math_Log2(Int32 value);

/* Performs rounding upwards integer division.*/
Int32 Math_CeilDiv(Int32 a, Int32 b);

/* Performs linear interpolation between two values. */
Real32 Math_Lerp(Real32 a, Real32 b, Real32 t);

/* Returns the next highest power of 2 that is greater or equal to the given value. */
Int32 Math_NextPowOf2(Int32 value);

/* Returns whether the given value is a power of 2. */
bool Math_IsPowOf2(Int32 value);

/* Returns the number of vertices needed to subdivide a quad. */
#define Math_CountVertices(axis1Len, axis2Len, axisSize) (Math_CeilDiv(axis1Len, axisSize) * Math_CeilDiv(axis2Len, axisSize) * 4)

#define Math_AdjViewDist(value) ((Int32)(1.4142135f * (value)))

#define Math_Clamp(value, min, max)\
value = value < (min) ? (min) : value;\
value = value > (max) ? (max) : value;
#endif