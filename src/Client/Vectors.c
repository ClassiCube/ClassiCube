#include "Vectors.h"
#include "ExtMath.h"
#include "Funcs.h"

Vector2 Vector2_Create2(Real32 x, Real32 y) {
	Vector2 v; v.X = x; v.Y = y; return v;
}

Vector3 Vector3_Create1(Real32 value) {
	Vector3 v; v.X = value; v.Y = value; v.Z = value; return v;
}

Vector3 Vector3_Create3(Real32 x, Real32 y, Real32 z) {
	Vector3 v; v.X = x; v.Y = y; v.Z = z; return v;
}

Vector3I Vector3I_Create1(Int32 value) {
	Vector3I v; v.X = value; v.Y = value; v.Z = value; return v;
}

Vector3I Vector3I_Create3(Int32 x, Int32 y, Int32 z) {
	Vector3I v; v.X = x; v.Y = y; v.Z = z; return v;
}

Real32 Vector3_Length(Vector3* v) {
	Real32 lenSquared = v->X * v->X + v->Y * v->Y + v->Z * v->Z;
	return Math_Sqrt(lenSquared);
}

Real32 Vector3_LengthSquared(Vector3* v) {
	return v->X * v->X + v->Y * v->Y + v->Z * v->Z;
}

#define Vec3_Add(result, a, b)\
result->X = a->X + b->X;\
result->Y = a->Y + b->Y;\
result->Z = a->Z + b->Z;

void Vector3_Add(Vector3* result, Vector3* a, Vector3* b) { Vec3_Add(result, a, b); }
void Vector3I_Add(Vector3I* result, Vector3I* a, Vector3I* b) { Vec3_Add(result, a, b); }
void Vector3_Add1(Vector3* result, Vector3* a, Real32 b) {
	result->X = a->X + b;
	result->Y = a->Y + b;
	result->Z = a->Z + b;
}

#define Vec3_Sub(result, a, b)\
result->X = a->X - b->X;\
result->Y = a->Y - b->Y;\
result->Z = a->Z - b->Z;

void Vector3_Subtract(Vector3* result, Vector3* a, Vector3* b) { Vec3_Sub(result, a, b); }
void Vector3I_Subtract(Vector3I* result, Vector3I* a, Vector3I* b) { Vec3_Sub(result, a, b); }

#define Vec3_Mul1(result, a, scale)\
result->X = a->X * scale;\
result->Y = a->Y * scale;\
result->Z = a->Z * scale;

void Vector3_Multiply1(Vector3* result, Vector3* a, Real32 scale) { Vec3_Mul1(result, a, scale); }
void Vector3I_Multiply1(Vector3I* result, Vector3I* a, Int32 scale) { Vec3_Mul1(result, a, scale); }
void Vector3_Multiply3(Vector3* result, Vector3* a, Vector3* scale) {
	result->X = a->X * scale->X;
	result->Y = a->Y * scale->Y;
	result->Z = a->Z * scale->Z;
}

void Vector3_Divide1(Vector3* result, Vector3* a, Real32 scale) {
	Vector3_Multiply1(result, a, 1.0f / scale);
}

void Vector3_Divide3(Vector3* result, Vector3* a, Vector3* scale) {
	result->X = a->X / scale->X;
	result->Y = a->Y / scale->Y;
	result->Z = a->Z / scale->Z;
}

#define Vec3_Negate(result, a)\
result->X = -a->X;\
result->Y = -a->Y;\
result->Z = -a->Z;

void Vector3_Negate(Vector3* result, Vector3* a) { Vec3_Negate(result, a); }
void Vector3I_Negate(Vector3I* result, Vector3I* a) { Vec3_Negate(result, a); }


void Vector3_Lerp(Vector3* result, Vector3* a, Vector3* b, Real32 blend) {
	result->X = blend * (b->X - a->X) + a->X;
	result->Y = blend * (b->Y - a->Y) + a->Y;
	result->Z = blend * (b->Z - a->Z) + a->Z;
}

Real32 Vector3_Dot(Vector3* a, Vector3* b) {
	return a->X * b->X + a->Y * b->Y + a->Z * b->Z;
}

void Vector3_Cross(Vector3* result, Vector3* a, Vector3* b) {
	/* a or b could be pointing to result - can't directly assign X/Y/Z therefore */
	Real32 x = a->Y * b->Z - a->Z * b->Y;
	Real32 y = a->Z * b->X - a->X * b->Z;
	Real32 z = a->X * b->Y - a->Y * b->X;
	result->X = x; result->Y = y; result->Z = z;
}

