#ifndef CS_RESPAWN_H
#define CS_RESPAWN_H
#include "Typedefs.h"
#include "Vectors.h"
#include "AABB.h"
/* Helper methods for spawning an entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Finds the highest free Y coordinate in the given bounding box.*/
Real32 Respawn_HighestFreeY(AABB* bb);

/* Finds a suitable spawn position for the entity, by iterating downards from top of
the world until the ground is found. */
Vector3 Respawn_FindSpawnPosition(Real32 x, Real32 z, Vector3 modelSize);
#endif