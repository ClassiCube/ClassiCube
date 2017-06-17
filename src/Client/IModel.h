#ifndef CS_MODEL_H
#define CS_MODEL_H
#include "Typedefs.h"
#include "Vectors.h"
/* Containts various structs and methods for an entity model.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Describes a vertex within a model. */
typedef struct ModelVertex {
	Real32 X, Y, Z;
	UInt16 U, V;
} ModelVertex;

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, UInt16 u, UInt16 v);


/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
typedef struct IModel {

	/* Pointer to the raw vertices of the model.*/
	ModelVertex* vertices;
	/* Count of assigned vertices within the raw vertices array. */
	Int32 index;

} IModel;
#endif