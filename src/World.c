#include "World.h"
#include "Logger.h"
#include "String.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "Entity.h"
#include "ExtMath.h"
#include "Physics.h"
#include "Game.h"
#include "TexturePack.h"
#include "Window.h"

struct _WorldData World;
/*########################################################################################################################*
*----------------------------------------------------------World----------------------------------------------------------*
*#########################################################################################################################*/
static void GenerateNewUuid(void) {
	RNGState rnd;
	int i;
	Random_SeedFromCurrentTime(&rnd);

	/* seed a bit more randomness for uuid */
	for (i = 0; i < Game_Username.length; i++) {
		Random_Next(&rnd, Game_Username.buffer[i] + 3);
	}

	for (i = 0; i < WORLD_UUID_LEN; i++) {
		World.Uuid[i] = Random_Next(&rnd, 256);
	}

	/* Set version and variant bits */
	World.Uuid[6] &= 0x0F;
	World.Uuid[6] |= 0x40; /* version 4*/
	World.Uuid[8] &= 0x3F;
	World.Uuid[8] |= 0x80; /* variant 2*/
}

void World_Reset(void) {
#ifdef EXTENDED_BLOCKS
	if (World.Blocks != World.Blocks2) Mem_Free(World.Blocks2);
	World.Blocks2 = NULL;
	World.IDMask  = 0xFF;
#endif
	Mem_Free(World.Blocks);
	World.Blocks = NULL;

	World_SetDimensions(0, 0, 0);
	World.Loaded = false;
	Env_Reset();
}

void World_NewMap(void) {
	World_Reset();
	Event_RaiseVoid(&WorldEvents.NewMap);
}

void World_SetNewMap(BlockRaw* blocks, int width, int height, int length) {
	/* TODO: TEMP HACK */
	if (!blocks) { width = 0; height = 0; length = 0; }

	World_SetDimensions(width, height, length);
	World.Blocks = blocks;

	if (!World.Volume) World.Blocks = NULL;
#ifdef EXTENDED_BLOCKS
	/* .cw maps may have set this to a non-NULL when importing */
	if (!World.Blocks2) {
		World.Blocks2 = World.Blocks;
		World.IDMask  = 0xFF;
	}
#endif

	if (Env.EdgeHeight == -1)   { Env.EdgeHeight   = height / 2; }
	if (Env.CloudsHeight == -1) { Env.CloudsHeight = height + 2; }

	GenerateNewUuid();
	World.Loaded = true;
	Event_RaiseVoid(&WorldEvents.MapLoaded);
}

CC_NOINLINE void World_SetDimensions(int width, int height, int length) {
	World.Width  = width; World.Height = height; World.Length = length;
	World.Volume = width * height * length;

	World.OneY = width * length;
	World.MaxX = width  - 1;
	World.MaxY = height - 1;
	World.MaxZ = length - 1;
}

#ifdef EXTENDED_BLOCKS
void World_SetMapUpper(BlockRaw* blocks) {
	World.Blocks2 = blocks;
	World.IDMask  = 0x3FF;
}
#endif


#ifdef EXTENDED_BLOCKS
static CC_NOINLINE void LazyInitUpper(int i, BlockID block) {
	BlockRaw* data = (BlockRaw*)Mem_TryAllocCleared(World.Volume, 1);
	if (data) {
		World_SetMapUpper(data);
		World.Blocks2[i] = (BlockRaw)(block >> 8);
	} else {
		Window_ShowDialog("Out of memory", "Not enough free memory to load the map.\nTry joining a different map.");
		World_Reset();
	}
}

void World_SetBlock(int x, int y, int z, BlockID block) {
	int i = World_Pack(x, y, z);
	World.Blocks[i] = (BlockRaw)block;

	/* defer allocation of second map array if possible */
	if (World.Blocks == World.Blocks2) {
		if (block < 256) return;
		LazyInitUpper(i, block);
		return;
	}
	World.Blocks2[i] = (BlockRaw)(block >> 8);
}
#else
void World_SetBlock(int x, int y, int z, BlockID block) {
	World.Blocks[World_Pack(x, y, z)] = block; 
}
#endif

