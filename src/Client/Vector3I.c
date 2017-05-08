#include "Vector3I.h"
#include "Funcs.h"
#include "ExtMath.h"

Vector3I Vector3I_Create1(Int32 value) {
	Vector3I v; v.X = value; v.Y = value; v.Z = value; return v;
}

Vector3I Vector3I_Create3(Int32 x, Int32 y, Int32 z) {
	Vector3I v; v.X = x; v.Y = y; v.Z = z; return v;
}


void Vector3I_Add(Vector3I* a, Vector3I* b, Vector3I* result) {
	result->X = a->X + b->X;
	result->Y = a->Y + b->Y;
	result->Z = a->Z + b->Z;
}

void Vector3I_Subtract(Vector3I* a, Vector3I* b, Vector3I* result) {
	result->X = a->X - b->X;
	result->Y = a->Y - b->Y;
	result->Z = a->Z - b->Z;
}

void Vector3I_Multiply1(Vector3I* a, Int32 scale, Vector3I* result) {
	result->X = a->X * scale;
	result->Y = a->Y * scale;
	result->Z = a->Z * scale;
}

void Vector3I_Negate(Vector3I* a, Vector3I* result) {
	result->X = -a->X;
	result->Y = -a->Y;
	result->Z = -a->Z;
}


bool Vector3I_Equals(Vector3I* a, Vector3I* b) {
	return a->X == b->X && a->Y == b->Y && a->Z == b->Z;
}

void Vector3I_Floor(Vector3* a, Vector3I* result) {
	result->X = Math_Floor(a->X);
	result->Y = Math_Floor(a->Y);
	result->Z = Math_Floor(a->Z);
}

void Vector3I_Min(Vector3I* a, Vector3I* b, Vector3I* result) {
	result->X = min(a->X, b->X);
	result->Y = min(a->Y, b->Y);
	result->Z = min(a->Z, b->Z);
}

void Vector3I_Max(Vector3I* a, Vector3I* b, Vector3I* result) {
	result->X = max(a->X, b->X);
	result->Y = max(a->Y, b->Y);
	result->Z = max(a->Z, b->Z);
}