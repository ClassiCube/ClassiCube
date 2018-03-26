#ifndef CC_MATRIX_H
#define CC_MATRIX_H
#include "Typedefs.h"
/* Represents a 4 by 4 matrix.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* forward declaration */
typedef struct Vector3_ Vector3;

typedef struct Vector4_ { Real32 X, Y, Z, W; } Vector4;
typedef struct Matrix_ { Vector4 Row0, Row1, Row2, Row3; } Matrix;
extern Matrix Matrix_Identity;

void Matrix_RotateX(Matrix* result, Real32 angle);
void Matrix_RotateY(Matrix* result, Real32 angle);
void Matrix_RotateZ(Matrix* result, Real32 angle);
void Matrix_Translate(Matrix* result, Real32 x, Real32 y, Real32 z);
void Matrix_Scale(Matrix* result, Real32 x, Real32 y, Real32 z);

#define Matrix_MulBy(dst, right) Matrix_Mul(dst, dst, right)
void Matrix_Mul(Matrix* result, Matrix* left, Matrix* right);

void Matrix_Orthographic(Matrix* result, Real32 width, Real32 height, Real32 zNear, Real32 zFar);
void Matrix_OrthographicOffCenter(Matrix* result, Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar);
void Matrix_PerspectiveFieldOfView(Matrix* result, Real32 fovy, Real32 aspect, Real32 zNear, Real32 zFar);
void Matrix_PerspectiveOffCenter(Matrix* result, Real32 left, Real32 right, Real32 bottom, Real32 top, Real32 zNear, Real32 zFar);
void Matrix_LookAt(Matrix* result, Vector3 eye, Vector3 target, Vector3 up);
#endif