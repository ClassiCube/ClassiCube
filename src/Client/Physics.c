#include "Physics.h"
#include "Random.h"
#include "World.h"
#include "WorldEnv.h"
#include "Constants.h"
#include "Funcs.h"
#include "WorldEvents.h"
#include "MiscEvents.h"
#include "ExtMath.h"
#include "TickQueue.h"
#include "Block.h"
#include "BlockEnums.h"
#include "Lighting.h"
#include "Options.h"
#include "TreeGen.h"

Random physics_rnd;
Int32 physics_tickCount;
Int32 physics_maxWaterX, physics_maxWaterY, physics_maxWaterZ;
TickQueue physics_lavaQ, physics_waterQ;

#define physics_tickMask 0xF8000000UL
#define physics_posMask 0x07FFFFFFUL
#define physics_tickShift 27
#define physics_defLavaTick (30UL << physics_tickShift)
#define physics_defWaterTick (5UL << physics_tickShift)

void Physics_SetEnabled(bool enabled) {
	Physics_Enabled = enabled;
	Physics_OnNewMapLoaded();
}

void Physics_Init(void) {
	EventHandler_RegisterVoid(WorldEvents_MapLoaded, Physics_OnNewMapLoaded);
	EventHandler_RegisterBlock(UserEvents_BlockChanged, Physics_BlockChanged);
	Physics_Enabled = Options_GetBool(OptionsKey_SingleplayerPhysics, true);
	TickQueue_Init(&physics_lavaQ);
	TickQueue_Init(&physics_waterQ);

	Physics_OnPlace[BlockID_Sand] = Physics_DoFalling;
	Physics_OnPlace[BlockID_Gravel] = Physics_DoFalling;
	Physics_OnActivate[BlockID_Sand] = Physics_DoFalling;
	Physics_OnActivate[BlockID_Gravel] = Physics_DoFalling;
	Physics_OnRandomTick[BlockID_Sand] = Physics_DoFalling;
	Physics_OnRandomTick[BlockID_Gravel] = Physics_DoFalling;

	Physics_OnPlace[BlockID_Sapling] = Physics_HandleSapling;
	Physics_OnRandomTick[BlockID_Sapling] = Physics_HandleSapling;
	Physics_OnRandomTick[BlockID_Dirt] = Physics_HandleDirt;
	Physics_OnRandomTick[BlockID_Grass] = Physics_HandleGrass;

	Physics_OnRandomTick[BlockID_Dandelion] = Physics_HandleFlower;
	Physics_OnRandomTick[BlockID_Rose] = Physics_HandleFlower;
	Physics_OnRandomTick[BlockID_RedMushroom] = Physics_HandleMushroom;
	Physics_OnRandomTick[BlockID_BrownMushroom] = Physics_HandleMushroom;

	Physics_OnPlace[BlockID_Lava] = Physics_PlaceLava;
	Physics_OnPlace[BlockID_Water] = Physics_PlaceWater;
	Physics_OnPlace[BlockID_Sponge] = Physics_PlaceSponge;
	Physics_OnDelete[BlockID_Sponge] = Physics_DeleteSponge;

	Physics_OnActivate[BlockID_Water] = Physics_OnPlace[BlockID_Water];
	Physics_OnActivate[BlockID_StillWater] = Physics_OnPlace[BlockID_Water];
	Physics_OnActivate[BlockID_Lava] = Physics_OnPlace[BlockID_Lava];
	Physics_OnActivate[BlockID_StillLava] = Physics_OnPlace[BlockID_Lava];

	Physics_OnRandomTick[BlockID_Water] = Physics_ActivateWater;
	Physics_OnRandomTick[BlockID_StillWater] = Physics_ActivateWater;
	Physics_OnRandomTick[BlockID_Lava] = Physics_ActivateLava;
	Physics_OnRandomTick[BlockID_StillLava] = Physics_ActivateLava;

	Physics_OnPlace[BlockID_Slab] = Physics_HandleSlab;
	Physics_OnPlace[BlockID_CobblestoneSlab] = Physics_HandleCobblestoneSlab;
	Physics_OnPlace[BlockID_TNT] = Physics_HandleTnt;
}

void Physics_Free(void) {
	EventHandler_UnregisterVoid(WorldEvents_MapLoaded, Physics_OnNewMapLoaded);
	EventHandler_UnregisterBlock(UserEvents_BlockChanged, Physics_BlockChanged);
}

void Physics_Tick(void) {
	if (!Physics_Enabled || World_Blocks == NULL) return;

	/*if ((tickCount % 5) == 0) {*/
	Physics_TickLava();
	Physics_TickWater();
	/*}*/
	physics_tickCount++;
	Physics_TickRandomBlocks();
}


