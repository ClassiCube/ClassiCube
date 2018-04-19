#ifndef CC_MATH_H
#define CC_MATH_H
#include "Typedefs.h"
/* Simple math functions and constants.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define MATH_PI 3.1415926535897931f
#define MATH_DEG2RAD (MATH_PI / 180.0f)
#define MATH_RAD2DEG (180.0f / MATH_PI)
#define MATH_LARGENUM 1000000000.0f
#define MATH_POS_INF ((Real32)(1e+300 * 1e+300))

#define Math_Deg2Packed(x) ((UInt8)((x) * 256.0f / 360.0f))
#define Math_Packed2Deg(x) ((x) * 360.0f / 256.0f)

Real32 Math_AbsF(Real32 x);
Real32 Math_SinF(Real32 x);
Real32 Math_CosF(Real32 x);
Real32 Math_TanF(Real32 x);
Real32 Math_SqrtF(Real32 x);
Real32 Math_ModF(Real32 x, Real32 y);

Real64 Math_Log(Real64 x);
Real64 Math_Exp(Real64 x);
Real64 Math_Asin(Real64 x);
Real64 Math_Atan2(Real64 y, Real64 x);

Int32 Math_AbsI(Int32 x);
Int32 Math_Floor(Real32 value);
Int32 Math_Ceil(Real32 value);
Int32 Math_Log2(Int32 value);
Int32 Math_CeilDiv(Int32 a, Int32 b);
Int32 Math_Sign(Real32 value);

Real32 Math_Lerp(Real32 a, Real32 b, Real32 t);
/* Linearly interpolates between a given angle range, adjusting if necessary. */
Real32 Math_LerpAngle(Real32 leftAngle, Real32 rightAngle, Real32 t);

Int32 Math_NextPowOf2(Int32 value);
bool Math_IsPowOf2(Int32 value);

/* Returns the number of vertices needed to subdivide a quad. */
#define Math_CountVertices(axis1Len, axis2Len, axisSize) (Math_CeilDiv(axis1Len, axisSize) * Math_CeilDiv(axis2Len, axisSize) * 4)

#define Math_Clamp(value, min, max)\
value = value < (min) ? (min) : value;\
value = value > (max) ? (max) : value;
#endif