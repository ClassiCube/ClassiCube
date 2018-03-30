#ifndef CC_VERTEXSTRUCTS_H
#define CC_VERTEXSTRUCTS_H
#include "PackedCol.h"
/* Represents simple vertex formats.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* 3 floats for position (XYZ), 4 bytes for colour. */
typedef struct VertexP3fC4b_ { Real32 X, Y, Z; PackedCol Col; } VertexP3fC4b;
void VertexP3fC4b_Set(VertexP3fC4b* target, Real32 x, Real32 y, Real32 z, PackedCol col);

/* 3 floats for position (XYZ), 2 floats for texture coordinates (UV), 4 bytes for colour. */
typedef struct VertexP3fT2fC4b_ { Real32 X, Y, Z; PackedCol Col; Real32 U, V; } VertexP3fT2fC4b;
void VertexP3fT2fC4b_Set(VertexP3fT2fC4b* target, Real32 x, Real32 y, Real32 z, Real32 u, Real32 v, PackedCol col);
#endif