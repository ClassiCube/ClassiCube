#include "BlockPhysics.h"
#include "Random.h"
#include "World.h"
#include "Constants.h"
#include "Funcs.h"
#include "Event.h"
#include "ExtMath.h"
#include "Block.h"
#include "Lighting.h"
#include "Options.h"
#include "TreeGen.h"
#include "Platform.h"
#include "Game.h"
#include "ErrorHandler.h"
#include "Vectors.h"

/* Data for a resizable queue, used for liquid physic tick entries. */
typedef struct TickQueue_ {
	UInt32* Buffer;    /* Buffer holding the items in the tick queue. */
	UInt32 BufferSize; /* Max number of elements in the buffer.*/
	UInt32 BufferMask; /* BufferSize - 1, as BufferSize is always a power of two. */
	UInt32 Size;       /* Number of used elements. */
	UInt32 Head;       /* Head index into the buffer. */
	UInt32 Tail;       /* Tail index into the buffer. */
} TickQueue;

void TickQueue_Init(TickQueue* queue) {
	queue->Buffer = NULL;
	queue->BufferSize = 0;
	queue->BufferMask = 0;
	queue->Head = 0;
	queue->Tail = 0;
	queue->Size = 0;
}

void TickQueue_Clear(TickQueue* queue) {
	if (queue->Buffer == NULL) return;
	Platform_MemFree(&queue->Buffer);
	TickQueue_Init(queue);
}

void TickQueue_Resize(TickQueue* queue) {
	if (queue->BufferSize >= (Int32_MaxValue / 4)) {
		ErrorHandler_Fail("TickQueue - too large to resize.");
	}
	UInt32 capacity = queue->BufferSize * 2;
	if (capacity < 32) capacity = 32;

	UInt32* newBuffer = Platform_MemAlloc(capacity, sizeof(UInt32));
	if (newBuffer == NULL) {
		ErrorHandler_Fail("TickQueue - failed to allocate memory");
	}

	UInt32 i, idx;
	for (i = 0; i < queue->Size; i++) {
		idx = (queue->Head + i) & queue->BufferMask;
		newBuffer[i] = queue->Buffer[idx];
	}
	Platform_MemFree(&queue->Buffer);

	queue->Buffer = newBuffer;
	queue->BufferSize = capacity;
	queue->BufferMask = capacity - 1; /* capacity is power of two */
	queue->Head = 0;
	queue->Tail = queue->Size;
}

void TickQueue_Enqueue(TickQueue* queue, UInt32 item) {
	if (queue->Size == queue->BufferSize)
		TickQueue_Resize(queue);

	queue->Buffer[queue->Tail] = item;
	queue->Tail = (queue->Tail + 1) & queue->BufferMask;
	queue->Size++;
}

UInt32 TickQueue_Dequeue(TickQueue* queue) {
	UInt32 result = queue->Buffer[queue->Head];
	queue->Head = (queue->Head + 1) & queue->BufferMask;
	queue->Size--;
	return result;
}


Random physics_rnd;
Int32 physics_tickCount;
Int32 physics_maxWaterX, physics_maxWaterY, physics_maxWaterZ;
TickQueue physics_lavaQ, physics_waterQ;

#define physics_tickMask 0xF8000000UL
#define physics_posMask 0x07FFFFFFUL
#define physics_tickShift 27
#define physics_defLavaTick (30UL << physics_tickShift)
#define physics_defWaterTick (5UL << physics_tickShift)

void Physics_OnNewMapLoaded(void* obj) {
	TickQueue_Clear(&physics_lavaQ);
	TickQueue_Clear(&physics_waterQ);

	physics_maxWaterX = World_MaxX - 2;
	physics_maxWaterY = World_MaxY - 2;
	physics_maxWaterZ = World_MaxZ - 2;

	Tree_Width = World_Width; Tree_Height = World_Height; Tree_Length = World_Length;
	Tree_Blocks = World_Blocks;
	Random_InitFromCurrentTime(&physics_rnd);
	Tree_Rnd = &physics_rnd;
}

