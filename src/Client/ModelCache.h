#ifndef CS_MODELCACHE_H
#define CS_MODELCACHE_H
#include "Typedefs.h"
#include "String.h"
#include "IModel.h"
#include "GraphicsEnums.h"
#include "Compiler.h"
#include "Stream.h"
#include "VertexStructs.h"
/* Contains a cache of model instances and default textures for models.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


typedef struct CachedModel_ {
	/* Name associated with the model, all lowercase. */
	String Name;
	/* Pointer to the actual model instance. */
	IModel* Instance;
} CachedModel;

typedef struct CachedTexture_ {
	/* Filename of the texture. */
	String Name;
	/* Native texture ID. */
	GfxResourceID TexID;
} CachedTexture;

#if FALSE
public CustomModel[] CustomModels = new CustomModel[256];
#endif

/* Maximum number of models (or textures) that can be registered. */
#define ModelCache_MaxModels 24
/* The models that have been registered previously.
The first model (index 0) is the humanoid model. */
CachedModel ModelCache_Models[ModelCache_MaxModels];
/* The textures that have been registered previously. */
CachedTexture ModelCache_Textures[ModelCache_MaxModels];
/* Maximum number of vertices a model can have. */
#define ModelCache_MaxVertices (24 * 12)
/* Dynamic vertex buffer for model rendering. */
GfxResourceID ModelCache_Vb;
/* Raw vertices for model rendering. */
VertexP3fT2fC4b ModelCache_Vertices[ModelCache_MaxVertices];

/* Initalises the model cache and hooks events. */
void ModelCache_Init(void);
/* Frees textures and vertex buffer of the model cache, and then unhooks events. */
void ModelCache_Free(void);
/* Gets the model which has the given name, or pointer to humanoid model if not found. */
IModel* ModelCache_Get(STRING_TRANSIENT String* name);
/* Gets the index of the texture which has the given name, or -1 if not found. */
Int32 ModelCache_GetTextureIndex(STRING_TRANSIENT String* texName);
/* Registers the given model to be able to be used as a model for entities.
You can use ModelCache_Get to get pointer to the model. */
void ModelCache_Register(STRING_REF const UInt8* name, STRING_TRANSIENT const UInt8* defaultTexName, IModel* instance);
/* Registers the given texture to be tracked by the model cache.
You can use ModelCache_GetTextureIndex to get the index of this texture. */
void ModelCache_RegisterTexture(STRING_REF const UInt8* texName);
#endif