void Physics_ActivateNeighbours(Int32 x, Int32 y, Int32 z, Int32 index) {
	if (x > 0)          Physics_Activate(index - 1);
	if (x < World_MaxX) Physics_Activate(index + 1);
	if (z > 0)          Physics_Activate(index - World_Width);
	if (z < World_MaxZ) Physics_Activate(index + World_Width);
	if (y > 0)          Physics_Activate(index - World_OneY);
	if (y < World_MaxY) Physics_Activate(index + World_OneY);
}

void Physics_Activate(Int32 index) {
	BlockID block = World_Blocks[index];
	PhysicsHandler activate = Physics_OnActivate[block];
	if (activate != NULL) activate(index, block);
}

bool Physics_IsEdgeWater(Int32 x, Int32 y, Int32 z) {
	if (!(WorldEnv_EdgeBlock == BlockID_Water || WorldEnv_EdgeBlock == BlockID_StillWater))
		return false;

	return y >= WorldEnv_SidesHeight && y < WorldEnv_EdgeHeight
		&& (x == 0 || z == 0 || x == World_MaxX || z == World_MaxZ);
}


void Physics_BlockChanged(Vector3I p, BlockID oldBlock, BlockID block) {
	if (!Physics_Enabled) return;
	Int32 index = World_Pack(p.X, p.Y, p.Z);

	if (block == BlockID_Air && Physics_IsEdgeWater(p.X, p.Y, p.Z)) {
		block = BlockID_StillWater;
		Game_UpdateBlock(p.X, p.Y, p.Z, BlockID_StillWater);
	}

	if (block == BlockID_Air) {
		PhysicsHandler deleteHandler = Physics_OnDelete[oldBlock];
		if (deleteHandler != NULL) deleteHandler(index, oldBlock);
	} else {
		PhysicsHandler placeHandler = Physics_OnPlace[block];
		if (placeHandler != NULL) placeHandler(index, block);
	}
	Physics_ActivateNeighbours(p.X, p.Y, p.Z, index);
}

void Physics_OnNewMapLoaded(void) {
	TickQueue_Clear(&physics_lavaQ);
	TickQueue_Clear(&physics_waterQ);

	physics_maxWaterX = World_MaxX - 2;
	physics_maxWaterY = World_MaxY - 2;
	physics_maxWaterZ = World_MaxZ - 2;

	Tree_Width = World_Width; Tree_Height = World_Height; Tree_Length = World_Length;
	Tree_Blocks = World_Blocks;
	Tree_Rnd = &physics_rnd;
}

void Physics_TickRandomBlocks(void) {
	Int32 x, y, z;
	for (y = 0; y < World_Height; y += CHUNK_SIZE) {
		Int32 y2 = min(y + CHUNK_MAX, World_MaxY);
		for (z = 0; z < World_Length; z += CHUNK_SIZE) {
			Int32 z2 = min(z + CHUNK_MAX, World_MaxZ);
			for (x = 0; x < World_Width; x += CHUNK_SIZE) {
				Int32 x2 = min(x + CHUNK_MAX, World_MaxX);
				Int32 lo = World_Pack( x,  y,  z);
				Int32 hi = World_Pack(x2, y2, z2);

				/* Inlined 3 random ticks for this chunk */
				Int32 index = Random_Range(&physics_rnd, lo, hi);
				BlockID block = World_Blocks[index];
				PhysicsHandler tick = Physics_OnRandomTick[block];
				if (tick != NULL) tick(index, block);

				index = Random_Range(&physics_rnd, lo, hi);
				block = World_Blocks[index];
				tick = Physics_OnRandomTick[block];
				if (tick != NULL) tick(index, block);

				index = Random_Range(&physics_rnd, lo, hi);
				block = World_Blocks[index];
				tick = Physics_OnRandomTick[block];
				if (tick != NULL) tick(index, block);
			}
		}
	}
}


void Physics_DoFalling(Int32 index, BlockID block) {
	Int32 found = -1, start = index;
	/* Find lowest block can fall into */
	while (index >= World_OneY) {
		index -= World_OneY;
		BlockID other = World_Blocks[index];
		if (other == BlockID_Air || (other >= BlockID_Water && other <= BlockID_StillLava))
			found = index;
		else
			break;
	}
	if (found == -1) return;

	Int32 x, y, z;
	World_Unpack(found, x, y, z);
	Game_UpdateBlock(x, y, z, block);

	World_Unpack(start, x, y, z);
	Game_UpdateBlock(x, y, z, BlockID_Air);
	Physics_ActivateNeighbours(x, y, z, start);
}

