#ifndef CC_VECTORS_H
#define CC_VECTORS_H
#include "Core.h"
#include "Constants.h"
/* Represents 2 and 3 component vectors, and 4 x 4 matrix.
   Frustum culling sourced from http://www.crownandcutlass.com/features/technicaldetails/frustum.html
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* 2 component vector (2D vector) */
typedef struct Vector2_ { float X, Y; } Vector2;
/* 3 component vector (3D vector) */
typedef struct Vector3_ { float X, Y, Z; } Vector3;
/* 3 component vector (3D integer vector) */
typedef struct Vector3I_ { int X, Y, Z; } Vector3I;
/* 4 component vector */
struct Vector4 { float X, Y, Z, W; };
/* 4x4 matrix. (for vertex transformations) */
struct Matrix { struct Vector4 Row0, Row1, Row2, Row3; };

/* Identity matrix. (A * Identity = A) */
extern const struct Matrix Matrix_Identity;
#define VECTOR3_CONST(x, y, z) { x, y, z };

/* Returns a vector with all components 0. */
static CC_INLINE Vector3 Vector3_Zero(void) {
	Vector3 v = { 0, 0, 0 }; return v;
}
/* Returns a vector with all components 1. */
static CC_INLINE Vector3 Vector3_One(void) {
	Vector3 v = { 1, 1, 1}; return v;
}
/* Returns a vector with all components set to Int32_MaxValue. */
static CC_INLINE Vector3I Vector3I_MaxValue(void) {
	Vector3I v = { Int32_MaxValue, Int32_MaxValue, Int32_MaxValue }; return v;
}
static CC_INLINE Vector3 Vector3_BigPos(void) {
	Vector3 v = { 1e25f, 1e25f, 1e25f }; return v;
}

static CC_INLINE Vector3 Vector3_Create1(float value) {
	Vector3 v; v.X = value; v.Y = value; v.Z = value; return v;
}
static CC_INLINE Vector3 Vector3_Create3(float x, float y, float z) {
	Vector3 v; v.X = x; v.Y = y; v.Z = z; return v;
}
/* Returns the squared length of the vector. */
/* Squared length can be used for comparison, to avoid a costly sqrt() */
/* However, you must sqrt() this when adding lengths. */
static CC_INLINE float Vector3_LengthSquared(const Vector3* v) {
	return v->X * v->X + v->Y * v->Y + v->Z * v->Z;
}
/* Adds components of two vectors together. */
static CC_INLINE void Vector3_Add(Vector3* result, const Vector3* a, const Vector3* b) {
	result->X = a->X + b->X; result->Y = a->Y + b->Y; result->Z = a->Z + b->Z;
}
/* Adds a value to each component of a vector. */
static CC_INLINE void Vector3_Add1(Vector3* result, const Vector3* a, float b) {
	result->X = a->X + b; result->Y = a->Y + b; result->Z = a->Z + b;
}
/* Subtracts components of two vectors from each other. */
static CC_INLINE void Vector3_Sub(Vector3* result, const Vector3* a, const Vector3* b) {
	result->X = a->X - b->X; result->Y = a->Y - b->Y; result->Z = a->Z - b->Z;
}
/* Mulitplies each component of a vector by a value. */
static CC_INLINE void Vector3_Mul1(Vector3* result, const Vector3* a, float b) {
	result->X = a->X * b; result->Y = a->Y * b; result->Z = a->Z * b;
}
/* Multiplies components of two vectors together. */
static CC_INLINE void Vector3_Mul3(Vector3* result, const Vector3* a, const Vector3* b) {
	result->X = a->X * b->X; result->Y = a->Y * b->Y; result->Z = a->Z * b->Z;
}
/* Negats the components of a vector. */
static CC_INLINE void Vector3_Negate(Vector3* result, Vector3* a) {
	result->X = -a->X; result->Y = -a->Y; result->Z = -a->Z;
}

#define Vector3_AddBy(dst, value) Vector3_Add(dst, dst, value)
#define Vector3_SubBy(dst, value) Vector3_Sub(dst, dst, value)
#define Vector3_Mul1By(dst, value) Vector3_Mul1(dst, dst, value)
#define Vector3_Mul3By(dst, value) Vector3_Mul3(dst, dst, value)

