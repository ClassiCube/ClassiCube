#ifndef CS_WORLD_H
#define CS_WORLD_H
#include "Typedefs.h"
#include "String.h"
#include "Vector3I.h"
/* Represents a fixed size 3D array of blocks.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Raw blocks of this world. */
BlockID* World_Blocks;

/* Length of world on X axis.*/
Int32 World_Width;

/* Length of world on Y axis (vertical).*/
Int32 World_Height;

/* Length of world on Z axis.*/
Int32 World_Length;

/* Unique uuid/guid of this particular world. */
UInt8 World_Uuid[16];

/* Current terrain.png or texture pack url of this map. */
String World_TextureUrl;


/* Resets all of the properties to their defaults. */
void World_Reset();

/* Updates the underlying block array, heightmap, and dimensions of this map. */
void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length);

BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z);


/* Sets the block at the given oordinates without bounds checking. */
void World_SetBlock(Int32 x, Int32 y, Int32 z, BlockID blockId);

/* Sets the block at the given oordinates without bounds checking. */
void World_SetBlock_3I(Vector3I p, BlockID blockId);

/* Returns the block at the given coordinates without bounds checking. */
BlockID World_GetBlock(Int32 x, Int32 y, Int32 z);

/* Returns the block at the given world coordinates without bounds checking. */
BlockID World_GetBlock_3I(Vector3I p);

/* Returns block at given coordinates if coordinates are inside the map, 0 if outside. */
BlockID World_SafeGetBlock(Int32 x, Int32 y, Int32 z);

/* Returns block at given coordinates if coordinates are inside the map, 0 if outside. */
BlockID World_SafeGetBlock_3I(Vector3I p);


/* Returns whether given coordinates are contained inside this map. */
bool World_IsValidPos(Int32 x, Int32 y, Int32 z);

/* Returns whether given coordinates are contained inside this map. */
bool World_IsValidPos_3I(Vector3I p);

/* Unpacks the given index into the map's block array into its original world coordinates. */
Vector3I World_GetCoords(int index);