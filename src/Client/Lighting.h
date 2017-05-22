#ifndef CS_WORLDLIGHTING_H
#define CS_WORLDLIGHTING_H
#include "Typedefs.h"
#include "PackedCol.h"
/* Manages lighting of blocks in the world.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Int16* Lighting_heightmap;
PackedCol Lighting_Outside;
PackedCol Lighting_OutsideZSide;
PackedCol Lighting_OutsideXSide;
PackedCol Lighting_OutsideYBottom;


/* Equivalent to (but far more optimised form of)
* for x = startX; x < startX + 18; x++
*   for z = startZ; z < startZ + 18; z++
*      CalcLight(x, maxY, z)                         */
void Lighting_LightHint(Int32 startX, Int32 startZ);

/* Called when a block is changed, to update the lighting information.
NOTE: Implementations ***MUST*** mark all chunks affected by this lighting changeas needing to be refreshed. */
void Lighting_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);

/* Discards all cached lighting information. */
void Lighting_Refresh();


/* Returns whether the block at the given coordinates is fully in sunlight.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
bool Lighting_IsLit(Int32 x, Int32 y, Int32 z);

/* Returns the light colour of the block at the given coordinates.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col(Int32 x, Int32 y, Int32 z);

/* Returns the light colour of the block at the given coordinates.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col_ZSide(Int32 x, Int32 y, Int32 z);


PackedCol LightCol_Sprite_Fast(Int32 x, Int32 y, Int32 z);
PackedCol LightCol_YTop_Fast(Int32 x, Int32 y, Int32 z);
PackedCol LightCol_YBottom_Fast(Int32 x, Int32 y, Int32 z);
PackedCol LightCol_XSide_Fast(Int32 x, Int32 y, Int32 z);
PackedCol LightCol_ZSide_Fast(Int32 x, Int32 y, Int32 z);

static void Lighting_Free();
static void Lighting_Reset();
static void Lighting_OnNewMap();
static void Lighting_OnNewMapLoaded();
static void Lighting_Init();
#endif