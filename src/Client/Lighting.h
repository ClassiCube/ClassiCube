#ifndef CS_WORLDLIGHTING_H
#define CS_WORLDLIGHTING_H
#include "Typedefs.h"
#include "PackedCol.h"
#include "WorldEvents.h"
#include "GameStructs.h"
/* Manages lighting of blocks in the world.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Int16* Lighting_heightmap;
PackedCol Lighting_Outside;
PackedCol Lighting_OutsideZSide;
PackedCol Lighting_OutsideXSide;
PackedCol Lighting_OutsideYBottom;


/* Creates game component implementation. */
IGameComponent Lighting_MakeGameComponent(void);

/* Equivalent to (but far more optimised form of)
* for x = startX; x < startX + 18; x++
*   for z = startZ; z < startZ + 18; z++
*      CalcLight(x, maxY, z)                         */
void Lighting_LightHint(Int32 startX, Int32 startZ);

/* Called when a block is changed, to update the lighting information.
NOTE: Implementations ***MUST*** mark all chunks affected by this lighting changeas needing to be refreshed. */
void Lighting_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);

/* Discards all cached lighting information. */
void Lighting_Refresh(void);


/* Returns whether the block at the given coordinates is fully in sunlight.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
bool Lighting_IsLit(Int32 x, Int32 y, Int32 z);

/* Returns the light colour of the block at the given coordinates.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col(Int32 x, Int32 y, Int32 z);

/* Returns the light colour of the block at the given coordinates.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col_ZSide(Int32 x, Int32 y, Int32 z);


PackedCol Lighting_Col_Sprite_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_YTop_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_YBottom_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_XSide_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_ZSide_Fast(Int32 x, Int32 y, Int32 z);


static void Lighting_Free(void);
static void Lighting_Reset(void);
static void Lighting_OnNewMap(void);
static void Lighting_OnNewMapLoaded(void);
static void Lighting_Init(void);


static void Lighting_EnvVariableChanged(EnvVar envVar);

static void Lighting_SetSun(PackedCol col);

static void Lighting_SetShadow(PackedCol col);

static Int32 Lighting_GetLightHeight(Int32 x, Int32 z);


static void Lighting_UpdateLighting(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock, Int32 index, Int32 lightH);

static void Lighting_RefreshAffected(Int32 x, Int32 y, Int32 z, BlockID block, Int32 oldHeight, Int32 newHeight);

static bool Lighting_Needs(BlockID block, BlockID other);

static void Lighting_ResetNeighbour(Int32 x, Int32 y, Int32 z, BlockID block,
	Int32 cx, Int32 cy, Int32 cz, Int32 minCy, Int32 maxCy);

static void Lighting_ResetNeighourChunk(Int32 cx, Int32 cy, Int32 cz, BlockID block, Int32 y, Int32 index, Int32 nY);

static void Lighting_ResetColumn(Int32 cx, Int32 cy, Int32 cz, Int32 minCy, Int32 maxCy);


static Int32 Lighting_CalcHeightAt(Int32 x, Int32 maxY, Int32 z, Int32 index);

static Int32 Lighting_InitialHeightmapCoverage(Int32 x1, Int32 z1, Int32 xCount, Int32 zCount, Int32* skip);

static bool Lighting_CalculateHeightmapCoverage(Int32 x1, Int32 z1, Int32 xCount, Int32 zCount, Int32 elemsLeft, Int32* skip);

static void Lighting_FinishHeightmapCoverage(Int32 x1, Int32 z1, Int32 xCount, Int32 zCount);
#endif