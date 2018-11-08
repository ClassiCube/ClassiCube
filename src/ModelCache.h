#ifndef CC_MODELCACHE_H
#define CC_MODELCACHE_H
#include "String.h"
#include "VertexStructs.h"
/* Contains a cache of model instances and default textures for models.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Model;

struct CachedModel { struct Model* Instance; String Name;  };
struct CachedTexture { uint8_t SkinType; GfxResourceID TexID; String Name; };
#if 0
public CustomModel[] CustomModels = new CustomModel[256];
#endif

#define MODELCACHE_MAX_MODELS 24
struct CachedModel ModelCache_Models[MODELCACHE_MAX_MODELS];
struct CachedTexture ModelCache_Textures[MODELCACHE_MAX_MODELS];
/* Maximum number of vertices a model can have */
#define MODELCACHE_MAX_VERTICES (24 * 12)
GfxResourceID ModelCache_Vb;
VertexP3fT2fC4b ModelCache_Vertices[MODELCACHE_MAX_VERTICES];

void ModelCache_Init(void);
void ModelCache_Free(void);
/* Returns pointer to model whose name caselessly matches given name. */
EXPORT_ struct Model* ModelCache_Get(const String* name);
/* Returns index of cached texture whose name caselessly matches given name. */
EXPORT_ int ModelCache_GetTextureIndex(const String* texName);
/* Adds a model to the list of cached models. (e.g. "skeleton") */
/* Cached models can be applied to entities to change their appearance. Use Entity_SetModel for that. */
/* NOTE: defaultTexName can be NULL, and is for some models. (such as the "block" model) */
EXPORT_ void ModelCache_Register(STRING_REF const char* name, const char* defaultTexName, struct Model* instance);
/* Adds a texture to the list of cached textures. (e.g. "skeleton.png") */
/* Cached textures are automatically loaded from texture packs. Used as a 'default skin' for models. */
/* NOTE: Textures should be registered BEFORE models are registered. */
EXPORT_ void ModelCache_RegisterTexture(STRING_REF const char* texName);
#endif