/* Linearly interpolates components of two vectors. */
void Vector3_Lerp(Vector3* result, Vector3* a, Vector3* b, float blend);
/* Scales all components of a vector to lie in [-1, 1] */
void Vector3_Normalize(Vector3* result, Vector3* a);

/* Transforms a vector by the given matrix. */
void Vector3_Transform(Vector3* result, Vector3* a, struct Matrix* mat);
/* Same as Vector3_Transform, but faster since X and Z are assumed as 0. */
void Vector3_TransformY(Vector3* result, float y, struct Matrix* mat);

Vector3 Vector3_RotateX(Vector3 v, float angle);
Vector3 Vector3_RotateY(Vector3 v, float angle);
Vector3 Vector3_RotateY3(float x, float y, float z, float angle);
Vector3 Vector3_RotateZ(Vector3 v, float angle);

/* Whether all of the components of the two vectors are equal. */
static CC_INLINE bool Vector3_Equals(const Vector3* a, const Vector3* b) {
	return a->X == b->X && a->Y == b->Y && a->Z == b->Z;
}
/* Whether any of the components of the two vectors differ. */
static CC_INLINE bool Vector3_NotEquals(const Vector3*  a, const Vector3* b) {
	return a->X != b->X || a->Y != b->Y || a->Z != b->Z;
}
/* Whether all of the components of the two vectors are equal. */
static CC_INLINE bool Vector3I_Equals(const Vector3I* a, const Vector3I* b) {
	return a->X == b->X && a->Y == b->Y && a->Z == b->Z;
}
/* Whether any of the components of the two vectors differ. */
static CC_INLINE bool Vector3I_NotEquals(const Vector3I* a, const Vector3I* b) {
	return a->X != b->X || a->Y != b->Y || a->Z != b->Z;
}

void Vector3I_Floor(Vector3I* result, Vector3* a);
void Vector3I_ToVector3(Vector3* result, Vector3I* a);
void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b);
void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b);

/* Returns a normalised vector facing in the direction described by the given yaw and pitch. */
Vector3 Vector3_GetDirVector(float yawRad, float pitchRad);
/* Returns the yaw and pitch of the given direction vector.
NOTE: This is not an identity function. Returned pitch is always within [-90, 90] degrees.*/
/*void Vector3_GetHeading(Vector3 dir, float* yawRad, float* pitchRad);*/

/* Returns a matrix representing a counter-clockwise rotation around X axis. */
CC_API void Matrix_RotateX(struct Matrix* result, float angle);
/* Returns a matrix representing a counter-clockwise rotation around Y axis. */
CC_API void Matrix_RotateY(struct Matrix* result, float angle);
/* Returns a matrix representing a counter-clockwise rotation around Z axis. */
CC_API void Matrix_RotateZ(struct Matrix* result, float angle);
/* Returns a matrix representing a translation to the given coordinates. */
CC_API void Matrix_Translate(struct Matrix* result, float x, float y, float z);
/* Returns a matrix representing a scaling by the given factors. */
CC_API void Matrix_Scale(struct Matrix* result, float x, float y, float z);

#define Matrix_MulBy(dst, right) Matrix_Mul(dst, dst, right)
/* Multiplies two matrices together. */
/* NOTE: result can be the same pointer as left or right. */
CC_API void Matrix_Mul(struct Matrix* result, const struct Matrix* left, const struct Matrix* right);

void Matrix_Orthographic(struct Matrix* result, float width, float height, float zNear, float zFar);
void Matrix_OrthographicOffCenter(struct Matrix* result, float left, float right, float bottom, float top, float zNear, float zFar);
void Matrix_PerspectiveFieldOfView(struct Matrix* result, float fovy, float aspect, float zNear, float zFar);
void Matrix_PerspectiveOffCenter(struct Matrix* result, float left, float right, float bottom, float top, float zNear, float zFar);
void Matrix_LookRot(struct Matrix* result, Vector3 pos, Vector2 rot);

bool FrustumCulling_SphereInFrustum(float x, float y, float z, float radius);
void FrustumCulling_CalcFrustumEquations(struct Matrix* projection, struct Matrix* modelView);
#endif
