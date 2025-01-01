#ifndef CC_WORLDLIGHTING_H
#define CC_WORLDLIGHTING_H
#include "PackedCol.h"
CC_BEGIN_HEADER

/*
Abstracts lighting of blocks in the world
  Built-in lighting engines:
  - ClassicLighting: Uses a simple heightmap, where each block is either in sun or shadow

Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Lighting_Component;

enum LightingMode {
	LIGHTING_MODE_CLASSIC, LIGHTING_MODE_FANCY, LIGHTING_MODE_COUNT
};
extern const char* const LightingMode_Names[LIGHTING_MODE_COUNT];
extern cc_uint8 Lighting_Mode;

extern cc_bool Lighting_ModeLockedByServer;
/* True if the current lighting mode has been set by the server instead of the client */
extern cc_bool Lighting_ModeSetByServer;
/* The lighting mode that was set by the client before being set by the server */
extern cc_uint8 Lighting_ModeUserCached;
void Lighting_SetMode(cc_uint8 mode, cc_bool fromServer);


/* How much ambient occlusion to apply in fancy lighting where 1.0f = none and 0.0f = maximum*/
#define FANCY_AO 0.5F
/* How many unique "levels" of light there are when fancy lighting is used. */
#define FANCY_LIGHTING_LEVELS 16
#define FANCY_LIGHTING_MAX_LEVEL (FANCY_LIGHTING_LEVELS - 1)
/* How many bits to shift lamplight level to the left when storing it in a byte along with lavalight level. */
#define FANCY_LIGHTING_LAMP_SHIFT 4
/* A byte that fills the lamp level area with ones. Equivalent to 0b_1111_0000 */
#define FANCY_LIGHTING_LAMP_MASK 0xF0

CC_VAR extern struct _Lighting {
	/* Releases/Frees the per-level lighting state */
	void (*FreeState)(void);
	/* Allocates the per-level lighting state */
	/*  (called after map has been fully loaded) */
	void (*AllocState)(void);
	/* Quickly calculates lighting for the blocks in the region */
	/*  [x, y, z] to [x + 18, y + 18, z + 18] */
	void (*LightHint)(int startX, int startY, int startZ);

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

void FancyLighting_SetActive(void);
void FancyLighting_OnInit(void);

/* Expose ClassicLighting functions for reuse in Fancy lighting */
void ClassicLighting_Refresh(void);
void ClassicLighting_FreeState(void);
void ClassicLighting_AllocState(void);
int ClassicLighting_GetLightHeight(int x, int z);
void ClassicLighting_LightHint(int startX, int startY, int startZ);
cc_bool ClassicLighting_IsLit(int x, int y, int z);
cc_bool ClassicLighting_IsLit_Fast(int x, int y, int z);
void ClassicLighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock);

CC_END_HEADER
#endif