bool Physics_CheckItem(TickQueue* queue, Int32* posIndex) {
	UInt32 packed = TickQueue_Dequeue(queue);
	Int32 tickDelay = (Int32)((packed & physics_tickMask) >> physics_tickShift);
	*posIndex = (Int32)(packed & physics_posMask);

	if (tickDelay > 0) {
		tickDelay--;
		UInt32 item = (UInt32)(*posIndex) | ((UInt32)tickDelay << physics_tickShift);
		TickQueue_Enqueue(queue, item);
		return false;
	}
	return true;
}


void Physics_HandleSapling(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	BlockID below = BlockID_Air;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (below != BlockID_Grass) return;

	Int32 treeHeight = 5 + Random_Next(&physics_rnd, 3);
	Game_UpdateBlock(x, y, z, BlockID_Air);

	if (TreeGen_CanGrow(x, y, z, treeHeight)) {
		Vector3I coords[Tree_BufferCount];
		BlockID blocks[Tree_BufferCount];
		Int32 count = TreeGen_Grow(x, y, z, treeHeight, coords, blocks);

		Int32 m;
		for (m = 0; m < count; m++) {
			Game_UpdateBlock(coords[m].X, coords[m].Y, coords[m].Z, blocks[m]);
		}
	} else {
		Game_UpdateBlock(x, y, z, BlockID_Sapling);
	}
}

void Physics_HandleDirt(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BlockID_Grass);
	}
}

void Physics_HandleGrass(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (!Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BlockID_Dirt);
	}
}

void Physics_HandleFlower(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (!Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BlockID_Air);
		Physics_ActivateNeighbours(x, y, z, index);
		return;
	}

	BlockID below = BlockID_Dirt;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (!(below == BlockID_Dirt || below == BlockID_Grass)) {
		Game_UpdateBlock(x, y, z, BlockID_Air);
		Physics_ActivateNeighbours(x, y, z, index);
	}
}

void Physics_HandleMushroom(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BlockID_Air);
		Physics_ActivateNeighbours(x, y, z, index);
		return;
	}

	BlockID below = BlockID_Stone;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (!(below == BlockID_Stone || below == BlockID_Cobblestone)) {
		Game_UpdateBlock(x, y, z, BlockID_Air);
		Physics_ActivateNeighbours(x, y, z, index);
	}
}


void Physics_TickLava(void) {
	Int32 i, count = physics_lavaQ.Size;
	for (i = 0; i < count; i++) {
		Int32 index;
		if (Physics_CheckItem(&physics_lavaQ, &index)) {
			BlockID block = World_Blocks[index];
			if (!(block == BlockID_Lava || block == BlockID_StillLava)) continue;
			Physics_ActivateLava(index, block);
		}
	}
}

void Physics_PlaceLava(Int32 index, BlockID block) {
	TickQueue_Enqueue(&physics_lavaQ, physics_defLavaTick | (UInt32)index);
}

void Physics_ActivateLava(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (x > 0)          Physics_PropagateLava(index - 1,           x - 1, y,     z);
	if (x < World_MaxX) Physics_PropagateLava(index + 1,           x + 1, y,     z);
	if (z > 0)          Physics_PropagateLava(index - World_Width, x,     y,     z - 1);
	if (z < World_MaxZ) Physics_PropagateLava(index + World_Width, x,     y,     z + 1);
	if (y > 0)          Physics_PropagateLava(index - World_OneY,  x,     y - 1, z);
}

void Physics_PropagateLava(Int32 posIndex, Int32 x, Int32 y, Int32 z) {
	BlockID block = World_Blocks[posIndex];
	if (block == BlockID_Water || block == BlockID_StillWater) {
		Game_UpdateBlock(x, y, z, BlockID_Stone);
	} else if (Block_Collide[block] == CollideType_Gas) {
		TickQueue_Enqueue(&physics_lavaQ, physics_defLavaTick | (UInt32)posIndex);
		Game_UpdateBlock(x, y, z, BlockID_Lava);
	}
}


void Physics_TickWater(void) {
	Int32 i, count = physics_waterQ.Size;
	for (i = 0; i < count; i++) {
		Int32 index;
		if (Physics_CheckItem(&physics_waterQ, &index)) {
			BlockID block = World_Blocks[index];
			if (!(block == BlockID_Water || block == BlockID_StillWater)) continue;
			Physics_ActivateWater(index, block);
		}
	}
}

void Physics_PlaceWater(Int32 index, BlockID block) {
	TickQueue_Enqueue(&physics_waterQ, physics_defWaterTick | (UInt32)index);
}

void Physics_ActivateWater(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (x > 0)          Physics_PropagateWater(index - 1,           x - 1, y,     z);
	if (x < World_MaxX) Physics_PropagateWater(index + 1,           x + 1, y,     z);
	if (z > 0)          Physics_PropagateWater(index - World_Width, x,     y,     z - 1);
	if (z < World_MaxZ) Physics_PropagateWater(index + World_Width, x,     y,     z + 1);
	if (y > 0)          Physics_PropagateWater(index - World_OneY,  x,     y - 1, z);
}

