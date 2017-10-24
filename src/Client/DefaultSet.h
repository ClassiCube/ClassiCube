#ifndef CC_DEFAULT_BLOCKS_H
#define CC_DEFAULT_BLOCKS_H
#include "Typedefs.h"
#include "Block.h"
#include "PackedCol.h"
/* List of properties for core blocks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Default height of a block. */
Real32 DefaultSet_Height(BlockID b);
/* Gets whether the given block is fully bright. */
bool DefaultSet_FullBright(BlockID b);
/* Gets default fog density of a block. 0 means no fog density. */
Real32 DefaultSet_FogDensity(BlockID b);
/* Gets the fog colour of this block. */
PackedCol DefaultSet_FogColour(BlockID b);
/* Gets the collide type of a block. */
CollideType DefaultSet_Collide(BlockID b);
/* Gets a backwards compatible collide type of a block. */
CollideType DefaultSet_MapOldCollide(BlockID b, CollideType collide);
/* Gets whether the given block prevents light passing through it. */
bool DefaultSet_BlocksLight(BlockID b);
/* Gets the ID of the sound played when stepping on this block. */
SoundType DefaultSet_StepSound(BlockID b);
/* Gets the type of method used to draw/render this block. */
DrawType DefaultSet_Draw(BlockID b);
/* Gets the ID of the sound played when deleting/placing this block. */
SoundType DefaultSet_DigSound(BlockID b);
#endif