#include "World.h"
#include "BlockID.h"
#include "ErrorHandler.h"
#include "String.h"
#include "Platform.h"
#include "Event.h"
#include "Random.h"
#include "Block.h"
#include "Entity.h"
#include "ExtMath.h"

void World_Reset(void) {
	World_Width = 0; World_Height = 0; World_Length = 0;
	World_Blocks = NULL; World_BlocksSize = 0;

	Random rnd;
	Random_InitFromCurrentTime(&rnd);
	Int32 i;
	for (i = 0; i < 16; i++) {
		World_Uuid[i] = (UInt8)Random_Next(&rnd, 256);
	}

	/* Set version and variant bits */
	World_Uuid[6] &= 0x0F;
	World_Uuid[6] |= 0x40; /* version 4*/
	World_Uuid[8] &= 0x3F;
	World_Uuid[8] |= 0x80; /* variant 2*/
}

void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length) {
	World_Width = width; World_Height = height; World_Length = length;
	World_Blocks = blocks; World_BlocksSize = blocksSize;
	if (World_BlocksSize == 0) World_Blocks = NULL;

	if (blocksSize != (width * height * length)) {
		ErrorHandler_Fail("Blocks array size does not match volume of map");
	}

	World_OneY = width * length;
	World_MaxX = width - 1;
	World_MaxY = height - 1;
	World_MaxZ = length - 1;

	if (WorldEnv_EdgeHeight == -1) {
		WorldEnv_EdgeHeight = height / 2;
	}
	if (WorldEnv_CloudsHeight == -1) {
		WorldEnv_CloudsHeight = height + 2;
	}
}


BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z) {
	if (x < 0 || x >= World_Width || z < 0 || z >= World_Length || y < 0) return BLOCK_BEDROCK;
	if (y >= World_Height) return BLOCK_AIR;

	return World_Blocks[World_Pack(x, y, z)];
}

BlockID World_SafeGetBlock_3I(Vector3I p) {
	return World_IsValidPos(p.X, p.Y, p.Z) ? 
		World_Blocks[World_Pack(p.X, p.Y, p.Z)] : BLOCK_AIR;
}

bool World_IsValidPos(Int32 x, Int32 y, Int32 z) {
	return x >= 0 && y >= 0 && z >= 0 &&
		x < World_Width && y < World_Height && z < World_Length;
}

bool World_IsValidPos_3I(Vector3I p) {
	return p.X >= 0 && p.Y >= 0 && p.Z <= 0 &&
		p.X < World_Width && p.Y < World_Height && p.Z < World_Length;
}

Vector3I World_GetCoords(Int32 index) {
	if (index < 0 || index >= World_BlocksSize)
		return Vector3I_Create1(-1);

	Vector3I v;
	World_Unpack(index, v.X, v.Y, v.Z);
	return v;
}


/* Sets a value and potentially raises event. */
#define WorldEnv_Set(value, dst, envVar)\
if (value == dst) return;\
dst = value;\
Event_RaiseInt32(&WorldEvents_EnvVarChanged, envVar);

/* Sets a colour and potentially raises event. */
#define WorldEnv_SetCol(value, dst, envVar)\
if (PackedCol_Equals(value, dst)) return;\
dst = value;\
Event_RaiseInt32(&WorldEvents_EnvVarChanged, envVar);


void WorldEnv_Reset(void) {
	WorldEnv_DefaultSkyCol = PackedCol_Create3(0x99, 0xCC, 0xFF);
	WorldEnv_DefaultFogCol = PackedCol_Create3(0xFF, 0xFF, 0xFF);
	WorldEnv_DefaultCloudsCol = PackedCol_Create3(0xFF, 0xFF, 0xFF);

	WorldEnv_EdgeHeight = -1;
	WorldEnv_SidesOffset = -2;
	WorldEnv_CloudsHeight = -1;

	WorldEnv_EdgeBlock = BLOCK_STILL_WATER;
	WorldEnv_SidesBlock = BLOCK_BEDROCK;

	WorldEnv_CloudsSpeed = 1.0f;
	WorldEnv_WeatherSpeed = 1.0f;
	WorldEnv_WeatherFade = 1.0f;
	WorldEnv_SkyboxHorSpeed = 0.0f;
	WorldEnv_SkyboxVerSpeed = 0.0f;

	WorldEnv_ResetLight();
	WorldEnv_SkyCol = WorldEnv_DefaultSkyCol;
	WorldEnv_FogCol = WorldEnv_DefaultFogCol;
	WorldEnv_CloudsCol = WorldEnv_DefaultCloudsCol;
	WorldEnv_Weather = WEATHER_SUNNY;
	WorldEnv_ExpFog = false;
}

