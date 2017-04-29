#ifndef CS_MATRIX_H
#define CS_MATRIX_H
#include "Typedefs.h"
#include "Vectors.h"
/* Represents a 4 by 4 matrix.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

// TODO: not nice we have to put it here
typedef struct Vector4 { Real32 X, Y, Z, W; } Vector4;

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