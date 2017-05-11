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

#define Math_Abs(x) abs(x)

#define Math_Sin(x) sinf(x)

#define Math_Cos(x) cosf(x)

#define Math_Tan(x) tanf(x)

#define Math_Sqrt(x) sqrtf(x)

/* Integer floor of a floating-point value. */
Int32 Math_Floor(Real32 value);
#endif