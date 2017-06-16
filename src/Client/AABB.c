#include "AABB.h"

void AABB_FromCoords6(AABB* result, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	result->Min = Vector3_Create3(x1, y1, z1);
	result->Max = Vector3_Create3(x2, y2, z2);
}

void AABB_FromCoords(AABB* result, Vector3* min, Vector3* max) {
	result->Min = *min;
	result->Max = *max;
}

void AABB_Make(AABB* result, Vector3* pos, Vector3* size) {
	result->Min.X = pos->X - size->X * 0.5f;
	result->Min.Y = pos->Y;
	result->Min.Z = pos->Z - size->Z * 0.5f;

	result->Max.X = pos->X + size->X * 0.5f;
	result->Min.Y = pos->Y + size->Y;
	result->Max.Z = pos->Z + size->Z * 0.5f;
}


void AABB_Offset(AABB* result, AABB* bb, Vector3* amount) {
	Vector3_Add(&result->Min, &bb->Min, amount);
	Vector3_Add(&result->Max, &bb->Max, amount);
}

bool AABB_Intersects(AABB* bb, AABB* other) {
	if (bb->Max.X >= other->Min.X && bb->Min.X <= other->Max.X) {
		if (bb->Max.Y < other->Min.Y || bb->Min.Y > other->Max.Y) {
			return false;
		}
		return bb->Max.Z >= other->Min.Z && bb->Min.Z <= other->Max.Z;
	}
	return false;
}

bool AABB_Contains(AABB* parent, AABB* child) {
	return 
		child->Min.X >= parent->Min.X && child->Min.Y >= parent->Min.Y && child->Min.Z >= parent->Min.Z &&
		child->Max.X <= parent->Max.X && child->Max.Y <= parent->Max.Y && child->Max.Z <= parent->Max.Z;
}

bool AABB_ContainsPoint(AABB* parent, Vector3* P) {
	return 
		P->X >= parent->Min.X && P->Y >= parent->Min.Y && P->Z >= parent->Min.Z &&
		P->X <= parent->Max.X && P->Y <= parent->Max.Y && P->Z <= parent->Max.Z;
}