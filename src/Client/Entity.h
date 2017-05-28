#ifndef CS_ENTITY_H
#define CS_ENTITY_H
#include "Typedefs.h"
#include "Vectors.h"
/* Represents an in-game entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Constant offset used to avoid floating point roundoff errors. */
#define Entity_Adjustment 0.001f


/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
typedef struct Entity {
	Vector3 Position;
	Real32 HeadX, HeadY, RotX, RotY, RotZ;
} Entity;
#endif