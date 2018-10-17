#include "BlockPhysics.h"
#include "World.h"
#include "Constants.h"
#include "Funcs.h"
#include "Event.h"
#include "ExtMath.h"
#include "Block.h"
#include "Lighting.h"
#include "Options.h"
#include "MapGenerator.h"
#include "Platform.h"
#include "Game.h"
#include "ErrorHandler.h"
#include "Vectors.h"
#include "Chat.h"

/* Data for a resizable queue, used for liquid physic tick entries. */
struct TickQueue {
	uint32_t* Entries;     /* Buffer holding the items in the tick queue */
	uint32_t  EntriesSize; /* Max number of elements in the buffer */
	uint32_t  EntriesMask; /* EntriesSize - 1, as EntriesSize is always a power of two */
	uint32_t  Size;        /* Number of used elements */
	uint32_t  Head;        /* Head index into the buffer */
	uint32_t  Tail;        /* Tail index into the buffer */
};

static void TickQueue_Init(struct TickQueue* queue) {
	queue->Entries     = NULL;
	queue->EntriesSize = 0;
	queue->EntriesMask = 0;
	queue->Head = 0;
	queue->Tail = 0;
	queue->Size = 0;
}

static void TickQueue_Clear(struct TickQueue* queue) {
	if (!queue->Entries) return;
	Mem_Free(queue->Entries);
	TickQueue_Init(queue);
}

static void TickQueue_Resize(struct TickQueue* queue) {
	if (queue->EntriesSize >= (Int32_MaxValue / 4)) {
		Chat_AddRaw("&cTickQueue too large, clearing");
		TickQueue_Clear(queue);
		return;
	}

	uint32_t capacity = queue->EntriesSize * 2;
	if (capacity < 32) capacity = 32;
	uint32_t* entries = Mem_Alloc(capacity, sizeof(uint32_t), "physics tick queue");

	uint32_t i, idx;
	for (i = 0; i < queue->Size; i++) {
		idx = (queue->Head + i) & queue->EntriesMask;
		entries[i] = queue->Entries[idx];
	}
	Mem_Free(queue->Entries);

	queue->Entries     = entries;
	queue->EntriesSize = capacity;
	queue->EntriesMask = capacity - 1; /* capacity is power of two */
	queue->Head = 0;
	queue->Tail = queue->Size;
}

static void TickQueue_Enqueue(struct TickQueue* queue, uint32_t item) {
	if (queue->Size == queue->EntriesSize)
		TickQueue_Resize(queue);

	queue->Entries[queue->Tail] = item;
	queue->Tail = (queue->Tail + 1) & queue->EntriesMask;
	queue->Size++;
}

static uint32_t TickQueue_Dequeue(struct TickQueue* queue) {
	uint32_t result = queue->Entries[queue->Head];
	queue->Head = (queue->Head + 1) & queue->EntriesMask;
	queue->Size--;
	return result;
}


typedef void(*PhysicsHandler)(int index, BlockID block);
PhysicsHandler Physics_OnActivate[BLOCK_COUNT];
PhysicsHandler Physics_OnRandomTick[BLOCK_COUNT];
PhysicsHandler Physics_OnPlace[BLOCK_COUNT];
PhysicsHandler Physics_OnDelete[BLOCK_COUNT];

Random physics_rnd;
int physics_tickCount;
int physics_maxWaterX, physics_maxWaterY, physics_maxWaterZ;
struct TickQueue physics_lavaQ, physics_waterQ;

#define PHYSICS_DELAY_MASK 0xF8000000UL
#define PHYSICS_POS_MASK   0x07FFFFFFUL
#define PHYSICS_DELAY_SHIFT 27
#define PHYSICS_LAVA_DELAY (30U << PHYSICS_DELAY_SHIFT)
#define PHYSICS_WATER_DELAY (5U << PHYSICS_DELAY_SHIFT)

