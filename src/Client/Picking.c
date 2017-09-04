#include "Picking.h"
#include "ExtMath.h"

Real32 PickedPos_dist;
void PickedPos_TestAxis(PickedPos* pos, Real32 dAxis, Face fAxis) {
	dAxis = Math_AbsF(dAxis);
	if (dAxis >= PickedPos_dist) return;

	PickedPos_dist = dAxis; 
	pos->ClosestFace = fAxis;
}

void PickedPos_SetAsValid(PickedPos* pos, Int32 x, Int32 y, Int32 z,
	Vector3 min, Vector3 max, BlockID block, Vector3 intersect) {
	pos->Min = min; pos->Max = max;
	pos->BlockPos = Vector3I_Create3(x, y, z);
	pos->Valid = true;
	pos->Block = block;
	pos->Intersect = intersect;

	PickedPos_dist = 1000000000.0f;
	PickedPos_TestAxis(pos, intersect.X - min.X, Face_XMin);
	PickedPos_TestAxis(pos, intersect.X - max.X, Face_XMax);
	PickedPos_TestAxis(pos, intersect.Y - min.Y, Face_YMin);
	PickedPos_TestAxis(pos, intersect.Y - max.Y, Face_YMax);
	PickedPos_TestAxis(pos, intersect.Z - min.Z, Face_ZMin);
	PickedPos_TestAxis(pos, intersect.Z - max.Z, Face_ZMax);

	Vector3I offsets[Face_Count];
	offsets[Face_XMin] = Vector3I_Create3(-1, 0, 0);
	offsets[Face_XMax] = Vector3I_Create3(+1, 0, 0);
	offsets[Face_ZMin] = Vector3I_Create3(0, 0, -1);
	offsets[Face_ZMax] = Vector3I_Create3(0, 0, +1);
	offsets[Face_YMin] = Vector3I_Create3(0, -1, 0);
	offsets[Face_YMax] = Vector3I_Create3(0, +1, 0);
	Vector3I_Add(&pos->TranslatedPos, &pos->BlockPos, &offsets[pos->ClosestFace]);
}

void PickedPos_SetAsInvalid(PickedPos* pos) {
	pos->Valid = false;
	pos->BlockPos = Vector3I_MinusOne;
	pos->TranslatedPos = Vector3I_MinusOne;
	pos->ClosestFace = (Face)Face_Count;
	pos->Block = BlockID_Air;
}