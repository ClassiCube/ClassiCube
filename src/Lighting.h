#ifndef CC_WORLDLIGHTING_H
#define CC_WORLDLIGHTING_H
#include "PackedCol.h"
/* Manages lighting of blocks in the world.
BasicLighting: Uses a simple heightmap, where each block is either in sun or shadow.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Lighting_Component;
/* Whether MC-style 16-level lighting should be used. */
extern cc_bool Lighting_Modern;

#define Lighting_Pack(x, z) ((x) + World.Width * (z))


CC_VAR extern struct _Lighting {
	/* Equivalent to (but far more optimised form of)
	* for x = startX; x < startX + 18; x++
	*   for z = startZ; z < startZ + 18; z++
	*      CalcLight(x, maxY, z)                         */
	void (*LightHint)(int startX, int startZ);

	/* Called when a block is changed to update internal lighting state. */
	/* NOTE: Implementations ***MUST*** mark all chunks affected by this lighting change as needing to be refreshed. */
	void (*OnBlockChanged)(int x, int y, int z, BlockID oldBlock, BlockID newBlock);
	void (*Refresh)(void);

	/* Returns whether the block at the given coordinates is fully in sunlight. */
	/* NOTE: Does ***NOT*** check that the coordinates are inside the map. */
	cc_bool(*IsLit)(int x, int y, int z);
	/* Returns the light colour at the given coordinates. */
	PackedCol(*Color)(int x, int y, int z);
	/* Returns the light colour at the given coordinates. */
	PackedCol(*Color_XSide)(int x, int y, int z);

	/* _Fast functions assume Lighting_LightHint has already been called */
	/* and performed the necessary calculations for the given x/z coords */
	/* _Fast functions also do NOT check coordinates are inside the map */

	cc_bool(*IsLit_Fast)(int x, int y, int z);
	PackedCol (*Color_Sprite_Fast)(int x, int y, int z);
	PackedCol (*Color_YMax_Fast)(int x, int y, int z);
	PackedCol (*Color_YMin_Fast)(int x, int y, int z);
	PackedCol (*Color_XSide_Fast)(int x, int y, int z);
	PackedCol (*Color_ZSide_Fast)(int x, int y, int z);
} Lighting;

void Lighting_ApplyActive(void);
#endif
