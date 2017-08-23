#if 0
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

typedef bool(*TouchesAny_Condition)(BlockID block);


/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
typedef struct Entity_ {
	/* Position of the entity in the world.*/
	Vector3 Position;
	/* Rotation of the entity in the world. */
	Real32 HeadX, HeadY, RotX, RotY, RotZ;
	/* Velocity the entity is moving at. */
	Vector3 Velocity, OldVelocity;
	
	/* Texture IDs for this entity's skin. */
	GfxResourceID TextureId, MobTextureId;
	/* Animation component of the entity. */
	AnimatedComp Anim;

	/* The model of this entity. (used for collision detection and rendering) */
	IModel* Model;
	/* The name of the model of this entity. Is "block" for all block ID models. */
	String ModelName;
		
	/* BlockID, if model name was originally a vaid block id. Avoids needing to repeatedly parse ModelName as a byte. */
	BlockID ModelBlock;
	/* AABB of the model. Centred around the origin, NOT the position. */
	AABB ModelAABB;
	/* Scale applied to the model on each axis. */
	Vector3 ModelScale;
} Entity;


/* Initalises the given entity. */
void Entity_Init(Entity* entity);

/* Returns the bounding box that contains the model, without any rotations applied. */
void Entity_GetPickingBounds(Entity* entity, AABB* bb);

/* Bounding box of the model that collision detection is performed with, in world coordinates. */
void Entity_GetBounds(Entity* entity, AABB* bb);

/* Determines whether any of the blocks that intersect the given bounding box satisfy the given condition. */
bool Entity_TouchesAny(AABB* bb, TouchesAny_Condition condition);
	
/* Determines whether any of the blocks that intersect the AABB of this entity are rope. */
bool Entity_TouchesAnyRope(Entity* entity);
		
/* Determines whether any of the blocks that intersect thee AABB of this entity are lava or still lava. */
bool Entity_TouchesAnyLava(Entity* entity);

/* Determines whether any of the blocks that intersect the AABB of this entity are water or still water. */
bool Entity_TouchesAnyWater(Entity* entity);
#endif
#endif