BlockID World_GetPhysicsBlock(int x, int y, int z) {
	if (y < 0 || !World_ContainsXZ(x, z)) return BLOCK_BEDROCK;
	if (y >= World.Height) return BLOCK_AIR;

	return World_GetBlock(x, y, z);
}

BlockID World_SafeGetBlock(int x, int y, int z) {
	return World_Contains(x, y, z) ? World_GetBlock(x, y, z) : BLOCK_AIR;
}


/*########################################################################################################################*
*-------------------------------------------------------Environment-------------------------------------------------------*
*#########################################################################################################################*/
#define Env_Set(src, dst, var) \
if (src != dst) { dst = src; Event_RaiseInt(&WorldEvents.EnvVarChanged, var); }

struct _EnvData Env;
const char* const Weather_Names[3] = { "Sunny", "Rainy", "Snowy" };

void Env_Reset(void) {
	Env.EdgeHeight   = -1;
	Env.SidesOffset  = -2;
	Env.CloudsHeight = -1;

	Env.EdgeBlock  = BLOCK_STILL_WATER;
	Env.SidesBlock = BLOCK_BEDROCK;

	Env.CloudsSpeed    = 1.0f;
	Env.WeatherSpeed   = 1.0f;
	Env.WeatherFade    = 1.0f;
	Env.SkyboxHorSpeed = 0.0f;
	Env.SkyboxVerSpeed = 0.0f;

	Env.ShadowCol = ENV_DEFAULT_SHADOW_COL;
	PackedCol_GetShaded(Env.ShadowCol, &Env.ShadowXSide,
		&Env.ShadowZSide, &Env.ShadowYMin);

	Env.SunCol = ENV_DEFAULT_SUN_COL;
	PackedCol_GetShaded(Env.SunCol, &Env.SunXSide,
		&Env.SunZSide, &Env.SunYMin);

	Env.SkyCol    = ENV_DEFAULT_SKY_COL;
	Env.FogCol    = ENV_DEFAULT_FOG_COL;
	Env.CloudsCol = ENV_DEFAULT_CLOUDS_COL;
	Env.SkyboxCol = ENV_DEFAULT_SKYBOX_COL;
	Env.Weather   = WEATHER_SUNNY;
	Env.ExpFog    = false;
}


void Env_SetEdgeBlock(BlockID block) {
	/* some server software wrongly uses this value */
	if (block == 255 && !Block_IsCustomDefined(255)) block = BLOCK_STILL_WATER; 
	Env_Set(block, Env.EdgeBlock, ENV_VAR_EDGE_BLOCK);
}
void Env_SetSidesBlock(BlockID block) {
	/* some server software wrongly uses this value */
	if (block == 255 && !Block_IsCustomDefined(255)) block = BLOCK_BEDROCK;
	Env_Set(block, Env.SidesBlock, ENV_VAR_SIDES_BLOCK);
}

void Env_SetEdgeHeight(int height) {
	Env_Set(height, Env.EdgeHeight, ENV_VAR_EDGE_HEIGHT);
}
void Env_SetSidesOffset(int offset) {
	Env_Set(offset, Env.SidesOffset, ENV_VAR_SIDES_OFFSET);
}
void Env_SetCloudsHeight(int height) {
	Env_Set(height, Env.CloudsHeight, ENV_VAR_CLOUDS_HEIGHT);
}
void Env_SetCloudsSpeed(float speed) {
	Env_Set(speed, Env.CloudsSpeed, ENV_VAR_CLOUDS_SPEED);
}

