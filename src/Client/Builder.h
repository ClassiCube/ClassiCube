#ifndef CC_BUILDER_H
#define CC_BUILDER_H
#include "Typedefs.h"
/* Converts a 16x16x16 chunk into a mesh of vertices.
NormalMeshBuilder:
   Implements a simple chunk mesh builder, where each block face is a single colour.
   (whatever lighting engine returns as light colour for given block face at given coordinates)

Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct ChunkInfo_ ChunkInfo;

Int32 Builder_SidesLevel, Builder_EdgeLevel;

void Builder_Init(void);
void Builder_SetDefault(void);
void Builder_OnNewMapLoaded(void);
void Builder_MakeChunk(ChunkInfo* info);

void NormalBuilder_SetActive(void);
void AdvLightingBuilder_SetActive(void);
#endif