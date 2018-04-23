#ifndef CC_VECTORS_H
#define CC_VECTORS_H
#include "Typedefs.h"
/* Represents 2, 3 dimensional vectors, and 4 x 4 matrix.
   Frustum culling sourced from http://www.crownandcutlass.com/features/technicaldetails/frustum.html
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Vector2_ { Real32 X, Y; } Vector2;
typedef struct Vector3_ { Real32 X, Y, Z; } Vector3;
typedef struct Vector3I_ { Int32 X, Y, Z; } Vector3I;
typedef struct Vector4_ { Real32 X, Y, Z, W; } Vector4;
typedef struct Matrix_ { Vector4 Row0, Row1, Row2, Row3; } Matrix;
extern Matrix Matrix_Identity;

Vector3 Vector3_Create1(Real32 value);
Vector3 Vector3_Create3(Real32 x, Real32 y, Real32 z);
Vector3I Vector3I_MaxValue(void);
Vector3 Vector3_BigPos(void);
Real32 Vector3_LengthSquared(Vector3* v);

#define VECTOR3_CONST1(val) { val, val, val };
#define VECTOR3_CONST(x, y, z) { x, y, z };
#define Vector3_UnitX VECTOR3_CONST(1.0f, 0.0f, 0.0f)
#define Vector3_UnitY VECTOR3_CONST(0.0f, 1.0f, 0.0f)
#define Vector3_UnitZ VECTOR3_CONST(0.0f, 0.0f, 1.0f)
#define Vector3_Zero  VECTOR3_CONST(0.0f, 0.0f, 0.0f)
#define Vector3_One   VECTOR3_CONST(1.0f, 1.0f, 1.0f)

void Vector3_Add(Vector3* result, Vector3* a, Vector3* b);
void Vector3_Add1(Vector3* result, Vector3* a, Real32 b);
void Vector3_Sub(Vector3* result, Vector3* a, Vector3* b);
void Vector3_Mul1(Vector3* result, Vector3* a, Real32 scale);
void Vector3_Mul3(Vector3* result, Vector3* a, Vector3* scale);
void Vector3_Negate(Vector3* result, Vector3* a);

#define Vector3_AddBy(dst, value) Vector3_Add(dst, dst, value)
#define Vector3_SubBy(dst, value) Vector3_Sub(dst, dst, value)
#define Vector3_Mul1By(dst, value) Vector3_Mul1(dst, dst, value)
#define Vector3_Mul3By(dst, value) Vector3_Mul3(dst, dst, value)

void Vector3_Lerp(Vector3* result, Vector3* a, Vector3* b, Real32 blend);
Real32 Vector3_Dot(Vector3* left, Vector3* right);
void Vector3_Cross(Vector3* result, Vector3* a, Vector3* b);
void Vector3_Normalize(Vector3* result, Vector3* a);

void Vector3_Transform(Vector3* result, Vector3* a, Matrix* mat);
void Vector3_TransformX(Vector3* result, Real32 x, Matrix* mat);
void Vector3_TransformY(Vector3* result, Real32 y, Matrix* mat);
void Vector3_TransformZ(Vector3* result, Real32 z, Matrix* mat);

Vector3 Vector3_RotateX(Vector3 v, Real32 angle);
Vector3 Vector3_RotateY(Vector3 v, Real32 angle);
Vector3 Vector3_RotateY3(Real32 x, Real32 y, Real32 z, Real32 angle);
Vector3 Vector3_RotateZ(Vector3 v, Real32 angle);

bool Vector3_Equals(Vector3* a, Vector3* b);
bool Vector3_NotEquals(Vector3* a, Vector3* b);
bool Vector3I_Equals(Vector3I* a, Vector3I* b);
bool Vector3I_NotEquals(Vector3I* a, Vector3I* b);

void Vector3I_Floor(Vector3I* result, Vector3* a);
void Vector3I_ToVector3(Vector3* result, Vector3I* a);
void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b);
void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b);

/* Returns a normalised vector that faces in the direction described by the given yaw and pitch. */
Vector3 Vector3_GetDirVector(Real32 yawRad, Real32 pitchRad);
/* Returns the yaw and pitch of the given direction vector.
NOTE: This is not an identity function. Returned pitch is always within [-90, 90] degrees.*/
void Vector3_GetHeading(Vector3 dir, Real32* yawRad, Real32* pitchRad);

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

bool FrustumCulling_SphereInFrustum(Real32 x, Real32 y, Real32 z, Real32 radius);
void FrustumCulling_CalcFrustumEquations(Matrix* projection, Matrix* modelView);
#endif