void Vector3_Normalize(Vector3* result, Vector3* a) {
	Real32 scale = 1.0f / Vector3_Length(a);
	result->X = a->X * scale;
	result->Y = a->Y * scale;
	result->Z = a->Z * scale;
}

void Vector3_Transform(Vector3* result, Vector3* a, Matrix* mat) {
	/* a could be pointing to result - can't directly assign X/Y/Z therefore */
	Real32 x = a->X * mat->Row0.X + a->Y * mat->Row1.X + a->Z * mat->Row2.X + mat->Row3.X;
	Real32 y = a->X * mat->Row0.Y + a->Y * mat->Row1.Y + a->Z * mat->Row2.Y + mat->Row3.Y;
	Real32 z = a->X * mat->Row0.Z + a->Y * mat->Row1.Z + a->Z * mat->Row2.Z + mat->Row3.Z;
	result->X = x; result->Y = y; result->Z = z;
}

void Vector3_TransformX(Vector3* result, Real32 x, Matrix* mat) {
	result->X = x * mat->Row0.X + mat->Row3.X;
	result->Y = x * mat->Row0.Y + mat->Row3.Y;
	result->Z = x * mat->Row0.Z + mat->Row3.Z;
}

void Vector3_TransformY(Vector3* result, Real32 y, Matrix* mat) {
	result->X = y * mat->Row1.X + mat->Row3.X;
	result->Y = y * mat->Row1.Y + mat->Row3.Y;
	result->Z = y * mat->Row1.Z + mat->Row3.Z;
}

void Vector3_TransformZ(Vector3* result, Real32 z, Matrix* mat) {
	result->X = z * mat->Row2.X + mat->Row3.X;
	result->Y = z * mat->Row2.Y + mat->Row3.Y;
	result->Z = z * mat->Row2.Z + mat->Row3.Z;
}

Vector3 Vector3_RotateX(Vector3 v, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(v.X, cosA * v.Y + sinA * v.Z, -sinA * v.Y + cosA * v.Z);
}

Vector3 Vector3_RotateY(Vector3 v, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(cosA * v.X - sinA * v.Z, v.Y, sinA * v.X + cosA * v.Z);
}

Vector3 Vector3_RotateY3(Real32 x, Real32 y, Real32 z, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(cosA * x - sinA * z, y, sinA * x + cosA * z);
}

Vector3 Vector3_RotateZ(Vector3 v, Real32 angle) {
	Real32 cosA = Math_Cos(angle), sinA = Math_Sin(angle);
	return Vector3_Create3(cosA * v.X + sinA * v.Y, -sinA * v.X + cosA * v.Y, v.Z);
}


#define Vec3_EQ(a, b) a->X == b->X && a->Y == b->Y && a->Z == b->Z
#define Vec3_NE(a, b) a->X != b->X || a->Y != b->Y || a->Z != b->Z

bool Vector3_Equals(Vector3* a, Vector3* b) { return Vec3_EQ(a, b); }
bool Vector3_NotEquals(Vector3* a, Vector3* b) { return Vec3_NE(a, b); }
bool Vector3I_Equals(Vector3I* a, Vector3I* b) { return Vec3_EQ(a, b); }
bool Vector3I_NotEquals(Vector3I* a, Vector3I* b) { return Vec3_NE(a, b); }


void Vector3I_Floor(Vector3I* result, Vector3* a) {
	result->X = Math_Floor(a->X);
	result->Y = Math_Floor(a->Y);
	result->Z = Math_Floor(a->Z);
}

void Vector3I_ToVector3(Vector3* result, Vector3I* a) {
	result->X = (Real32)a->X;
	result->Y = (Real32)a->Y;
	result->Z = (Real32)a->Z;
}

void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b) {
	result->X = min(a->X, b->X);
	result->Y = min(a->Y, b->Y);
	result->Z = min(a->Z, b->Z);
}

void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b) {
	result->X = max(a->X, b->X);
	result->Y = max(a->Y, b->Y);
	result->Z = max(a->Z, b->Z);
}


Vector3 Vector3_GetDirVector(Real32 yawRad, Real32 pitchRad) {
	Real32 x = -Math_Cos(pitchRad) * -Math_Sin(yawRad);
	Real32 y = -Math_Sin(pitchRad);
	Real32 z = -Math_Cos(pitchRad) * Math_Cos(yawRad);
	return Vector3_Create3(x, y, z);
}

void Vector3_GetHeading(Vector3 dir, Real32* yaw, Real32* pitch) {
	*pitch = Math_Asin(-dir.Y);
	*yaw = Math_Atan2(dir.X, -dir.Z);
}