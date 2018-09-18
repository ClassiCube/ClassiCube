#include "World.h"
#include "BlockID.h"
#include "ErrorHandler.h"
#include "String.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "Entity.h"
#include "ExtMath.h"
#include "Physics.h"
#include "Game.h"

void World_Reset(void) {
	Mem_Free(World_Blocks);
	World_Width = 0; World_Height = 0; World_Length = 0;
	World_MaxX = 0;  World_MaxY = 0;   World_MaxZ = 0;
	World_Blocks = NULL; World_BlocksSize = 0;
	Env_Reset();

	Random rnd;
	Random_InitFromCurrentTime(&rnd);
	/* add a bit of randomness for uuid */
	Int32 i;
	for (i = 0; i < Game_Username.length; i++) {
		Random_Next(&rnd, Game_Username.buffer[i] + 3);
	}

	for (i = 0; i < 16; i++) {
		World_Uuid[i] = Random_Next(&rnd, 256);
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
	if (!World_BlocksSize) World_Blocks = NULL;

	if (blocksSize != (width * height * length)) {
		ErrorHandler_Fail("Blocks array size does not match volume of map");
	}

	World_OneY = width * length;
	World_MaxX = width - 1;
	World_MaxY = height - 1;
	World_MaxZ = length - 1;

	if (Env_EdgeHeight == -1) {
		Env_EdgeHeight = height / 2;
	}
	if (Env_CloudsHeight == -1) {
		Env_CloudsHeight = height + 2;
	}
}


BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z) {
	if (x < 0 || x >= World_Width || z < 0 || z >= World_Length || y < 0) return BLOCK_BEDROCK;
	if (y >= World_Height) return BLOCK_AIR;

	return World_GetBlock(x, y, z);
}

BlockID World_SafeGetBlock_3I(Vector3I p) {
	return World_IsValidPos(p.X, p.Y, p.Z) ? World_GetBlock(p.X, p.Y, p.Z) : BLOCK_AIR;
}

bool World_IsValidPos(Int32 x, Int32 y, Int32 z) {
	return x >= 0 && y >= 0 && z >= 0 && 
		x < World_Width && y < World_Height && z < World_Length;
}

bool World_IsValidPos_3I(Vector3I p) {
	return p.X >= 0 && p.Y >= 0 && p.Z >= 0 &&
		p.X < World_Width && p.Y < World_Height && p.Z < World_Length;
}


#define Env_Set(src, dst, var) \
if (src != dst) { dst = src; Event_RaiseInt(&WorldEvents_EnvVarChanged, var); }

#define Env_SetCol(src, dst, var)\
if (!PackedCol_Equals(src, dst)) { dst = src; Event_RaiseInt(&WorldEvents_EnvVarChanged, var); }

const char* Weather_Names[3] = { "Sunny", "Rainy", "Snowy" };

PackedCol Env_DefaultSkyCol    = PACKEDCOL_CONST(0x99, 0xCC, 0xFF, 0xFF);
PackedCol Env_DefaultFogCol    = PACKEDCOL_CONST(0xFF, 0xFF, 0xFF, 0xFF);
PackedCol Env_DefaultCloudsCol = PACKEDCOL_CONST(0xFF, 0xFF, 0xFF, 0xFF);
PackedCol Env_DefaultSunCol    = PACKEDCOL_CONST(0xFF, 0xFF, 0xFF, 0xFF);
PackedCol Env_DefaultShadowCol = PACKEDCOL_CONST(0x9B, 0x9B, 0x9B, 0xFF);
char World_TextureUrlBuffer[STRING_SIZE];
String World_TextureUrl = String_FromArray(World_TextureUrlBuffer);

void Env_Reset(void) {
	Env_EdgeHeight   = -1;
	Env_SidesOffset  = -2;
	Env_CloudsHeight = -1;

	Env_EdgeBlock  = BLOCK_STILL_WATER;
	Env_SidesBlock = BLOCK_BEDROCK;

	Env_CloudsSpeed    = 1.0f;
	Env_WeatherSpeed   = 1.0f;
	Env_WeatherFade    = 1.0f;
	Env_SkyboxHorSpeed = 0.0f;
	Env_SkyboxVerSpeed = 0.0f;

	Env_ResetLight();
	Env_SkyCol    = Env_DefaultSkyCol;
	Env_FogCol    = Env_DefaultFogCol;
	Env_CloudsCol = Env_DefaultCloudsCol;
	Env_Weather = WEATHER_SUNNY;
	Env_ExpFog = false;
}

void Env_ResetLight(void) {
	Env_ShadowCol = Env_DefaultShadowCol;
	PackedCol_GetShaded(Env_ShadowCol, &Env_ShadowXSide,
		&Env_ShadowZSide, &Env_ShadowYMin);

	Env_SunCol = Env_DefaultSunCol;
	PackedCol_GetShaded(Env_SunCol, &Env_SunXSide,
		&Env_SunZSide, &Env_SunYMin);
}


