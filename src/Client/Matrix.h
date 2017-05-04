#ifndef CS_MATRIX_H
#define CS_MATRIX_H
#include "Typedefs.h"
/* Represents a 4 by 4 matrix.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

// TODO: not nice we have to put it here
typedef struct Vector4 { Real32 X, Y, Z, W; } Vector4;

/* Constructs a vector from the given coordinates. */
Vector4 Vector4_Create4(Real32 x, Real32 y, Real32 z, Real32 w);


#define Vector4_UnitX Vector4_Create4(1.0f, 0.0f, 0.0f, 0.0f)
#define Vector4_UnitY Vector4_Create4(0.0f, 1.0f, 0.0f, 0.0f)
#define Vector4_UnitZ Vector4_Create4(0.0f, 0.0f, 1.0f, 0.0f)
#define Vector4_UnitW Vector4_Create4(0.0f, 0.0f, 1.0f, 0.0f)
#define Vector4_Zero Vector4_Create4(0.0f, 0.0f, 0.0f, 0.0f)
#define Vector4_One Vector4_Create4(1.0f, 1.0f, 1.0f, 1.0f)


typedef struct Matrix {
	/* Top row of the matrix */
	Vector4 Row0;
	/* 2nd row of the matrix */
	Vector4 Row1;
	/* 3rd row of the matrix */
	Vector4 Row2;
	/* Bottom row of the matrix */
	Vector4 Row3;
} Matrix;
#endif