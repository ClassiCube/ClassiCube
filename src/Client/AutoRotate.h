#ifndef CS_AUTOROTATE_H
#define CS_AUTOROTATE_H
#include "Typedefs.h"
#include "BlockID.h"
#include "String.h"
#include "Vectors.h"

/* Performs automatic rotation of directional blocks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Attempts to find the rotated block based on the user's orientation and offset on selected block. */
BlockID AutoRotate_RotateBlock(BlockID block);


static BlockID AutoRotate_RotateCorner(BlockID block, String* name, Vector3 offset);

static BlockID AutoRotate_RotateVertical(BlockID block, String* name, Vector3 offset);

static BlockID AutoRotate_RotateOther(BlockID block, String* name, Vector3 offset);

static BlockID AutoRotate_RotateDirection(BlockID block, String* name, Vector3 offset);

static BlockID AutoRotate_Find(BlockID block, String* name, const UInt8* suffix);
#endif