void WorldEnv_ResetLight(void) {
	WorldEnv_DefaultShadowCol = PackedCol_Create3(0x9B, 0x9B, 0x9B);
	WorldEnv_DefaultSunCol = PackedCol_Create3(0xFF, 0xFF, 0xFF);

	WorldEnv_ShadowCol = WorldEnv_DefaultShadowCol;
	PackedCol_GetShaded(WorldEnv_ShadowCol, &WorldEnv_ShadowXSide,
		&WorldEnv_ShadowZSide, &WorldEnv_ShadowYBottom);

	WorldEnv_SunCol = WorldEnv_DefaultSunCol;
	PackedCol_GetShaded(WorldEnv_SunCol, &WorldEnv_SunXSide,
		&WorldEnv_SunZSide, &WorldEnv_SunYBottom);
}


void WorldEnv_SetEdgeBlock(BlockID block) {
	/* some server software wrongly uses this value */
	if (block == 255 && !Block_IsCustomDefined(255)) block = BLOCK_STILL_WATER; 
	WorldEnv_Set(block, WorldEnv_EdgeBlock, ENV_VAR_EDGE_BLOCK);
}

void WorldEnv_SetSidesBlock(BlockID block) {
	/* some server software wrongly uses this value */
	if (block == 255 && !Block_IsCustomDefined(255)) block = BLOCK_BEDROCK;
	WorldEnv_Set(block, WorldEnv_SidesBlock, ENV_VAR_SIDES_BLOCK);
}

void WorldEnv_SetEdgeHeight(Int32 height) {
	WorldEnv_Set(height, WorldEnv_EdgeHeight, ENV_VAR_EDGE_HEIGHT);
}

void WorldEnv_SetSidesOffset(Int32 offset) {
	WorldEnv_Set(offset, WorldEnv_SidesOffset, ENV_VAR_SIDES_OFFSET);
}

void WorldEnv_SetCloudsHeight(Int32 height) {
	WorldEnv_Set(height, WorldEnv_CloudsHeight, ENV_VAR_CLOUDS_HEIGHT);
}

void WorldEnv_SetCloudsSpeed(Real32 speed) {
	WorldEnv_Set(speed, WorldEnv_CloudsSpeed, ENV_VAR_CLOUDS_SPEED);
}


void WorldEnv_SetWeatherSpeed(Real32 speed) {
	WorldEnv_Set(speed, WorldEnv_WeatherSpeed, ENV_VAR_WEATHER_SPEED);
}

void WorldEnv_SetWeatherFade(Real32 rate) {
	WorldEnv_Set(rate, WorldEnv_WeatherFade, ENV_VAR_WEATHER_FADE);
}

void WorldEnv_SetWeather(Int32 weather) {
	WorldEnv_Set(weather, WorldEnv_Weather, ENV_VAR_WEATHER);
}

void WorldEnv_SetExpFog(bool expFog) {
	WorldEnv_Set(expFog, WorldEnv_ExpFog, ENV_VAR_EXP_FOG);
}

void WorldEnv_SetSkyboxHorSpeed(Real32 speed) {
	WorldEnv_Set(speed, WorldEnv_SkyboxHorSpeed, ENV_VAR_SKYBOX_HOR_SPEED);
}

void WorldEnv_SetSkyboxVerSpeed(Real32 speed) {
	WorldEnv_Set(speed, WorldEnv_SkyboxVerSpeed, ENV_VAR_SKYBOX_VER_SPEED);
}


void WorldEnv_SetSkyCol(PackedCol col) {
	WorldEnv_SetCol(col, WorldEnv_SkyCol, ENV_VAR_SKY_COL);
}

void WorldEnv_SetFogCol(PackedCol col) {
	WorldEnv_SetCol(col, WorldEnv_FogCol, ENV_VAR_FOG_COL);
}

void WorldEnv_SetCloudsCol(PackedCol col) {
	WorldEnv_SetCol(col, WorldEnv_CloudsCol, ENV_VAR_CLOUDS_COL);
}

void WorldEnv_SetSunCol(PackedCol col) {
	if (PackedCol_Equals(col, WorldEnv_SunCol)) return;

	WorldEnv_SunCol = col;
	PackedCol_GetShaded(WorldEnv_SunCol, &WorldEnv_SunXSide,
		&WorldEnv_SunZSide, &WorldEnv_SunYBottom);
	Event_RaiseInt32(&WorldEvents_EnvVarChanged, ENV_VAR_SUN_COL);
}

void WorldEnv_SetShadowCol(PackedCol col) {
	if (PackedCol_Equals(col, WorldEnv_ShadowCol)) return;

	WorldEnv_ShadowCol = col;
	PackedCol_GetShaded(WorldEnv_ShadowCol, &WorldEnv_ShadowXSide,
		&WorldEnv_ShadowZSide, &WorldEnv_ShadowYBottom);
	Event_RaiseInt32(&WorldEvents_EnvVarChanged, ENV_VAR_SHADOW_COL);
}

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

				if (Block_Collide[block] != COLLIDE_SOLID) continue;
				if (!AABB_Intersects(bb, &blockBB)) continue;
				if (blockBB.Max.Y > spawnY) blockBB.Max.Y = spawnY;
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
		bb.Min.Y -= 1.0f; bb.Max.Y -= 1.0f;
	}
	return spawn;
}