void Env_SetEdgeBlock(BlockID block) {
	/* some server software wrongly uses this value */
	if (block == 255 && !Block_IsCustomDefined(255)) block = BLOCK_STILL_WATER; 
	Env_Set(block, Env_EdgeBlock, ENV_VAR_EDGE_BLOCK);
}

void Env_SetSidesBlock(BlockID block) {
	/* some server software wrongly uses this value */
	if (block == 255 && !Block_IsCustomDefined(255)) block = BLOCK_BEDROCK;
	Env_Set(block, Env_SidesBlock, ENV_VAR_SIDES_BLOCK);
}

void Env_SetEdgeHeight(Int32 height) {
	Env_Set(height, Env_EdgeHeight, ENV_VAR_EDGE_HEIGHT);
}

void Env_SetSidesOffset(Int32 offset) {
	Env_Set(offset, Env_SidesOffset, ENV_VAR_SIDES_OFFSET);
}

void Env_SetCloudsHeight(Int32 height) {
	Env_Set(height, Env_CloudsHeight, ENV_VAR_CLOUDS_HEIGHT);
}

void Env_SetCloudsSpeed(Real32 speed) {
	Env_Set(speed, Env_CloudsSpeed, ENV_VAR_CLOUDS_SPEED);
}


void Env_SetWeatherSpeed(Real32 speed) {
	Env_Set(speed, Env_WeatherSpeed, ENV_VAR_WEATHER_SPEED);
}

void Env_SetWeatherFade(Real32 rate) {
	Env_Set(rate, Env_WeatherFade, ENV_VAR_WEATHER_FADE);
}

void Env_SetWeather(Int32 weather) {
	Env_Set(weather, Env_Weather, ENV_VAR_WEATHER);
}

void Env_SetExpFog(bool expFog) {
	Env_Set(expFog, Env_ExpFog, ENV_VAR_EXP_FOG);
}

void Env_SetSkyboxHorSpeed(Real32 speed) {
	Env_Set(speed, Env_SkyboxHorSpeed, ENV_VAR_SKYBOX_HOR_SPEED);
}

void Env_SetSkyboxVerSpeed(Real32 speed) {
	Env_Set(speed, Env_SkyboxVerSpeed, ENV_VAR_SKYBOX_VER_SPEED);
}


void Env_SetSkyCol(PackedCol col) {
	Env_SetCol(col, Env_SkyCol, ENV_VAR_SKY_COL);
}

void Env_SetFogCol(PackedCol col) {
	Env_SetCol(col, Env_FogCol, ENV_VAR_FOG_COL);
}

void Env_SetCloudsCol(PackedCol col) {
	Env_SetCol(col, Env_CloudsCol, ENV_VAR_CLOUDS_COL);
}

void Env_SetSunCol(PackedCol col) {
	PackedCol_GetShaded(col, &Env_SunXSide, &Env_SunZSide, &Env_SunYMin);
	Env_SetCol(col, Env_SunCol, ENV_VAR_SUN_COL);
}

void Env_SetShadowCol(PackedCol col) {
	PackedCol_GetShaded(col, &Env_ShadowXSide, &Env_ShadowZSide, &Env_ShadowYMin);
	Env_SetCol(col, Env_ShadowCol, ENV_VAR_SHADOW_COL);
}

Real32 Respawn_HighestFreeY(struct AABB* bb) {
	Int32 minX = Math_Floor(bb->Min.X), maxX = Math_Floor(bb->Max.X);
	Int32 minY = Math_Floor(bb->Min.Y), maxY = Math_Floor(bb->Max.Y);
	Int32 minZ = Math_Floor(bb->Min.Z), maxZ = Math_Floor(bb->Max.Z);

	Real32 spawnY = RESPAWN_NOT_FOUND;
	struct AABB blockBB;
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
				if (blockBB.Max.Y > spawnY) spawnY = blockBB.Max.Y;
			}
		}
	}
	return spawnY;
}

Vector3 Respawn_FindSpawnPosition(Real32 x, Real32 z, Vector3 modelSize) {
	Vector3 spawn = Vector3_Create3(x, 0.0f, z);
	spawn.Y = World_Height + ENTITY_ADJUSTMENT;
	struct AABB bb;
	AABB_Make(&bb, &spawn, &modelSize);
	spawn.Y = 0.0f;

	Int32 y;
	for (y = World_Height; y >= 0; y--) {
		Real32 highestY = Respawn_HighestFreeY(&bb);
		if (highestY != RESPAWN_NOT_FOUND) {
			spawn.Y = highestY; break;
		}
		bb.Min.Y -= 1.0f; bb.Max.Y -= 1.0f;
	}
	return spawn;
}
