#ifndef CC_WORLDLIGHTING_H
#define CC_WORLDLIGHTING_H
#include "Typedefs.h"
#include "PackedCol.h"
#include "Event.h"
#include "GameStructs.h"
/* Manages lighting of blocks in the world.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

PackedCol Lighting_Outside;
PackedCol Lighting_OutsideZSide;
PackedCol Lighting_OutsideXSide;
PackedCol Lighting_OutsideYBottom;

IGameComponent Lighting_MakeGameComponent(void);
/* Equivalent to (but far more optimised form of)
* for x = startX; x < startX + 18; x++
*   for z = startZ; z < startZ + 18; z++
*      CalcLight(x, maxY, z)                         */
void Lighting_LightHint(Int32 startX, Int32 startZ);

/* Called when a block is changed, to update the lighting information.
NOTE: Implementations ***MUST*** mark all chunks affected by this lighting changeas needing to be refreshed. */
void Lighting_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock);
void Lighting_Refresh(void);

/* Returns whether the block at the given coordinates is fully in sunlight.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
bool Lighting_IsLit(Int32 x, Int32 y, Int32 z);
/* Returns the light colour of the block at the given coordinates.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col(Int32 x, Int32 y, Int32 z);
/* Returns the light colour of the block at the given coordinates.
NOTE: Does ***NOT*** check that the coordinates are inside the map. */
PackedCol Lighting_Col_XSide(Int32 x, Int32 y, Int32 z);

PackedCol Lighting_Col_Sprite_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_YTop_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_YBottom_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_XSide_Fast(Int32 x, Int32 y, Int32 z);
PackedCol Lighting_Col_ZSide_Fast(Int32 x, Int32 y, Int32 z);
#endif