void Physics_PropagateWater(Int32 posIndex, Int32 x, Int32 y, Int32 z) {
	BlockID block = World_Blocks[posIndex];
	if (block == BlockID_Lava || block == BlockID_StillLava) {
		Game_UpdateBlock(x, y, z, BlockID_Stone);
	} else if (Block_Collide[block] == CollideType_Gas && block != BlockID_Rope) {
		/* Sponge check */
		Int32 xx, yy, zz;
		for (yy = (y < 2 ? 0 : y - 2); yy <= (y > physics_maxWaterY ? World_MaxY : y + 2); yy++) {
			for (zz = (z < 2 ? 0 : z - 2); zz <= (z > physics_maxWaterZ ? World_MaxZ : z + 2); zz++) {
				for (xx = (x < 2 ? 0 : x - 2); xx <= (x > physics_maxWaterX ? World_MaxX : x + 2); xx++) {
					block = World_GetBlock(xx, yy, zz);
					if (block == BlockID_Sponge) return;
				}
			}
		}

		TickQueue_Enqueue(&physics_waterQ, physics_defWaterTick | (UInt32)posIndex);
		Game_UpdateBlock(x, y, z, BlockID_Water);
	}
}


void Physics_PlaceSponge(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Int32 xx, yy, zz;

	for (yy = y - 2; yy <= y + 2; yy++) {
		for (zz = z - 2; zz <= z + 2; zz++) {
			for (xx = x - 2; xx <= x + 2; xx++) {
				block = World_SafeGetBlock(xx, yy, zz);
				if (block == BlockID_Water || block == BlockID_StillWater) {
					Game_UpdateBlock(xx, yy, zz, BlockID_Air);
				}
			}
		}
	}
}

void Physics_DeleteSponge(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Int32 xx, yy, zz;

	for (yy = y - 3; yy <= y + 3; yy++) {
		for (zz = z - 3; zz <= z + 3; zz++) {
			for (xx = x - 3; xx <= x + 3; xx++) {
				if (Math_AbsI(yy - y) == 3 || Math_AbsI(zz - z) == 3 || Math_AbsI(xx - x) == 3) {
					if (!World_IsValidPos(xx, yy, zz)) continue;

					index = World_Pack(xx, yy, zz);
					block = World_Blocks[index];
					if (block == BlockID_Water || block == BlockID_StillWater) {
						UInt32 item = (1UL << physics_tickShift) | (UInt32)index;
						TickQueue_Enqueue(&physics_waterQ, item);
					}
				}
			}
		}
	}
}


void Physics_HandleSlab(Int32 index, BlockID block) {
	if (index < World_OneY) return;
	if (World_Blocks[index - World_OneY] != BlockID_Slab) return;

	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Game_UpdateBlock(x, y, z, BlockID_Air);
	Game_UpdateBlock(x, y - 1, z, BlockID_DoubleSlab);
}

void Physics_HandleCobblestoneSlab(Int32 index, BlockID block) {
	if (index < World_OneY) return;
	if (World_Blocks[index - World_OneY] != BlockID_CobblestoneSlab) return;

	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Game_UpdateBlock(x, y, z, BlockID_Air);
	Game_UpdateBlock(x, y - 1, z, BlockID_Cobblestone);
}


UInt8 physics_blocksTnt[Block_CpeCount] = {
	0, 1, 0, 0, 1, 0, 0, 1,  1, 1, 1, 1, 0, 0, 1, 1,  1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 1, 1, 1,  1, 1,
};

void Physics_HandleTnt(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Physics_Explode(z, y, z, 4);
}

void Physics_Explode(Int32 x, Int32 y, Int32 z, Int32 power) {
	Game_UpdateBlock(x, y, z, BlockID_Air);
	Int32 index = World_Pack(x, y, z);
	Physics_ActivateNeighbours(x, y, z, index);

	Int32 powerSquared = power * power;
	Int32 dx, dy, dz;
	for (dy = -power; dy <= power; dy++) {
		for (dz = -power; dz <= power; dz++) {
			for (dx = -power; dx <= power; dx++) {
				if (dx * dx + dy * dy + dz * dz > powerSquared) continue;

				Int32 xx = x + dx, yy = y + dy, zz = z + dz;
				if (!World_IsValidPos(xx, yy, zz)) continue;
				index = World_Pack(xx, yy, zz);

				BlockID block = World_Blocks[index];
				if (block < Block_CpeCount && physics_blocksTnt[block]) continue;

				Game_UpdateBlock(xx, yy, zz, BlockID_Air);
				Physics_ActivateNeighbours(xx, yy, zz, index);
			}
		}
	}
}