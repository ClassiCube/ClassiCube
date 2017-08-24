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
/* Constant value specifying an angle is not included in an orientation update. */
#define LocationUpdate_Excluded -100000.31415926535f

typedef bool (*TouchesAny_Condition)(BlockID block);

/* Represents a location update for an entity.
This can be a relative position, full position, and/or an orientation update. */
typedef struct LocationUpdate_ {
	/* Position of the update (if included). */
	Vector3 Pos;
	/* Orientation of the update (if included). If not, has the value of LocationUpdate_Excluded. */
	Real32 RotX, RotY, RotZ, HeadX;
	/* Whether this update includes an absolute or relative position. */
	bool IncludesPosition;
	/* Whether the positon is absolute, or relative to the last positionreceived from the server. */
	bool RelativePosition;
} LocationUpdate;

/* Clamps the given angle so it lies between [0, 360). */
Real32 LocationUpdate_Clamp(Real32 degrees);

/* Constructs a location update with values for every field.
You should generally prefer using the alternative constructors. */
void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos);
/* Constructs a location update that does not have any position or orientation information. */
void LocationUpdate_Empty(LocationUpdate* update);
/* Constructs a location update that only consists of orientation information. */
void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX);
/* Constructs a location update that only consists of position information. */
void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel);
/* Constructs a location update that consists of position and orientation information. */
void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel);


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