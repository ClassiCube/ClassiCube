#ifndef CC_MODELCACHE_H
#define CC_MODELCACHE_H
#include "String.h"
#include "VertexStructs.h"
/* Contains a cache of model instances and default textures for models.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct IModel_ IModel;

typedef struct CachedModel_ {
	String Name;      /* Name associated with the model, all lowercase. */
	IModel* Instance; /* Pointer to the actual model instance. */
} CachedModel;

typedef struct CachedTexture_ {	
	String Name;         /* Filename of the texture. */
	GfxResourceID TexID; /* Native texture ID. */
} CachedTexture;

#if FALSE
public CustomModel[] CustomModels = new CustomModel[256];
#endif

#define MODELCACHE_MAX_MODELS 24
CachedModel ModelCache_Models[MODELCACHE_MAX_MODELS];
CachedTexture ModelCache_Textures[MODELCACHE_MAX_MODELS];
/* Maximum number of vertices a model can have. */
#define MODELCACHE_MAX_VERTICES (24 * 12)
GfxResourceID ModelCache_Vb;
VertexP3fT2fC4b ModelCache_Vertices[MODELCACHE_MAX_VERTICES];

void ModelCache_Init(void);
void ModelCache_Free(void);
IModel* ModelCache_Get(STRING_PURE String* name);
Int32 ModelCache_GetTextureIndex(STRING_PURE String* texName);
void ModelCache_Register(STRING_REF const UChar* name, STRING_PURE const UChar* defaultTexName, IModel* instance);
void ModelCache_RegisterTexture(STRING_REF const UChar* texName);
#endif
