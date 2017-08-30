#include "Entity.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"

Real32 LocationUpdate_Clamp(Real32 degrees) {
	degrees = Math_Mod(degrees, 360.0f);
	if (degrees < 0) degrees += 360.0f;
	return degrees;
}

void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos) {
	update->Pos = Vector3_Create3(x, y, z);
	update->RotX = LocationUpdate_Clamp(rotX);
	update->RotY = LocationUpdate_Clamp(rotY);
	update->RotZ = LocationUpdate_Clamp(rotZ);
	update->HeadX = LocationUpdate_Clamp(headX);
	update->IncludesPosition = incPos;
	update->RelativePosition = relPos;
}

#define exc LocationUpdate_Excluded
void LocationUpdate_Empty(LocationUpdate* update) {
	LocationUpdate_Construct(update, 0.0f, 0.0f, 0.0f, exc, exc, exc, exc, false, false);
}

void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX) {
	LocationUpdate_Construct(update, 0.0f, 0.0f, 0.0f, exc, rotY, exc, exc, false, false);
}

void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel) {
	LocationUpdate_Construct(update, pos.X, pos.Y, pos.Z, exc, exc, exc, exc, true, rel);
}

void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel) {
	LocationUpdate_Construct(update, pos.X, pos.Y, pos.Z, exc, rotY, exc, headX, true, rel);
}


Vector3 Entity_GetEyePosition(Entity* entity) {
	Vector3 pos = entity->Position;
	pos.Y += entity->Model->GetEyeY(entity) * entity->ModelScale.Y;
	return pos;
}

void Entity_GetTransform(Entity* entity, Vector3 pos, Vector3 scale, Matrix* m) {
	*m = Matrix_Identity;
	Matrix tmp;

	Matrix_RotateZ(&tmp, -entity->RotZ * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateX(&tmp, -entity->RotX * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_RotateY(&tmp, -entity->RotY * MATH_DEG2RAD);
	Matrix_MulBy(m, &tmp);
	Matrix_Scale(&tmp, scale.X, scale.Y, scale.Z);
	Matrix_MulBy(m, &tmp);
	Matrix_Translate(&tmp, pos.X, pos.Y, pos.Z);
	Matrix_MulBy(m, &tmp);
	/* return rotZ * rotX * rotY * scale * translate; */
}

void Entity_GetPickingBounds(Entity* entity, AABB* bb) {
	AABB_Offset(bb, &entity->ModelAABB, &entity->Position);
}

void Entity_GetBounds(Entity* entity, AABB* bb) {
	AABB_Make(bb, &entity->Position, &entity->Size);
}

bool Entity_TouchesAny(AABB* bounds, TouchesAny_Condition condition) {
	Vector3I bbMin, bbMax;
	Vector3I_Floor(&bbMin, &bounds->Min);
	Vector3I_Floor(&bbMax, &bounds->Max);
	AABB blockBB;
	Vector3 v;

	/* Order loops so that we minimise cache misses */
	Int32 x, y, z;
	for (y = bbMin.Y; y <= bbMax.Y; y++) {
		v.Y = (Real32)y;
		for (z = bbMin.Z; z <= bbMax.Z; z++) {
			v.Z = (Real32)z;
			for (x = bbMin.X; x <= bbMax.X; x++) {
				if (!World_IsValidPos(x, y, z)) continue;
				v.X = (Real32)x;

				BlockID block = World_GetBlock(x, y, z);
				Vector3_Add(&blockBB.Min, &v, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &v, &Block_MaxBB[block]);

				if (!AABB_Intersects(&blockBB, bounds)) continue;
				if (condition(block)) return true;
			}
		}
	}
	return false;
}

bool Entity_IsRope(BlockID b) { return b == BlockID_Rope; }
bool Entity_TouchesAnyRope(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	bounds.Max.Y += 0.5f / 16.0f;
	return Entity_TouchesAny(&bounds, Entity_IsRope);
}

Vector3 entity_liqExpand = { 0.25f / 16.0f, 0.0f / 16.0f, 0.25f / 16.0f };

bool Entity_IsLava(BlockID b) { return Block_ExtendedCollide[b] == CollideType_LiquidLava; }
bool Entity_TouchesAnyLava(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsLava);
}

bool Entity_IsWater(BlockID b) { return Block_ExtendedCollide[b] == CollideType_LiquidWater; }
bool Entity_TouchesAnyWater(Entity* entity) {
	AABB bounds; Entity_GetBounds(entity, &bounds);
	AABB_Offset(&bounds, &bounds, &entity_liqExpand);
	return Entity_TouchesAny(&bounds, Entity_IsWater);
}