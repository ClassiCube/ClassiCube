#ifndef CC_BUILDER_H
#define CC_BUILDER_H
#include "Core.h"
/* Converts a 16x16x16 chunk into a mesh of vertices.
NormalMeshBuilder:
   Implements a simple chunk mesh builder, where each block face is a single colour.
   (whatever lighting engine returns as light colour for given block face at given coordinates)

Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct ChunkInfo;

extern int Builder_SidesLevel, Builder_EdgeLevel;
/* Whether smooth/advanced lighting mesh builder is used. */
extern cc_bool Builder_SmoothLighting;

void Builder_Init(void);
void Builder_OnNewMapLoaded(void);
/* Builds the mesh of vertices for the given chunk. */
void Builder_MakeChunk(struct ChunkInfo* info);

void NormalBuilder_SetActive(void);
void AdvBuilder_SetActive(void);
void Builder_ApplyActive(void);
#endif
