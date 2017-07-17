#ifndef CS_ENTITY_H
#define CS_ENTITY_H
#include "Typedefs.h"
#include "Vectors.h"
#include "AABB.h"
#include "AnimatedComp.h"
#include "GraphicsEnums.h"
/* Represents an in-game entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Constant offset used to avoid floating point roundoff errors. */
#define Entity_Adjustment 0.001f


/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
typedef struct Entity_ {
	/* Position of the entity in the world.*/
	Vector3 Position;

	/* Texture IDs for this entity's skin. */
	GfxResourceID TextureId, MobTextureId;

	/* Rotation of the entity in the world. */
	Real32 HeadX, HeadY, RotX, RotY, RotZ;

	/* AABB of the model. Centred around the origin, NOT the position. */
	AABB ModelAABB;

	/* Animation component of the entity. */
	AnimatedComp Anim;
} Entity;


/* Initalises the given entity. */
void Entity_Init(Entity* entity);

/* Returns the bounding box that contains the model, without any rotations applied. */
AABB Entity_GetPickingBounds(Entity* entity);

/* Bounding box of the model that collision detection is performed with, in world coordinates. */
AABB Entity_GetBounds(Entity* entity);
#endif