#ifndef CS_MATRIX_H
#define CS_MATRIX_H
#include "Typedefs.h"
#include "Vectors.h"
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

/* Identity matrix. */
Matrix Matrix_Identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};


/* Transformation matrix representing rotation angle radians around X axis. */
void Matrix_RotateX(Real32 angle, Matrix* result);

/* Transformation matrix representing rotation angle radians around Y axis. */
void Matrix_RotateY(Real32 angle, Matrix* result);

/* Transformation matrix representing rotation angle radians around Z axis. */
void Matrix_RotateZ(Real32 angle, Matrix* result);

/* Transformation matrix representing translation of given coordinates. */
void Matrix_Translate(Real32 x, Real32 y, Real32 z, Matrix* result);

/* Transformation matrix representing scaling of given axes. */
void Matrix_Scale(Real32 x, Real32 y, Real32 z, Matrix* result);

/* Multiplies two matrices.*/
void Matrix_Mul(Matrix* left, Matrix* right, Matrix* result);


/* Transformation matrix representing orthographic projection. */
void Matrix_Orthographic(Real32 width, Real32 height, Real32 zNear, Real32 zFar, Matrix* result);

/* Transformation matrix representing orthographic projection. */
void Matrix_OrthographicOffCenter(Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar, Matrix* result);

/* Transformation matrix representing perspective projection. */
void Matrix_PerspectiveFieldOfView(Real32 fovy, Real32 aspect, Real32 zNear, Real32 zFar, Matrix* result);

/* Transformation matrix representing perspective projection. */
void Matrix_PerspectiveOffCenter(Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar, Matrix* result);

/* Transformation matrix representing camera look at. */
void Matrix_LookAt(Vector3 eye, Vector3 target, Vector3 up, Matrix* result);
#endif