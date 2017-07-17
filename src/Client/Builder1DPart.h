#ifndef CS_BUILDER1DPART_H
#define CS_BUILDER1DPART_H
#include "Typedefs.h"
#include "VertexStructs.h"
#include "BlockEnums.h"
/* Contains state for vertices for a portion of a chunk mesh (vertices that are in a 1D atlas)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


typedef struct Builder1DPart_ {
	/* Pointers to offset within vertices, indexed by face. */
	VertexP3fT2fC4b* fVertices[Face_Count];
	/* Number of indices, indexed by face. */
	Int32 fCount[Face_Count];

	/* Number of indices, for sprites. */
	Int32 sCount;
	/* Current offset within vertices for sprites, delta between each sprite face. */
	Int32 sOffset, sAdvance;

	/* Pointer to vertex data. */
	VertexP3fT2fC4b* vertices;
	/* Number of elements in the vertices pointer. */
	Int32 verticesBufferCount;
	/* Total number of indices. (Sprite indices + number indices for each face). */
	Int32 iCount;
} Builder1DPart;

/* Prepares the given part for building vertices. */
void Builder1DPart_Prepare(Builder1DPart* part);

/* Resets counts to zero for the given part.*/
void Builder1DPart_Reset(Builder1DPart* part);
#endif