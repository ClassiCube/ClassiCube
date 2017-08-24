#if 0
#include "ModelCache.h"
#include "GraphicsAPI.h"
#include "Game.h"
#include "Events.h"
#include "String.h"

String ModelCache_charPngString = String_FromConstant("char.png");
Int32 ModelCache_texCount, ModelCache_modelCount;

void ModelCache_Init(void) {
	ModelCache_RegisterDefaultModels();
	ModelCache_ContextRecreated();

	Event_RegisterStream(&TextureEvents_FileChanged, ModelCache_TextureChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost, ModelCache_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, ModelCache_ContextRecreated);
}

void ModelCache_Free(void) {
	Int32 i;
	for (i = 0; i < ModelCache_texCount; i++) {
		CachedTexture* tex = &ModelCache_Textures[i];
		Gfx_DeleteTexture(&tex->TexID);
	}
	ModelCache_ContextLost();

	Event_UnregisterStream(&TextureEvents_FileChanged, ModelCache_TextureChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost, ModelCache_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, ModelCache_ContextRecreated);
}


IModel* ModelCache_Get(STRING_TRANSIENT String* name) {
	Int32 i;
	for (i = 0; i < ModelCache_modelCount; i++) {
		CachedModel* m = &ModelCache_Models[i];
		if (!String_Equals(&m->Name, name)) continue;
		
		if (!m->Instance->initalised) {
			m->Instance->CreateParts();
			m->Instance->initalised = true;
		}
		return m->Instance;
	}
	return ModelCache_Models[0].Instance;
}

Int32 ModelCache_GetTextureIndex(STRING_TRANSIENT String* texName) {
	Int32 i;
	for (i = 0; i < ModelCache_texCount; i++) {
		CachedTexture* tex = &ModelCache_Textures[i];
		if (String_Equals(&tex->Name, texName)) return 1;
	}
	return -1;
}

void ModelCache_RegisterModel(STRING_REF String* name, STRING_REF String* defaultTexName, IModel* instance) {
	if (ModelCache_texCount < ModelCache_MaxModels) {
		CachedModel model; model.Name = *name; model.Instance = instance;
		ModelCache_Models[ModelCache_modelCount] = model;
		ModelCache_modelCount++;
		instance->defaultTexIndex = ModelCache_GetTextureIndex(defaultTexName);
	} else {
		ErrorHandler_Fail("ModelCache_RegisterModel - hit max models");
	}
}

void ModelCache_RegisterTexture(STRING_REF String* texName) {
	if (ModelCache_texCount < ModelCache_MaxModels) {
		CachedTexture tex; tex.Name = *texName; tex.TexID = -1;
		ModelCache_Textures[ModelCache_texCount] = tex;
		ModelCache_texCount++;
	} else {
		ErrorHandler_Fail("ModelCache_RegisterTexture - hit max textures");
	}
}


static void ModelCache_TextureChanged(Stream* stream) {
	Int32 i;
	for (i = 0; i < ModelCache_texCount; i++) {
		CachedTexture* tex = &ModelCache_Textures[i];
		if (!String_Equals(&tex->Name, &stream->Name)) continue;

		bool isCharPng = String_Equals(&stream->Name, &ModelCache_charPngString);
		Game_UpdateTexture(&tex->TexID, stream, isCharPng);
		return;
	}
}

static void ModelCache_ContextLost(void) {
	Gfx_DeleteVb(&ModelCache_Vb);
}

static void ModelCache_ContextRecreated(void) {
	ModelCache_Vb = Gfx_CreateDynamicVb(VertexFormat_P3fT2fC4b, ModelCache_MaxVertices);
}
#endif