static void Physics_OnNewMapLoaded(void* obj) {
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

static void Physics_Activate(int index) {
	BlockID block = World_Blocks[index];
	PhysicsHandler activate = Physics_OnActivate[block];
	if (activate) activate(index, block);
}

static void Physics_ActivateNeighbours(int x, int y, int z, int index) {
	if (x > 0)          Physics_Activate(index - 1);
	if (x < World_MaxX) Physics_Activate(index + 1);
	if (z > 0)          Physics_Activate(index - World_Width);
	if (z < World_MaxZ) Physics_Activate(index + World_Width);
	if (y > 0)          Physics_Activate(index - World_OneY);
	if (y < World_MaxY) Physics_Activate(index + World_OneY);
}

static bool Physics_IsEdgeWater(int x, int y, int z) {
	return
		(Env_EdgeBlock == BLOCK_WATER || Env_EdgeBlock == BLOCK_STILL_WATER)
		&& (y >= Env_SidesHeight && y < Env_EdgeHeight)
		&& (x == 0 || z == 0 || x == World_MaxX || z == World_MaxZ);
}


static void Physics_BlockChanged(void* obj, Vector3I p, BlockID old, BlockID now) {
	if (!Physics_Enabled) return;
	int index = World_Pack(p.X, p.Y, p.Z);

	if (now == BLOCK_AIR && Physics_IsEdgeWater(p.X, p.Y, p.Z)) {
		now = BLOCK_STILL_WATER;
		Game_UpdateBlock(p.X, p.Y, p.Z, BLOCK_STILL_WATER);
	}

	if (now == BLOCK_AIR) {
		PhysicsHandler deleteHandler = Physics_OnDelete[old];
		if (deleteHandler) deleteHandler(index, old);
	} else {
		PhysicsHandler placeHandler = Physics_OnPlace[now];
		if (placeHandler) placeHandler(index, now);
	}
	Physics_ActivateNeighbours(p.X, p.Y, p.Z, index);
}

static void Physics_TickRandomBlocks(void) {
	int x, y, z;
	for (y = 0; y < World_Height; y += CHUNK_SIZE) {
		int y2 = min(y + CHUNK_MAX, World_MaxY);
		for (z = 0; z < World_Length; z += CHUNK_SIZE) {
			int z2 = min(z + CHUNK_MAX, World_MaxZ);
			for (x = 0; x < World_Width; x += CHUNK_SIZE) {
				int x2 = min(x + CHUNK_MAX, World_MaxX);
				int lo = World_Pack( x,  y,  z);
				int hi = World_Pack(x2, y2, z2);

				/* Inlined 3 random ticks for this chunk */
				int index = Random_Range(&physics_rnd, lo, hi);
				BlockID block = World_Blocks[index];
				PhysicsHandler tick = Physics_OnRandomTick[block];
				if (tick) tick(index, block);

				index = Random_Range(&physics_rnd, lo, hi);
				block = World_Blocks[index];
				tick = Physics_OnRandomTick[block];
				if (tick) tick(index, block);

				index = Random_Range(&physics_rnd, lo, hi);
				block = World_Blocks[index];
				tick = Physics_OnRandomTick[block];
				if (tick) tick(index, block);
			}
		}
	}
}


static void Physics_DoFalling(int index, BlockID block) {
	int found = -1, start = index;
	int x, y, z;

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
	World_Unpack(found, x, y, z);
	Game_UpdateBlock(x, y, z, block);

	World_Unpack(start, x, y, z);
	Game_UpdateBlock(x, y, z, BLOCK_AIR);
	Physics_ActivateNeighbours(x, y, z, start);
}

static bool Physics_CheckItem(struct TickQueue* queue, int* posIndex) {
	uint32_t packed = TickQueue_Dequeue(queue);
	int tickDelay = (int)((packed & PHYSICS_DELAY_MASK) >> PHYSICS_DELAY_SHIFT);
	*posIndex = (int)(packed & PHYSICS_POS_MASK);

	if (tickDelay > 0) {
		tickDelay--;
		uint32_t item = (uint32_t)(*posIndex) | ((uint32_t)tickDelay << PHYSICS_DELAY_SHIFT);
		TickQueue_Enqueue(queue, item);
		return false;
	}
	return true;
}


static void Physics_HandleSapling(int index, BlockID block) {
	Vector3I coords[TREE_MAX_COUNT];
	BlockRaw blocks[TREE_MAX_COUNT];
	int i, count, height;

	BlockID below;
	int x, y, z;
	World_Unpack(index, x, y, z);

	below = BLOCK_AIR;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (below != BLOCK_GRASS) return;

	height = 5 + Random_Next(&physics_rnd, 3);
	Game_UpdateBlock(x, y, z, BLOCK_AIR);

	if (TreeGen_CanGrow(x, y, z, height)) {	
		count = TreeGen_Grow(x, y, z, height, coords, blocks);

		for (i = 0; i < count; i++) {
			Game_UpdateBlock(coords[i].X, coords[i].Y, coords[i].Z, blocks[i]);
		}
	} else {
		Game_UpdateBlock(x, y, z, BLOCK_SAPLING);
	}
}

static void Physics_HandleDirt(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);

	if (Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_GRASS);
	}
}