void Env_SetWeatherSpeed(float speed) {
	Env_Set(speed, Env.WeatherSpeed, ENV_VAR_WEATHER_SPEED);
}
void Env_SetWeatherFade(float rate) {
	Env_Set(rate, Env.WeatherFade, ENV_VAR_WEATHER_FADE);
}
void Env_SetWeather(int weather) {
	Env_Set(weather, Env.Weather, ENV_VAR_WEATHER);
}
void Env_SetExpFog(cc_bool expFog) {
	Env_Set(expFog, Env.ExpFog, ENV_VAR_EXP_FOG);
}
void Env_SetSkyboxHorSpeed(float speed) {
	Env_Set(speed, Env.SkyboxHorSpeed, ENV_VAR_SKYBOX_HOR_SPEED);
}
void Env_SetSkyboxVerSpeed(float speed) {
	Env_Set(speed, Env.SkyboxVerSpeed, ENV_VAR_SKYBOX_VER_SPEED);
}

void Env_SetSkyCol(PackedCol col) {
	Env_Set(col, Env.SkyCol, ENV_VAR_SKY_COL);
}
void Env_SetFogCol(PackedCol col) {
	Env_Set(col, Env.FogCol, ENV_VAR_FOG_COL);
}
void Env_SetCloudsCol(PackedCol col) {
	Env_Set(col, Env.CloudsCol, ENV_VAR_CLOUDS_COL);
}
void Env_SetSkyboxCol(PackedCol col) {
	Env_Set(col, Env.SkyboxCol, ENV_VAR_SKYBOX_COL);
}

void Env_SetSunCol(PackedCol col) {
	PackedCol_GetShaded(col, &Env.SunXSide, &Env.SunZSide, &Env.SunYMin);
	Env_Set(col, Env.SunCol, ENV_VAR_SUN_COL);
}
void Env_SetShadowCol(PackedCol col) {
	PackedCol_GetShaded(col, &Env.ShadowXSide, &Env.ShadowZSide, &Env.ShadowYMin);
	Env_Set(col, Env.ShadowCol, ENV_VAR_SHADOW_COL);
}


/*########################################################################################################################*
*-------------------------------------------------------Respawning--------------------------------------------------------*
*#########################################################################################################################*/
float Respawn_HighestSolidY(struct AABB* bb) {
	int minX = Math_Floor(bb->Min.X), maxX = Math_Floor(bb->Max.X);
	int minY = Math_Floor(bb->Min.Y), maxY = Math_Floor(bb->Max.Y);
	int minZ = Math_Floor(bb->Min.Z), maxZ = Math_Floor(bb->Max.Z);
	float highestY = RESPAWN_NOT_FOUND;

	BlockID block;
	struct AABB blockBB;
	Vec3 v;
	int x, y, z;	

	for (y = minY; y <= maxY; y++) { v.Y = (float)y;
		for (z = minZ; z <= maxZ; z++) { v.Z = (float)z;
			for (x = minX; x <= maxX; x++) { v.X = (float)x;

				/* TODO: Maybe use how picking gets blocks, so the bedrock */
				/* just below and just on borders of the map is treated as such */
				/* Not sure if this is really necessary though, it seems to work */
				/* just fine already when you're standing on the bottom of the map. */
				block = World_SafeGetBlock(x, y, z);
				Vec3_Add(&blockBB.Min, &v, &Blocks.MinBB[block]);
				Vec3_Add(&blockBB.Max, &v, &Blocks.MaxBB[block]);

				if (Blocks.Collide[block] != COLLIDE_SOLID) continue;
				if (!AABB_Intersects(bb, &blockBB)) continue;
				if (blockBB.Max.Y > highestY) highestY = blockBB.Max.Y;
			}
		}
	}
	return highestY;
}

Vec3 Respawn_FindSpawnPosition(float x, float z, Vec3 modelSize) {
	Vec3 spawn = Vec3_Create3(x, 0.0f, z);
	struct AABB bb;
	float highestY;
	int y;

	spawn.Y = World.Height + ENTITY_ADJUSTMENT;
	AABB_Make(&bb, &spawn, &modelSize);
	spawn.Y = 0.0f;
	
	for (y = World.Height; y >= 0; y--) {
		highestY = Respawn_HighestSolidY(&bb);
		if (highestY != RESPAWN_NOT_FOUND) {
			spawn.Y = highestY; break;
		}
		bb.Min.Y -= 1.0f; bb.Max.Y -= 1.0f;
	}
	return spawn;
}
