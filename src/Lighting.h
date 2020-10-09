#ifndef CC_WORLDLIGHTING_H
#define CC_WORLDLIGHTING_H
#include "PackedCol.h"
/* Manages lighting of blocks in the world.
BasicLighting: Uses a simple heightmap, where each block is either in sun or shadow.
   Copyright 2014-2020 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Lighting_Component;

#define Lighting_Pack(x, z) ((x) + World.Width * (z))
extern cc_int16* Lighting_Heightmap;

/* Equivalent to (but far more optimised form of)
* for x = startX; x < startX + 18; x++
*   for z = startZ; z < startZ + 18; z++
*      CalcLight(x, maxY, z)                         */
void Lighting_LightHint(int startX, int startZ);

/* Called when a block is changed to update internal lighting state. */
/* NOTE: Implementations ***MUST*** mark all chunks affected by this lighting change as needing to be refreshed. */
void Lighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock);
void Lighting_Refresh(void);

/* Returns whether the block at the given coordinates is fully in sunlight. */
/* NOTE: Does ***NOT*** check that the coordinates are inside the map. */
cc_bool Lighting_IsLit(int x, int y, int z);
/* Returns the light colour of the block at the given coordinates. */
/* NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col(int x, int y, int z);
/* Returns the light colour of the block at the given coordinates. */
/* NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col_XSide(int x, int y, int z);

PackedCol Lighting_Col_Sprite_Fast(int x, int y, int z);
PackedCol Lighting_Col_YMax_Fast(int x, int y, int z);
PackedCol Lighting_Col_YMin_Fast(int x, int y, int z);
PackedCol Lighting_Col_XSide_Fast(int x, int y, int z);
PackedCol Lighting_Col_ZSide_Fast(int x, int y, int z);
#endif
