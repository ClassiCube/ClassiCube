#ifndef CC_MODELCACHE_H
#define CC_MODELCACHE_H
#include "String.h"
#include "VertexStructs.h"
/* Contains a cache of model instances and default textures for models.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Model;
struct ModelTex;
struct ModelTex { const char* Name; uint8_t SkinType; GfxResourceID TexID; struct ModelTex* Next; };
#if 0
public CustomModel[] CustomModels = new CustomModel[256];
#endif

/* Maximum number of vertices a model can have */
#define MODELCACHE_MAX_VERTICES (24 * 12)
GfxResourceID ModelCache_Vb;
VertexP3fT2fC4b ModelCache_Vertices[MODELCACHE_MAX_VERTICES];
struct Model* Human_ModelPtr;

void ModelCache_Init(void);
void ModelCache_Free(void);
/* Returns pointer to model whose name caselessly matches given name. */
CC_EXPORT struct Model* ModelCache_Get(const String* name);
/* Returns index of cached texture whose name caselessly matches given name. */
CC_EXPORT struct ModelTex* ModelCache_GetTexture(const String* name);
/* Adds a model to the list of models. (e.g. "skeleton") */
/* Models can be applied to entities to change their appearance. Use Entity_SetModel for that. */
CC_EXPORT void ModelCache_Register(struct Model* model);
/* Adds a texture to the list of model textures. (e.g. "skeleton.png") */
/* Model textures are automatically loaded from texture packs. Used as a 'default skin' for models. */
CC_EXPORT void ModelCache_RegisterTexture(struct ModelTex* tex);
#endif
