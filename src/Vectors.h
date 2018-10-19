#ifndef CC_VECTORS_H
#define CC_VECTORS_H
#include "Core.h"
/* Represents 2, 3 dimensional vectors, and 4 x 4 matrix.
   Frustum culling sourced from http://www.crownandcutlass.com/features/technicaldetails/frustum.html
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Vector2_ { float X, Y; } Vector2;
typedef struct Vector3_ { float X, Y, Z; } Vector3;
typedef struct Vector3I_ { int X, Y, Z; } Vector3I;
struct Vector4 { float X, Y, Z, W; };
struct Matrix { struct Vector4 Row0, Row1, Row2, Row3; };
extern struct Matrix Matrix_Identity;

Vector3 Vector3_Create1(float value);
Vector3 Vector3_Create3(float x, float y, float z);
Vector3I Vector3I_MaxValue(void);
Vector3 Vector3_BigPos(void);
float Vector3_LengthSquared(const Vector3* v);

#define VECTOR3_CONST(x, y, z) { x, y, z };
#define Vector3_Zero  VECTOR3_CONST(0.0f, 0.0f, 0.0f)
#define Vector3_One   VECTOR3_CONST(1.0f, 1.0f, 1.0f)

void Vector3_Add(Vector3* result, Vector3* a, Vector3* b);
void Vector3_Add1(Vector3* result, Vector3* a, float b);
void Vector3_Sub(Vector3* result, Vector3* a, Vector3* b);
void Vector3_Mul1(Vector3* result, Vector3* a, float scale);
void Vector3_Mul3(Vector3* result, Vector3* a, Vector3* scale);
void Vector3_Negate(Vector3* result, Vector3* a);

#define Vector3_AddBy(dst, value) Vector3_Add(dst, dst, value)
#define Vector3_SubBy(dst, value) Vector3_Sub(dst, dst, value)
#define Vector3_Mul1By(dst, value) Vector3_Mul1(dst, dst, value)
#define Vector3_Mul3By(dst, value) Vector3_Mul3(dst, dst, value)

void Vector3_Lerp(Vector3* result, Vector3* a, Vector3* b, float blend);
void Vector3_Normalize(Vector3* result, Vector3* a);

void Vector3_Transform(Vector3* result, Vector3* a, struct Matrix* mat);
void Vector3_TransformY(Vector3* result, float y, struct Matrix* mat);

Vector3 Vector3_RotateX(Vector3 v, float angle);
Vector3 Vector3_RotateY(Vector3 v, float angle);
Vector3 Vector3_RotateY3(float x, float y, float z, float angle);
Vector3 Vector3_RotateZ(Vector3 v, float angle);

bool Vector3_Equals(const     Vector3*  a, const Vector3* b);
bool Vector3_NotEquals(const  Vector3*  a, const Vector3* b);
bool Vector3I_Equals(const    Vector3I* a, const Vector3I* b);
bool Vector3I_NotEquals(const Vector3I* a, const Vector3I* b);

void Vector3I_Floor(Vector3I* result, Vector3* a);
void Vector3I_ToVector3(Vector3* result, Vector3I* a);
void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b);
void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b);

/* Returns a normalised vector facing in the direction described by the given yaw and pitch. */
Vector3 Vector3_GetDirVector(float yawRad, float pitchRad);
/* Returns the yaw and pitch of the given direction vector.
NOTE: This is not an identity function. Returned pitch is always within [-90, 90] degrees.*/
/*void Vector3_GetHeading(Vector3 dir, float* yawRad, float* pitchRad);*/

void Matrix_RotateX(struct Matrix* result, float angle);
void Matrix_RotateY(struct Matrix* result, float angle);
void Matrix_RotateZ(struct Matrix* result, float angle);
void Matrix_Translate(struct Matrix* result, float x, float y, float z);
void Matrix_Scale(struct Matrix* result, float x, float y, float z);

#define Matrix_MulBy(dst, right) Matrix_Mul(dst, dst, right)
void Matrix_Mul(struct Matrix* result, struct Matrix* left, struct Matrix* right);

void Matrix_Orthographic(struct Matrix* result, float width, float height, float zNear, float zFar);
void Matrix_OrthographicOffCenter(struct Matrix* result, float left, float right, float bottom, float top, float zNear, float zFar);
void Matrix_PerspectiveFieldOfView(struct Matrix* result, float fovy, float aspect, float zNear, float zFar);
void Matrix_PerspectiveOffCenter(struct Matrix* result, float left, float right, float bottom, float top, float zNear, float zFar);
void Matrix_LookRot(struct Matrix* result, Vector3 pos, Vector2 rot);

bool FrustumCulling_SphereInFrustum(float x, float y, float z, float radius);
void FrustumCulling_CalcFrustumEquations(struct Matrix* projection, struct Matrix* modelView);
#endif