void Physics_SetEnabled(bool enabled) {
	Physics_Enabled = enabled;
	Physics_OnNewMapLoaded(NULL);
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
	if (!(WorldEnv_EdgeBlock == BLOCK_WATER || WorldEnv_EdgeBlock == BLOCK_STILL_WATER))
		return false;

	return y >= WorldEnv_SidesHeight && y < WorldEnv_EdgeHeight
		&& (x == 0 || z == 0 || x == World_MaxX || z == World_MaxZ);
}


void Physics_BlockChanged(void* obj, Vector3I p, BlockID oldBlock, BlockID block) {
	if (!Physics_Enabled) return;
	Int32 index = World_Pack(p.X, p.Y, p.Z);

	if (block == BLOCK_AIR && Physics_IsEdgeWater(p.X, p.Y, p.Z)) {
		block = BLOCK_STILL_WATER;
		Game_UpdateBlock(p.X, p.Y, p.Z, BLOCK_STILL_WATER);
	}

	if (block == BLOCK_AIR) {
		PhysicsHandler deleteHandler = Physics_OnDelete[oldBlock];
		if (deleteHandler != NULL) deleteHandler(index, oldBlock);
	} else {
		PhysicsHandler placeHandler = Physics_OnPlace[block];
		if (placeHandler != NULL) placeHandler(index, block);
	}
	Physics_ActivateNeighbours(p.X, p.Y, p.Z, index);
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
		if (other == BLOCK_AIR || (other >= BLOCK_WATER && other <= BLOCK_STILL_LAVA))
			found = index;
		else
			break;
	}
	if (found == -1) return;

	Int32 x, y, z;
	World_Unpack(found, x, y, z);
	Game_UpdateBlock(x, y, z, block);

	World_Unpack(start, x, y, z);
	Game_UpdateBlock(x, y, z, BLOCK_AIR);
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

	BlockID below = BLOCK_AIR;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (below != BLOCK_GRASS) return;

	Int32 treeHeight = 5 + Random_Next(&physics_rnd, 3);
	Game_UpdateBlock(x, y, z, BLOCK_AIR);

	if (TreeGen_CanGrow(x, y, z, treeHeight)) {
		Vector3I coords[Tree_BufferCount];
		BlockID blocks[Tree_BufferCount];
		Int32 count = TreeGen_Grow(x, y, z, treeHeight, coords, blocks);

		Int32 m;
		for (m = 0; m < count; m++) {
			Game_UpdateBlock(coords[m].X, coords[m].Y, coords[m].Z, blocks[m]);
		}
	} else {
		Game_UpdateBlock(x, y, z, BLOCK_SAPLING);
	}
}

void Physics_HandleDirt(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_GRASS);
	}
}

void Physics_HandleGrass(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (!Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_DIRT);
	}
}

void Physics_HandleFlower(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (!Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
		return;
	}

	BlockID below = BLOCK_DIRT;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (!(below == BLOCK_DIRT || below == BLOCK_GRASS)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
	}
}

void Physics_HandleMushroom(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
		return;
	}

	BlockID below = BLOCK_STONE;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (!(below == BLOCK_STONE || below == BLOCK_COBBLE)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
	}
}


void Physics_PlaceLava(Int32 index, BlockID block) {
	TickQueue_Enqueue(&physics_lavaQ, physics_defLavaTick | (UInt32)index);
}

void Physics_PropagateLava(Int32 posIndex, Int32 x, Int32 y, Int32 z) {
	BlockID block = World_Blocks[posIndex];
	if (block == BLOCK_WATER || block == BLOCK_STILL_WATER) {
		Game_UpdateBlock(x, y, z, BLOCK_STONE);
	} else if (Block_Collide[block] == COLLIDE_GAS) {
		TickQueue_Enqueue(&physics_lavaQ, physics_defLavaTick | (UInt32)posIndex);
		Game_UpdateBlock(x, y, z, BLOCK_LAVA);
	}
}