static void Physics_HandleGrass(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);

	if (!Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_DIRT);
	}
}

static void Physics_HandleFlower(int index, BlockID block) {
	BlockID below;
	int x, y, z;
	World_Unpack(index, x, y, z);

	if (!Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
		return;
	}

	below = BLOCK_DIRT;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (!(below == BLOCK_DIRT || below == BLOCK_GRASS)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
	}
}

static void Physics_HandleMushroom(int index, BlockID block) {
	BlockID below;
	int x, y, z;
	World_Unpack(index, x, y, z);

	if (Lighting_IsLit(x, y, z)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
		return;
	}

	below = BLOCK_STONE;
	if (y > 0) below = World_Blocks[index - World_OneY];
	if (!(below == BLOCK_STONE || below == BLOCK_COBBLE)) {
		Game_UpdateBlock(x, y, z, BLOCK_AIR);
		Physics_ActivateNeighbours(x, y, z, index);
	}
}


static void Physics_PlaceLava(int index, BlockID block) {
	TickQueue_Enqueue(&physics_lavaQ, PHYSICS_LAVA_DELAY | index);
}

static void Physics_PropagateLava(int posIndex, int x, int y, int z) {
	BlockID block = World_Blocks[posIndex];
	if (block == BLOCK_WATER || block == BLOCK_STILL_WATER) {
		Game_UpdateBlock(x, y, z, BLOCK_STONE);
	} else if (Block_Collide[block] == COLLIDE_GAS) {
		TickQueue_Enqueue(&physics_lavaQ, PHYSICS_LAVA_DELAY | posIndex);
		Game_UpdateBlock(x, y, z, BLOCK_LAVA);
	}
}

static void Physics_ActivateLava(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);

	if (x > 0)          Physics_PropagateLava(index - 1, x - 1, y, z);
	if (x < World_MaxX) Physics_PropagateLava(index + 1, x + 1, y, z);
	if (z > 0)          Physics_PropagateLava(index - World_Width, x, y, z - 1);
	if (z < World_MaxZ) Physics_PropagateLava(index + World_Width, x, y, z + 1);
	if (y > 0)          Physics_PropagateLava(index - World_OneY, x, y - 1, z);
}

static void Physics_TickLava(void) {
	int i, count = physics_lavaQ.Size;
	for (i = 0; i < count; i++) {
		int index;
		if (Physics_CheckItem(&physics_lavaQ, &index)) {
			BlockID block = World_Blocks[index];
			if (!(block == BLOCK_LAVA || block == BLOCK_STILL_LAVA)) continue;
			Physics_ActivateLava(index, block);
		}
	}
}


static void Physics_PlaceWater(int index, BlockID block) {
	TickQueue_Enqueue(&physics_waterQ, PHYSICS_WATER_DELAY | index);
}

static void Physics_PropagateWater(int posIndex, int x, int y, int z) {
	BlockID block = World_Blocks[posIndex];
	if (block == BLOCK_LAVA || block == BLOCK_STILL_LAVA) {
		Game_UpdateBlock(x, y, z, BLOCK_STONE);
	} else if (Block_Collide[block] == COLLIDE_GAS && block != BLOCK_ROPE) {
		/* Sponge check */
		int xx, yy, zz;
		for (yy = (y < 2 ? 0 : y - 2); yy <= (y > physics_maxWaterY ? World_MaxY : y + 2); yy++) {
			for (zz = (z < 2 ? 0 : z - 2); zz <= (z > physics_maxWaterZ ? World_MaxZ : z + 2); zz++) {
				for (xx = (x < 2 ? 0 : x - 2); xx <= (x > physics_maxWaterX ? World_MaxX : x + 2); xx++) {
					block = World_GetBlock(xx, yy, zz);
					if (block == BLOCK_SPONGE) return;
				}
			}
		}

		TickQueue_Enqueue(&physics_waterQ, PHYSICS_WATER_DELAY | posIndex);
		Game_UpdateBlock(x, y, z, BLOCK_WATER);
	}
}

