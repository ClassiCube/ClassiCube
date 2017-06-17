#include "ModelCache.h"
#include "GraphicsAPI.h"
#include "Game.h"
#include "MiscEvents.h"
#include "StringConvert.h"

String ModelCache_blockString, ModelCache_charPngString;
Int32 ModelCache_texCount, ModelCache_modelCount;

void ModelCache_Init(void) {
	ModelCache_blockString = String_FromConstant("block");
	ModelCache_charPngString = String_FromConstant("char.png");
	ModelCache_RegisterDefaultModels();
	ModelCache_ContextRecreated();

	EventHandler_RegisterStream(TextureEvents_FileChanged, ModelCache_TextureChanged);
	EventHandler_RegisterVoid(Gfx_ContextLost, ModelCache_ContextLost);
	EventHandler_RegisterVoid(Gfx_ContextRecreated, ModelCache_ContextRecreated);
}

void ModelCache_Free(void) {
	Int32 i;
	for (i = 0; i < ModelCache_texCount; i++) {
		CachedTexture* tex = &ModelCache_Textures[i];
		Gfx_DeleteTexture(&tex->TexID);
	}
	ModelCache_ContextLost();

	EventHandler_UnregisterStream(TextureEvents_FileChanged, ModelCache_TextureChanged);
	EventHandler_UnregisterVoid(Gfx_ContextLost, ModelCache_ContextLost);
	EventHandler_UnregisterVoid(Gfx_ContextRecreated, ModelCache_ContextRecreated);
}


IModel* ModelCache_Get(STRING_TRANSIENT String* name) {
	if (String_Equals(name, &ModelCache_blockString)) {
		return ModelCache_Models[0].Instance;
	}

	String* modelName = name;
	UInt8 blockId;
	if (Convert_TryParseByte(name, &blockId)) {
		modelName = &ModelCache_blockString;
	}

	Int32 i;
	for (i = 0; i < ModelCache_modelCount; i++) {
		CachedModel* m = &ModelCache_Models[i];
		if (!String_Equals(&m->Name, modelName)) continue;
		
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