void Physics_ActivateLava(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);

	if (x > 0)          Physics_PropagateLava(index - 1, x - 1, y, z);
	if (x < World_MaxX) Physics_PropagateLava(index + 1, x + 1, y, z);
	if (z > 0)          Physics_PropagateLava(index - World_Width, x, y, z - 1);
	if (z < World_MaxZ) Physics_PropagateLava(index + World_Width, x, y, z + 1);
	if (y > 0)          Physics_PropagateLava(index - World_OneY, x, y - 1, z);
}

void Physics_TickLava(void) {
	Int32 i, count = physics_lavaQ.Size;
	for (i = 0; i < count; i++) {
		Int32 index;
		if (Physics_CheckItem(&physics_lavaQ, &index)) {
			BlockID block = World_Blocks[index];
			if (!(block == BLOCK_LAVA || block == BLOCK_STILL_LAVA)) continue;
			Physics_ActivateLava(index, block);
		}
	}
}


void Physics_PlaceWater(Int32 index, BlockID block) {
	TickQueue_Enqueue(&physics_waterQ, physics_defWaterTick | (UInt32)index);
}

void Physics_PropagateWater(Int32 posIndex, Int32 x, Int32 y, Int32 z) {
	BlockID block = World_Blocks[posIndex];
	if (block == BLOCK_LAVA || block == BLOCK_STILL_LAVA) {
		Game_UpdateBlock(x, y, z, BLOCK_STONE);
	} else if (Block_Collide[block] == COLLIDE_GAS && block != BLOCK_ROPE) {
		/* Sponge check */
		Int32 xx, yy, zz;
		for (yy = (y < 2 ? 0 : y - 2); yy <= (y > physics_maxWaterY ? World_MaxY : y + 2); yy++) {
			for (zz = (z < 2 ? 0 : z - 2); zz <= (z > physics_maxWaterZ ? World_MaxZ : z + 2); zz++) {
				for (xx = (x < 2 ? 0 : x - 2); xx <= (x > physics_maxWaterX ? World_MaxX : x + 2); xx++) {
					block = World_GetBlock(xx, yy, zz);
					if (block == BLOCK_SPONGE) return;
				}
			}
		}

		TickQueue_Enqueue(&physics_waterQ, physics_defWaterTick | (UInt32)posIndex);
		Game_UpdateBlock(x, y, z, BLOCK_WATER);
	}
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

void Physics_TickWater(void) {
	Int32 i, count = physics_waterQ.Size;
	for (i = 0; i < count; i++) {
		Int32 index;
		if (Physics_CheckItem(&physics_waterQ, &index)) {
			BlockID block = World_Blocks[index];
			if (!(block == BLOCK_WATER || block == BLOCK_STILL_WATER)) continue;
			Physics_ActivateWater(index, block);
		}
	}
}


void Physics_PlaceSponge(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Int32 xx, yy, zz;

	for (yy = y - 2; yy <= y + 2; yy++) {
		for (zz = z - 2; zz <= z + 2; zz++) {
			for (xx = x - 2; xx <= x + 2; xx++) {
				if (!World_IsValidPos(xx, yy, zz)) continue;
				block = World_GetBlock(xx, yy, zz);
				if (block == BLOCK_WATER || block == BLOCK_STILL_WATER) {
					Game_UpdateBlock(xx, yy, zz, BLOCK_AIR);
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
					if (block == BLOCK_WATER || block == BLOCK_STILL_WATER) {
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
	if (World_Blocks[index - World_OneY] != BLOCK_SLAB) return;

	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Game_UpdateBlock(x, y, z, BLOCK_AIR);
	Game_UpdateBlock(x, y - 1, z, BLOCK_DOUBLE_SLAB);
}

void Physics_HandleCobblestoneSlab(Int32 index, BlockID block) {
	if (index < World_OneY) return;
	if (World_Blocks[index - World_OneY] != BLOCK_COBBLE_SLAB) return;

	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Game_UpdateBlock(x, y, z, BLOCK_AIR);
	Game_UpdateBlock(x, y - 1, z, BLOCK_COBBLE);
}


UInt8 physics_blocksTnt[BLOCK_CPE_COUNT] = {
	0, 1, 0, 0, 1, 0, 0, 1,  1, 1, 1, 1, 0, 0, 1, 1,  1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 1, 1, 1,  1, 1,
};

void Physics_Explode(Int32 x, Int32 y, Int32 z, Int32 power) {
	Game_UpdateBlock(x, y, z, BLOCK_AIR);
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
				if (block < BLOCK_CPE_COUNT && physics_blocksTnt[block]) continue;

				Game_UpdateBlock(xx, yy, zz, BLOCK_AIR);
				Physics_ActivateNeighbours(xx, yy, zz, index);
			}
		}
	}
}

void Physics_HandleTnt(Int32 index, BlockID block) {
	Int32 x, y, z;
	World_Unpack(index, x, y, z);
	Physics_Explode(x, y, z, 4);
}

void Physics_Init(void) {
	Event_RegisterVoid(&WorldEvents_MapLoaded,    NULL, Physics_OnNewMapLoaded);
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, Physics_BlockChanged);
	Physics_Enabled = Options_GetBool(OPT_BLOCK_PHYSICS, true);
	TickQueue_Init(&physics_lavaQ);
	TickQueue_Init(&physics_waterQ);

	Physics_OnPlace[BLOCK_SAND]        = Physics_DoFalling;
	Physics_OnPlace[BLOCK_GRAVEL]      = Physics_DoFalling;
	Physics_OnActivate[BLOCK_SAND]     = Physics_DoFalling;
	Physics_OnActivate[BLOCK_GRAVEL]   = Physics_DoFalling;
	Physics_OnRandomTick[BLOCK_SAND]   = Physics_DoFalling;
	Physics_OnRandomTick[BLOCK_GRAVEL] = Physics_DoFalling;

	Physics_OnPlace[BLOCK_SAPLING]      = Physics_HandleSapling;
	Physics_OnRandomTick[BLOCK_SAPLING] = Physics_HandleSapling;
	Physics_OnRandomTick[BLOCK_DIRT]    = Physics_HandleDirt;
	Physics_OnRandomTick[BLOCK_GRASS]   = Physics_HandleGrass;

	Physics_OnRandomTick[BLOCK_DANDELION]    = Physics_HandleFlower;
	Physics_OnRandomTick[BLOCK_ROSE]         = Physics_HandleFlower;
	Physics_OnRandomTick[BLOCK_RED_SHROOM]   = Physics_HandleMushroom;
	Physics_OnRandomTick[BLOCK_BROWN_SHROOM] = Physics_HandleMushroom;

	Physics_OnPlace[BLOCK_LAVA]    = Physics_PlaceLava;
	Physics_OnPlace[BLOCK_WATER]   = Physics_PlaceWater;
	Physics_OnPlace[BLOCK_SPONGE]  = Physics_PlaceSponge;
	Physics_OnDelete[BLOCK_SPONGE] = Physics_DeleteSponge;

	Physics_OnActivate[BLOCK_WATER]       = Physics_OnPlace[BLOCK_WATER];
	Physics_OnActivate[BLOCK_STILL_WATER] = Physics_OnPlace[BLOCK_WATER];
	Physics_OnActivate[BLOCK_LAVA]        = Physics_OnPlace[BLOCK_LAVA];
	Physics_OnActivate[BLOCK_STILL_LAVA]  = Physics_OnPlace[BLOCK_LAVA];

	Physics_OnRandomTick[BLOCK_WATER]       = Physics_ActivateWater;
	Physics_OnRandomTick[BLOCK_STILL_WATER] = Physics_ActivateWater;
	Physics_OnRandomTick[BLOCK_LAVA]        = Physics_ActivateLava;
	Physics_OnRandomTick[BLOCK_STILL_LAVA]  = Physics_ActivateLava;

	Physics_OnPlace[BLOCK_SLAB]        = Physics_HandleSlab;
	Physics_OnPlace[BLOCK_COBBLE_SLAB] = Physics_HandleCobblestoneSlab;
	Physics_OnPlace[BLOCK_TNT]         = Physics_HandleTnt;
}

void Physics_Free(void) {
	Event_UnregisterVoid(&WorldEvents_MapLoaded,    NULL, Physics_OnNewMapLoaded);
	Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, Physics_BlockChanged);
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
