#if 0
#include "Respawn.h"
#include "ExtMath.h"
#include "World.h"
#include "Block.h"
#include "Funcs.h"
#include "Entity.h"

#define Respawn_NotFound -10000.0f
Real32 Respawn_HighestFreeY(AABB* bb) {
	Int32 minX = Math_Floor(bb->Min.X), maxX = Math_Floor(bb->Max.X);
	Int32 minY = Math_Floor(bb->Min.Y), maxY = Math_Floor(bb->Max.Y);
	Int32 minZ = Math_Floor(bb->Min.Z), maxZ = Math_Floor(bb->Max.Z);

	Real32 spawnY = Respawn_NotFound;
	AABB blockBB;
	Int32 x, y, z;
	Vector3 pos;

	for (y = minY; y <= maxY; y++) {
		pos.Y = (Real32)y;
		for (z = minZ; z <= maxZ; z++) {
			pos.Z = (Real32)z;
			for (x = minX; x <= maxX; x++) {
				pos.X = (Real32)x;
				BlockID block = World_GetPhysicsBlock(x, y, z);
				Vector3_Add(&blockBB.Min, &pos, &Block_MinBB[block]);
				Vector3_Add(&blockBB.Max, &pos, &Block_MaxBB[block]);

				if (Block_Collide[block] != CollideType_Solid) continue;
				if (!AABB_Intersects(bb, &blockBB)) continue;
				spawnY = max(spawnY, blockBB.Max.Y);
			}
		}
	}
	return spawnY;
}

Vector3 Respawn_FindSpawnPosition(Real32 x, Real32 z, Vector3 modelSize) {
	Vector3 spawn = Vector3_Create3(x, 0.0f, z);
	spawn.Y = World_Height + ENTITY_ADJUSTMENT;
	AABB bb;
	AABB_Make(&bb, &spawn, &modelSize);
	spawn.Y = 0.0f;

	Int32 y;
	for (y = World_Height; y >= 0; y--) {
		Real32 highestY = Respawn_HighestFreeY(&bb);
		if (highestY != Respawn_NotFound) {
			spawn.Y = highestY; break;
		}
		bb.Min.Y -= 1; bb.Max.Y -= 1;
	}
	return spawn;
}
#endif