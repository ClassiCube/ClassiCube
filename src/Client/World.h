#ifndef CS_WORLD_H
#define CS_WORLD_H
#include "Typedefs.h"
#include "String.h"
#include "Vector3I.h"
/* Represents a fixed size 3D array of blocks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Converts a single integer index into coordinates. */
#define World_Unpack(index, x, y, z)\
x = index % World_Width;\
z = (index / World_Width) % World_Length;\
y = (index / World_Width) / World_Length;

/* Packs a coordinate into a single integer index. */
#define World_Pack(x, y, z) (((y) * World_Length + (z)) * World_Width + (x))


/* Raw blocks of this world. */
BlockID* World_Blocks;

/* Size of blocks array. */
Int32 World_BlocksSize;

/* Length of world on X axis.*/
Int32 World_Width;

/* Length of world on Y axis (vertical).*/
Int32 World_Height;

/* Length of world on Z axis.*/
Int32 World_Length;

/* Largest valid X coordinate. */
Int32 World_MaxX;

/* Largest valid Y coordinate. */
Int32 World_MaxY;

/* Largest valid Z coordinate. */
Int32 World_MaxZ;

/* Amount a packed index must be changed by to advance Y coordinate. */
Int32 World_OneY;

/* Unique uuid/guid of this particular world. */
UInt8 World_Uuid[16];

/* Current terrain.png or texture pack url of this map. */
String World_TextureUrl;
/* TODO: how to initalise this string */


/* Resets all of the properties to their defaults. */
void World_Reset(void);

/* Updates the underlying block array, heightmap, and dimensions of this map. */
void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length);

BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z);


/* Sets the block at the given coordinates without bounds checking. */
#define World_SetBlock(x, y, z, blockId) World_Blocks[World_Pack(x, y, z)] = blockId

/* Sets the block at the given coordinates without bounds checking. */
#define World_SetBlock_3I(p, blockId) World_Blocks[World_Pack(p.X, p.Y, p.Z)] = blockId

/* Returns the block at the given coordinates without bounds checking. */
#define World_GetBlock(x, y, z) World_Blocks[World_Pack(x, y, z)]

/* Returns the block at the given world coordinates without bounds checking. */
#define World_GetBlock_3I(x, y, z) World_Blocks[World_Pack(p.X, p.Y, p.Z)]

/* Returns block at given coordinates if coordinates are inside the map, 0 if outside. */
BlockID World_SafeGetBlock(Int32 x, Int32 y, Int32 z);

/* Returns block at given coordinates if coordinates are inside the map, 0 if outside. */
BlockID World_SafeGetBlock_3I(Vector3I p);


/* Returns whether given coordinates are contained inside this map. */
bool World_IsValidPos(Int32 x, Int32 y, Int32 z);

/* Returns whether given coordinates are contained inside this map. */
bool World_IsValidPos_3I(Vector3I p);

/* Unpacks the given index into the map's block array into its original world coordinates. */
Vector3I World_GetCoords(Int32 index);
#endif