#ifndef CC_WORLDLIGHTING_H
#define CC_WORLDLIGHTING_H
#include "PackedCol.h"
/*
Abstracts lighting of blocks in the world
  Built-in lighting engines:
  - ClassicLighting: Uses a simple heightmap, where each block is either in sun or shadow

Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Lighting_Component;
/* Whether MC-style 16-level lighting should be used. */
extern cc_bool Lighting_Modern;
/* How much ambient occlusion to apply in modern lighting where 1.0f = none and 0.0f = maximum*/
#define MODERN_AO 0.5F
/* How many unique "levels" of light there are when modern lighting is used. */
#define MODERN_LIGHTING_LEVELS 16
#define MODERN_LIGHTING_MAX_LEVEL MODERN_LIGHTING_LEVELS - 1
/* How many bits to shift sunlight level to the left when storing it in a byte along with blocklight level. */
#define MODERN_LIGHTING_SUN_SHIFT 4
#define SUN_LEVELS 16
/* A byte that fills the sun level area with ones. Equivalent to 0b_1111_0000 */
#define MODERN_LIGHTING_SUN_MASK 0xF0

CC_VAR extern struct _Lighting {
	/* Releases/Frees the per-level lighting state */
	void (*FreeState)(void);
	/* Allocates the per-level lighting state */
	/*  (called after map has been fully loaded) */
	void (*AllocState)(void);
	/* Equivalent to (but far more optimised form of)
	* for x = startX; x < startX + 18; x++
	*   for z = startZ; z < startZ + 18; z++
	*      CalcLight(x, maxY, z)                         */
	void (*LightHint)(int startX, int startZ);

	/* Called when a block is changed to update internal lighting state. */
	/* NOTE: Implementations ***MUST*** mark all chunks affected by this lighting change as needing to be refreshed. */
	void (*OnBlockChanged)(int x, int y, int z, BlockID oldBlock, BlockID newBlock);
	/* Invalidates/Resets lighting state for all of the blocks in the world */
	/*  (e.g. because a block changed whether it is full bright or not) */
	void (*Refresh)(void);

	/* Returns whether the block at the given coordinates is fully in sunlight. */
	/* NOTE: Does ***NOT*** check that the coordinates are inside the map. */
	cc_bool   (*IsLit)(int x, int y, int z);
	/* Returns the light colour at the given coordinates. */
	PackedCol (*Color)(int x, int y, int z);
	/* Returns the light colour at the given coordinates. */
	PackedCol (*Color_XSide)(int x, int y, int z);

	/* _Fast functions assume Lighting_LightHint has already been called */
	/* and performed the necessary calculations for the given x/z coords */
	/* _Fast functions also do NOT check coordinates are inside the map */

	cc_bool   (*IsLit_Fast)(int x, int y, int z);
	PackedCol (*Color_Sprite_Fast)(int x, int y, int z);
	PackedCol (*Color_YMax_Fast)(int x, int y, int z);
	PackedCol (*Color_YMin_Fast)(int x, int y, int z);
	PackedCol (*Color_XSide_Fast)(int x, int y, int z);
	PackedCol (*Color_ZSide_Fast)(int x, int y, int z);
} Lighting;

void Lighting_SwitchActive(void);
void Lighting_ApplyActive(void);
void ModernLighting_SetActive(void);
void ModernLighting_OnEnvVariableChanged(void* obj, int envVar);

void ClassicLighting_FreeState(void);
void ClassicLighting_AllocState(void);
int ClassicLighting_GetLightHeight(int x, int z);
void ClassicLighting_LightHint(int startX, int startZ);
void ClassicLighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock);
#endif
