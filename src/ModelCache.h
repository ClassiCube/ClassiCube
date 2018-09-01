#ifndef CC_MODELCACHE_H
#define CC_MODELCACHE_H
#include "String.h"
#include "VertexStructs.h"
/* Contains a cache of model instances and default textures for models.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Model;

struct CachedModel { struct Model* Instance; String Name;  };
struct CachedTexture { UInt8 SkinType; GfxResourceID TexID; String Name; };
#if FALSE
public CustomModel[] CustomModels = new CustomModel[256];
#endif

#define MODELCACHE_MAX_MODELS 24
struct CachedModel ModelCache_Models[MODELCACHE_MAX_MODELS];
struct CachedTexture ModelCache_Textures[MODELCACHE_MAX_MODELS];
/* Maximum number of vertices a model can have. */
#define MODELCACHE_MAX_VERTICES (24 * 12)
GfxResourceID ModelCache_Vb;
VertexP3fT2fC4b ModelCache_Vertices[MODELCACHE_MAX_VERTICES];

void ModelCache_Init(void);
void ModelCache_Free(void);
FUNC_NOINLINE struct Model* ModelCache_Get(STRING_PURE String* name);
FUNC_NOINLINE Int32 ModelCache_GetTextureIndex(STRING_PURE String* texName);
FUNC_NOINLINE void ModelCache_Register(STRING_REF const char* name, STRING_PURE const char* defaultTexName, struct Model* instance);
FUNC_NOINLINE void ModelCache_RegisterTexture(STRING_REF const char* texName);
#endif