static void Physics_ActivateWater(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);

	if (x > 0)          Physics_PropagateWater(index - 1,           x - 1, y,     z);
	if (x < World_MaxX) Physics_PropagateWater(index + 1,           x + 1, y,     z);
	if (z > 0)          Physics_PropagateWater(index - World_Width, x,     y,     z - 1);
	if (z < World_MaxZ) Physics_PropagateWater(index + World_Width, x,     y,     z + 1);
	if (y > 0)          Physics_PropagateWater(index - World_OneY,  x,     y - 1, z);
}

static void Physics_TickWater(void) {
	int i, count = physics_waterQ.Size;
	for (i = 0; i < count; i++) {
		int index;
		if (Physics_CheckItem(&physics_waterQ, &index)) {
			BlockID block = World_Blocks[index];
			if (!(block == BLOCK_WATER || block == BLOCK_STILL_WATER)) continue;
			Physics_ActivateWater(index, block);
		}
	}
}


static void Physics_PlaceSponge(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);
	int xx, yy, zz;

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

static void Physics_DeleteSponge(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);
	int xx, yy, zz;

	for (yy = y - 3; yy <= y + 3; yy++) {
		for (zz = z - 3; zz <= z + 3; zz++) {
			for (xx = x - 3; xx <= x + 3; xx++) {
				if (Math_AbsI(yy - y) == 3 || Math_AbsI(zz - z) == 3 || Math_AbsI(xx - x) == 3) {
					if (!World_IsValidPos(xx, yy, zz)) continue;

					index = World_Pack(xx, yy, zz);
					block = World_Blocks[index];
					if (block == BLOCK_WATER || block == BLOCK_STILL_WATER) {
						uint32_t item = (1UL << PHYSICS_DELAY_SHIFT) | (uint32_t)index;
						TickQueue_Enqueue(&physics_waterQ, item);
					}
				}
			}
		}
	}
}


static void Physics_HandleSlab(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);
	if (index < World_OneY) return;

	if (World_Blocks[index - World_OneY] != BLOCK_SLAB) return;
	Game_UpdateBlock(x, y,     z, BLOCK_AIR);
	Game_UpdateBlock(x, y - 1, z, BLOCK_DOUBLE_SLAB);
}

static void Physics_HandleCobblestoneSlab(int index, BlockID block) {
	int x, y, z;
	World_Unpack(index, x, y, z);
	if (index < World_OneY) return;

	if (World_Blocks[index - World_OneY] != BLOCK_COBBLE_SLAB) return;
	Game_UpdateBlock(x, y,     z, BLOCK_AIR);
	Game_UpdateBlock(x, y - 1, z, BLOCK_COBBLE);
}


uint8_t physics_blocksTnt[BLOCK_CPE_COUNT] = {
	0, 1, 0, 0, 1, 0, 0, 1,  1, 1, 1, 1, 0, 0, 1, 1,  1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 1, 1, 1,  1, 1,
};

static void Physics_Explode(int x, int y, int z, int power) {
	Game_UpdateBlock(x, y, z, BLOCK_AIR);
	int index = World_Pack(x, y, z);
	Physics_ActivateNeighbours(x, y, z, index);

	int powerSquared = power * power;
	int dx, dy, dz;
	for (dy = -power; dy <= power; dy++) {
		for (dz = -power; dz <= power; dz++) {
			for (dx = -power; dx <= power; dx++) {
				if (dx * dx + dy * dy + dz * dz > powerSquared) continue;

				int xx = x + dx, yy = y + dy, zz = z + dz;
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

static void Physics_HandleTnt(int index, BlockID block) {
	int x, y, z;
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
	if (!Physics_Enabled || !World_Blocks) return;

	/*if ((tickCount % 5) == 0) {*/
	Physics_TickLava();
	Physics_TickWater();
	/*}*/
	physics_tickCount++;
	Physics_TickRandomBlocks();
}
