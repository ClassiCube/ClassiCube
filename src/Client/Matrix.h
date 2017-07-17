#ifndef CS_MATRIX_H
#define CS_MATRIX_H
#include "Typedefs.h"
/* Represents a 4 by 4 matrix.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* forward declaration */
typedef struct Vector3_ Vector3;

typedef struct Vector4_ { Real32 X, Y, Z, W; } Vector4;

typedef struct Matrix_ {
	/* Top row of the matrix */
	Vector4 Row0;
	/* 2nd row of the matrix */
	Vector4 Row1;
	/* 3rd row of the matrix */
	Vector4 Row2;
	/* Bottom row of the matrix */
	Vector4 Row3;
} Matrix;

/* Identity matrix. */
extern Matrix Matrix_Identity;

/* Transformation matrix representing rotation angle radians around X axis. */
void Matrix_RotateX(Matrix* result, Real32 angle);

/* Transformation matrix representing rotation angle radians around Y axis. */
void Matrix_RotateY(Matrix* result, Real32 angle);

/* Transformation matrix representing rotation angle radians around Z axis. */
void Matrix_RotateZ(Matrix* result, Real32 angle);

/* Transformation matrix representing translation of given coordinates. */
void Matrix_Translate(Matrix* result, Real32 x, Real32 y, Real32 z);

/* Transformation matrix representing scaling of given axes. */
void Matrix_Scale(Matrix* result, Real32 x, Real32 y, Real32 z);


/* Multiplies a matrix by another.*/
#define Matrix_MulBy(dst, right) Matrix_Mul(dst, dst, right)

/* Multiplies two matrices.*/
void Matrix_Mul(Matrix* result, Matrix* left, Matrix* right);


/* Transformation matrix representing orthographic projection. */
void Matrix_Orthographic(Matrix* result, Real32 width, Real32 height, Real32 zNear, Real32 zFar);

/* Transformation matrix representing orthographic projection. */
void Matrix_OrthographicOffCenter(Matrix* result, Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar);

/* Transformation matrix representing perspective projection. */
void Matrix_PerspectiveFieldOfView(Matrix* result, Real32 fovy, Real32 aspect, Real32 zNear, Real32 zFar);

/* Transformation matrix representing perspective projection. */
void Matrix_PerspectiveOffCenter(Matrix* result, Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar);

/* Transformation matrix representing camera look at. */
void Matrix_LookAt(Matrix* result, Vector3 eye, Vector3 target, Vector3 up);
#endif