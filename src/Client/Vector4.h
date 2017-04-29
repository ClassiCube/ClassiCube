#ifndef CS_VECTOR4_H
#define CS_VECTOR4_H
#include "Typedefs.h"
/* Represents a 4 dimensional vector.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


typedef struct Vector4 {
	Real32 X, Y, Z, W;
} Vector4;


/* Constructs a vector from the given coordinates. */
Vector4 Vector4_Create4(Real32 x, Real32 y, Real32 z, Real32 w) {
	Vector4 v; v.X = x; v.Y = y; v.Z = z; v.W = w; return v;
}


#define Vector4_UnitX Vector4_Create4(1.0f, 0.0f, 0.0f, 0.0f)

#define Vector4_UnitY Vector4_Create4(0.0f, 1.0f, 0.0f, 0.0f)

#define Vector4_UnitZ Vector4_Create4(0.0f, 0.0f, 1.0f, 0.0f)

#define Vector4_UnitW Vector4_Create4(0.0f, 0.0f, 1.0f, 0.0f)

#define Vector4_Zero Vector4_Create4(0.0f, 0.0f, 0.0f, 0.0f)

#define Vector4_One Vector4_Create4(1.0f, 1.0f, 1.0f, 1.0f)
#endif