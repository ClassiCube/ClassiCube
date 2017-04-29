#include "Vector3.h"

Vector3 Vector3_Create3(Real32 x, Real32 y, Real32 z) {
	Vector3 v; v.X = x; v.Y = y; v.Z = z; return v;
}

Vector3 Vector3_Create1(Real32 value) {
	Vector3 v; v.X = value; v.Y = value; v.Z = value; return v;
}

/* Returns the length of the given vector. */
Real32 Vector3_Length(Vector3* v) {
	Real32 lenSquared = v->X * v->X + v->Y * v->Y + v->Z * v->Z;
	return Math_Sqrt(lenSquared);
}

/* Returns the squared length of the given vector. */
Real32 Vector3_LengthSquared(Vector3* v) {
	return v->X * v->X + v->Y * v->Y + v->Z * v->Z;
}


void Vector3_Add(Vector3* a, Vector3* b, Vector3* result) {
	result->X = a->X + b->X;
	result->Y = a->Y + b->Y;
	result->Z = a->Z + b->Z;
}

void Vector3_Subtract(Vector3* a, Vector3* b, Vector3* result) {
	result->X = a->X - b->X;
	result->Y = a->Y - b->Y;
	result->Z = a->Z - b->Z;
}

void Vector3_Multiply1(Vector3* a, Real32 scale, Vector3* result) {
	result->X = a->X * scale;
	result->Y = a->Y * scale;
	result->Z = a->Z * scale;
}

void Vector3_Multiply3(Vector3* a, Vector3* scale, Vector3* result) {
	result->X = a->X * scale->X;
	result->Y = a->Y * scale->Y;
	result->Z = a->Z * scale->Z;
}

void Vector3_Divide1(Vector3* a, Real32 scale, Vector3* result) {
	Vector3_Multiply1(a, 1.0f / scale, result);
}

void Vector3_Divide3(Vector3* a, Vector3* scale, Vector3* result) {
	result->X = a->X / scale->X;
	result->Y = a->Y / scale->Y;
	result->Z = a->Z / scale->Z;
}


void Vector3_Lerp(Vector3* a, Vector3* b, float blend, Vector3* result) {
	result->X = blend * (b->X - a->X) + a->X;
	result->Y = blend * (b->Y - a->Y) + a->Y;
	result->Z = blend * (b->Z - a->Z) + a->Z;
}

Real32 Vector3_Dot(Vector3* a, Vector3* b) {
	return a->X * b->X + a->Y * b->Y + a->Z * b->Z;
}

void Vector3_Cross(Vector3* a, Vector3* b, Vector3* result) {
	result->X = a->Y * b->Z - a->Z * b->Y;
	result->Y = a->Z * b->X - a->X * b->Z;
	result->Z = a->X * b->Y - a->Y * b->X;
}

void Vector3_Normalize(Vector3* a, Vector3* result) {
	float scale = 1.0f / Vector3_Length(a);
	result->X = a->X * scale;
	result->Y = a->Y * scale;
	result->Z = a->Z * scale;
}

void Transform(Vector3* a, Matrix* mat, Vector3* result) {
	result->X = a->X * mat->Row0.X + a->Y * mat->Row1.X + a->Z * mat->Row2.X + mat->Row3.X;
	result->Y = a->X * mat->Row0.Y + a->Y * mat->Row1.Y + a->Z * mat->Row2.Y + mat->Row3.Y;
	result->Z = a->X * mat->Row0.Z + a->Y * mat->Row1.Z + a->Z * mat->Row2.Z + mat->Row3.Z;
}

void TransformY(Real32 y, Matrix* mat, Vector3* result) {
	result->X = y * mat->Row1.X + mat->Row3.X;
	result->Y = y * mat->Row1.Y + mat->Row3.Y;
	result->Z = y * mat->Row1.Z + mat->Row3.Z;;
}


bool Vector3_Equals(Vector3* a, Vector3* b) {
	return a->X == b->X && a->Y == b->Y && a->Z == b->Z;
}

bool Vector3_NotEquals(Vector3* a, Vector3* b) {
	return a->X != b->X || a->Y != b->Y || a->Z != b->Z;
}