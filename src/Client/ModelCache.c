#include "ModelCache.h"
#include "GraphicsAPI.h"
#include "Game.h"
#include "Event.h"
#include "ExtMath.h"
#include "IModel.h"
#include "String.h"
#include "TerrainAtlas.h"
#include "Drawer.h"
#include "Block.h"
#include "Stream.h"
#include "ErrorHandler.h"
#include "Entity.h"
#include "IModel.h"


/*########################################################################################################################*
*---------------------------------------------------------ModelCache------------------------------------------------------*
*#########################################################################################################################*/
UInt32 ModelCache_texCount, ModelCache_modelCount;
#define MODEL_RET_SIZE(x,y,z) static Vector3 P = { (x)/16.0f,(y)/16.0f,(z)/16.0f }; *size = P;
#define MODEL_RET_AABB(x1,y1,z1, x2,y2,z2) static struct AABB BB = { (x1)/16.0f,(y1)/16.0f,(z1)/16.0f, (x2)/16.0f,(y2)/16.0f,(z2)/16.0f }; *bb = BB;
#define BOXDESC_DIM(p1, p2) p1 < p2 ? p2 - p1 : p1 - p2

#define BOXDESC_TEX(x, y)                 x,y
#define BOXDESC_DIMS(x1,y1,z1,x2,y2,z2)   BOXDESC_DIM(z1,z2), BOXDESC_DIM(x1,x2), BOXDESC_DIM(y1,y2)
#define BOXDESC_BOUNDS(x1,y1,z1,x2,y2,z2) x1/16.0f,x2/16.0f, y1/16.0f,y2/16.0f, z1/16.0f,z2/16.0f
#define BOXDESC_ROT(x, y, z)              x/16.0f,y/16.0f,z/16.0f
#define BOXDESC_BOX(x1,y1,z1,x2,y2,z2)    BOXDESC_DIMS(x1,y1,z1,x2,y2,z2), BOXDESC_BOUNDS(x1,y1,z1,x2,y2,z2)

static void ModelCache_ContextLost(void* obj) {
	Gfx_DeleteVb(&ModelCache_Vb);
}

static void ModelCache_ContextRecreated(void* obj) {
	ModelCache_Vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, MODELCACHE_MAX_VERTICES);
}

static void ModelCache_InitModel(struct IModel* model) {
	struct IModel* active = IModel_ActiveModel;
	IModel_ActiveModel = model;
	model->CreateParts();

	model->initalised = true;
	model->index = 0;
	IModel_ActiveModel = active;
}

struct IModel* ModelCache_Get(STRING_PURE String* name) {
	UInt32 i;
	for (i = 0; i < ModelCache_modelCount; i++) {
		struct CachedModel* m = &ModelCache_Models[i];
		if (!String_CaselessEquals(&m->Name, name)) continue;
		
		if (!m->Instance->initalised) {
			ModelCache_InitModel(m->Instance);
		}
		return m->Instance;
	}
	return ModelCache_Models[0].Instance;
}

Int32 ModelCache_GetTextureIndex(STRING_PURE String* texName) {
	UInt32 i;
	for (i = 0; i < ModelCache_texCount; i++) {
		struct CachedTexture* tex = &ModelCache_Textures[i];
		if (String_CaselessEquals(&tex->Name, texName)) return i;
	}
	return -1;
}

void ModelCache_Register(STRING_REF const UChar* name, STRING_PURE const UChar* defaultTexName, struct IModel* instance) {
	if (ModelCache_modelCount < MODELCACHE_MAX_MODELS) {
		struct CachedModel model;
		model.Name = String_FromReadonly(name);
		model.Instance = instance;
		ModelCache_Models[ModelCache_modelCount++] = model;

		if (defaultTexName) {
			String defaultTex = String_FromReadonly(defaultTexName);
			instance->defaultTexIndex = ModelCache_GetTextureIndex(&defaultTex);
		}		
	} else {
		ErrorHandler_Fail("ModelCache_RegisterModel - hit max models");
	}
}

void ModelCache_RegisterTexture(STRING_REF const UChar* texName) {
	if (ModelCache_texCount < MODELCACHE_MAX_MODELS) {
		struct CachedTexture tex;
		tex.Name = String_FromReadonly(texName);
		tex.TexID = NULL;
		ModelCache_Textures[ModelCache_texCount++] = tex;
	} else {
		ErrorHandler_Fail("ModelCache_RegisterTexture - hit max textures");
	}
}

static void ModelCache_TextureChanged(void* obj, struct Stream* stream, String* name) {
	UInt32 i;
	String charPng = String_FromConst("char.png");
	for (i = 0; i < ModelCache_texCount; i++) {
		struct CachedTexture* tex = &ModelCache_Textures[i];
		if (!String_CaselessEquals(&tex->Name, name)) continue;

		Game_UpdateTexture(&tex->TexID, stream, name, &tex->SkinType);
		return;
	}
}


/*########################################################################################################################*
*--------------------------------------------------------ChickenModel-----------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Chicken_Head, Chicken_Head2, Chicken_Head3, Chicken_Torso;
struct ModelPart Chicken_LeftLeg, Chicken_RightLeg, Chicken_LeftWing, Chicken_RightWing;
struct ModelVertex ChickenModel_Vertices[IMODEL_BOX_VERTICES * 6 + (IMODEL_QUAD_VERTICES * 2) * 2];
struct IModel ChickenModel;

static void ChickenModel_MakeLeg(struct ModelPart* part, Int32 x1, Int32 x2, Int32 legX1, Int32 legX2) {
#define ch_y1 (1.0f  / 64.0f)
#define ch_y2 (5.0f  / 16.0f)
#define ch_z2 (1.0f  / 16.0f)
#define ch_z1 (-2.0f / 16.0f)

	struct IModel* m = &ChickenModel;
	BoxDesc_YQuad(m, 32, 0, 3, 3,
		x2 / 16.0f, x1 / 16.0f, ch_z1, ch_z2, ch_y1, false); /* bottom feet */
	BoxDesc_ZQuad(m, 36, 3, 1, 5,
		legX1 / 16.0f, legX2 / 16.0f, ch_y1, ch_y2, ch_z2, false); /* vertical part of leg */

	ModelPart_Init(part, m->index - IMODEL_QUAD_VERTICES * 2, IMODEL_QUAD_VERTICES * 2,
		0.0f / 16.0f, 5.0f / 16.0f, 1.0f / 16.0f);
}

static void ChickenModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-2,9,-6, 2,15,-3),
		BOXDESC_ROT(0,9,-4),
	}; BoxDesc_BuildBox(&Chicken_Head, &head);

	static struct BoxDesc head2 = { /* TODO: Find a more appropriate name. */
		BOXDESC_TEX(14,4),
		BOXDESC_BOX(-1,9,-7, 1,11,-5),
		BOXDESC_ROT(0,9,-4),
	}; BoxDesc_BuildBox(&Chicken_Head2, &head2);

	static struct BoxDesc head3 = {
		BOXDESC_TEX(14,0),
		BOXDESC_BOX(-2,11,-8, 2,13,-6),
		BOXDESC_ROT(0,9,-4),
	}; BoxDesc_BuildBox(&Chicken_Head3, &head3);

	static struct BoxDesc torso = {
		BOXDESC_TEX(0,9),
		BOXDESC_BOX(-3,5,-4, 3,11,3),
		BOXDESC_ROT(0,5,0),
	}; BoxDesc_BuildRotatedBox(&Chicken_Torso, &torso);

	static struct BoxDesc lWing = {
		BOXDESC_TEX(24,13),
		BOXDESC_BOX(-4,7,-3, -3,11,3),
		BOXDESC_ROT(-3,11,0),
	}; BoxDesc_BuildBox(&Chicken_LeftWing, &lWing);

	static struct BoxDesc rWing = {
		BOXDESC_TEX(24,13),
		BOXDESC_BOX(3,7,-3, 4,11,3),
		BOXDESC_ROT(3,11,0),
	}; BoxDesc_BuildBox(&Chicken_RightWing, &rWing);

	ChickenModel_MakeLeg(&Chicken_LeftLeg, -3, 0, -2, -1);
	ChickenModel_MakeLeg(&Chicken_RightLeg, 0, 3, 1, 2);
}

static Real32 ChickenModel_GetEyeY(struct Entity* entity) { return 14.0f / 16.0f; }
static void ChickenModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(8.0f, 12.0f, 8.0f); }
static void ChickenModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-4.0f, 0.0f, -8.0f,  4.0f, 15.0f, 4.0f);
}

static void ChickenModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Chicken_Head,  true);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Chicken_Head2, true);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Chicken_Head3, true);

	IModel_DrawPart(&Chicken_Torso);
	IModel_DrawRotate(0, 0, -Math_AbsF(entity->Anim.LeftArmX), &Chicken_LeftWing,  false);
	IModel_DrawRotate(0, 0,  Math_AbsF(entity->Anim.LeftArmX), &Chicken_RightWing, false);

	PackedCol col = IModel_Cols[0];
	UInt32 i;
	for (i = 0; i < FACE_COUNT; i++) {
		IModel_Cols[i] = PackedCol_Scale(col, 0.7f);
	}

	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Chicken_LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Chicken_RightLeg, false);
	IModel_UpdateVB();
}

static struct IModel* ChickenModel_GetInstance(void) {
	IModel_Init(&ChickenModel);
	IModel_SetPointers(ChickenModel);
	ChickenModel.NameYOffset = 1.0125f;
	ChickenModel.vertices = ChickenModel_Vertices;
	return &ChickenModel;
}


/*########################################################################################################################*
*--------------------------------------------------------CreeperModel-----------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Creeper_Head, Creeper_Torso, Creeper_LeftLegFront;
struct ModelPart Creeper_RightLegFront, Creeper_LeftLegBack, Creeper_RightLegBack;
struct ModelVertex CreeperModel_Vertices[IMODEL_BOX_VERTICES * 6];
struct IModel CreeperModel;

static void CreeperModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-4,18,-4, 4,26,4),
		BOXDESC_ROT(0,18,0),
	}; BoxDesc_BuildBox(&Creeper_Head, &head);

	static struct BoxDesc torso = {
		BOXDESC_TEX(16,16),
		BOXDESC_BOX(-4,6,-2, 4,18,2),
		BOXDESC_ROT(0,6,0),
	}; BoxDesc_BuildBox(&Creeper_Torso, &torso);

	static struct BoxDesc lFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-4,0,-6, 0,6,-2),
		BOXDESC_ROT(0,6,-2),
	}; BoxDesc_BuildBox(&Creeper_LeftLegFront, &lFront);

	static struct BoxDesc rFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(0,0,-6, 4,6,-2),
		BOXDESC_ROT(0,6,-2),
	}; BoxDesc_BuildBox(&Creeper_RightLegFront, &rFront);

	static struct BoxDesc lBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-4,0,2, 0,6,6),
		BOXDESC_ROT(0,6,2),
	}; BoxDesc_BuildBox(&Creeper_LeftLegBack, &lBack);

	static struct BoxDesc rBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(0,0,2, 4,6,6),
		BOXDESC_ROT(0,6,2),
	}; BoxDesc_BuildBox(&Creeper_RightLegBack, &rBack);
}

static Real32 CreeperModel_GetEyeY(struct Entity* entity) { return 22.0f / 16.0f; }
static void CreeperModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(8.0f, 26.0f, 8.0f); }
static void CreeperModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-4.0f, 0.0f, -6.0f,  4.0f, 26.0f, 6.0f);
}

static void CreeperModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Creeper_Head, true);

	IModel_DrawPart(&Creeper_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Creeper_LeftLegFront,  false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Creeper_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Creeper_LeftLegBack,   false);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Creeper_RightLegBack,  false);
	IModel_UpdateVB();
}

static struct IModel* CreeperModel_GetInstance(void) {
	IModel_Init(&CreeperModel);
	IModel_SetPointers(CreeperModel);
	CreeperModel.vertices = CreeperModel_Vertices;
	CreeperModel.NameYOffset = 1.7f;
	return &CreeperModel;
}


/*########################################################################################################################*
*----------------------------------------------------------PigModel-------------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Pig_Head, Pig_Torso, Pig_LeftLegFront, Pig_RightLegFront;
struct ModelPart Pig_LeftLegBack, Pig_RightLegBack;
struct ModelVertex PigModel_Vertices[IMODEL_BOX_VERTICES * 6];
struct IModel PigModel;

static void PigModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-4,8,-14, 4,16,-6),
		BOXDESC_ROT(0,12,-6),
	}; BoxDesc_BuildBox(&Pig_Head, &head);

	static struct BoxDesc torso = {
		BOXDESC_TEX(28,8),
		BOXDESC_BOX(-5,6,-8, 5,14,8),
		BOXDESC_ROT(0,6,0),
	}; BoxDesc_BuildRotatedBox(&Pig_Torso, &torso);

	static struct BoxDesc lFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-5,0,-7, -1,6,-3),
		BOXDESC_ROT(0,6,-5),
	}; BoxDesc_BuildBox(&Pig_LeftLegFront, &lFront);

	static struct BoxDesc rFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(1,0,-7, 5,6,-3),
		BOXDESC_ROT(0,6,-5),
	}; BoxDesc_BuildBox(&Pig_RightLegFront, &rFront);

	static struct BoxDesc lBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-5,0,5, -1,6,9),
		BOXDESC_ROT(0,6,7),
	}; BoxDesc_BuildBox(&Pig_LeftLegBack, &lBack);

	static struct BoxDesc rBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(1,0,5, 5,6,9),
		BOXDESC_ROT(0,6,7),
	}; BoxDesc_BuildBox(&Pig_RightLegBack, &rBack);
}

static Real32 PigModel_GetEyeY(struct Entity* entity) { return 12.0f / 16.0f; }
static void PigModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(14.0f, 14.0f, 14.0f); }

static void PigModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-5.0f, 0.0f, -14.0f,  5.0f, 16.0f, 9.0f);
}

static void PigModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Pig_Head, true);

	IModel_DrawPart(&Pig_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Pig_LeftLegFront,  false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Pig_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Pig_LeftLegBack,   false);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Pig_RightLegBack,  false);
	IModel_UpdateVB();
}

static struct IModel* PigModel_GetInstance(void) {
	IModel_Init(&PigModel);
	IModel_SetPointers(PigModel);
	PigModel.vertices = PigModel_Vertices;
	PigModel.NameYOffset = 1.075f;
	return &PigModel;
}


/*########################################################################################################################*
*---------------------------------------------------------SheepModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Sheep_Head, Sheep_Torso, Sheep_LeftLegFront;
struct ModelPart Sheep_RightLegFront, Sheep_LeftLegBack, Sheep_RightLegBack;
struct ModelPart Fur_Head, Fur_Torso, Fur_LeftLegFront, Fur_RightLegFront;
struct ModelPart Fur_LeftLegBack, Fur_RightLegBack;
struct ModelVertex SheepModel_Vertices[IMODEL_BOX_VERTICES * 6 * 2];
struct IModel SheepModel;
Int32 fur_Index;

static void SheepModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-3,16,-14, 3,22,-6),
		BOXDESC_ROT(0,18,-8),
	}; BoxDesc_BuildBox(&Sheep_Head, &head);

	static struct BoxDesc torso = {
		BOXDESC_TEX(28,8),
		BOXDESC_BOX(-4,12,-8, 4,18,8),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildRotatedBox(&Sheep_Torso, &torso);
	
	static struct BoxDesc lFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-5,0,-7, -1,12,-3),
		BOXDESC_ROT(0,12,-5),
	}; BoxDesc_BuildBox(&Sheep_LeftLegFront, &lFront);

	static struct BoxDesc rFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(1,0,-7, 5,12,-3),
		BOXDESC_ROT(0,12,-5),
	}; BoxDesc_BuildBox(&Sheep_RightLegFront, &rFront);

	static struct BoxDesc lBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-5,0,5, -1,12,9),
		BOXDESC_ROT(0,12,7),
	}; BoxDesc_BuildBox(&Sheep_LeftLegBack, &lBack);

	static struct BoxDesc rBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(1,0,5, 5,12,9),
		BOXDESC_ROT(0,12,7),
	}; BoxDesc_BuildBox(&Sheep_RightLegBack, &rBack);


	static struct BoxDesc fHead = {
		BOXDESC_TEX(0,0),
		BOXDESC_DIMS(-3,16,-12, 3,22,-6),
		BOXDESC_BOUNDS(-3.5f,15.5f,-12.5f, 3.5f,22.5f,-5.5f),
		BOXDESC_ROT(0,18,-8),
	}; BoxDesc_BuildBox(&Fur_Head, &fHead);

	static struct BoxDesc fTorso = {
		BOXDESC_TEX(28,8),
		BOXDESC_DIMS(-4,12,-8, 4,18,8),
		BOXDESC_BOUNDS(-6.0f,10.5f,-10.0f, 6.0f,19.5f,10.0f),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildRotatedBox(&Fur_Torso, &fTorso);

	static struct BoxDesc flFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_DIMS(-5,6,-7, -1,12,-3),
		BOXDESC_BOUNDS(-5.5f,5.5f,-7.5f, -0.5f,12.5f,-2.5f),
		BOXDESC_ROT(0,12,-5),
	}; BoxDesc_BuildBox(&Fur_LeftLegFront, &flFront);

	static struct BoxDesc frFront = {
		BOXDESC_TEX(0,16),
		BOXDESC_DIMS(1,6,-7, 5,12,-3),
		BOXDESC_BOUNDS(0.5f,5.5f,-7.5f, 5.5f,12.5f,-2.5f),
		BOXDESC_ROT(0,12,-5),
	}; BoxDesc_BuildBox(&Fur_RightLegFront, &frFront);

	static struct BoxDesc flBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_DIMS(-5,6,5, -1,12,9),
		BOXDESC_BOUNDS(-5.5f,5.5f,4.5f, -0.5f,12.5f,9.5f),
		BOXDESC_ROT(0,12,7),
	}; BoxDesc_BuildBox(&Fur_LeftLegBack, &flBack);

	static struct BoxDesc frBack = {
		BOXDESC_TEX(0,16),
		BOXDESC_DIMS(1,6,5, 5,12,9),
		BOXDESC_BOUNDS(0.5f,5.5f,4.5f, 5.5f,12.5f,9.5f),
		BOXDESC_ROT(0,12,7),
	}; BoxDesc_BuildBox(&Fur_RightLegBack, &frBack);
}

static Real32 SheepModel_GetEyeY(struct Entity* entity) { return 20.0f / 16.0f; }
static void SheepModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(10.0f, 20.0f, 10.0f); }
static void SheepModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-6.0f, 0.0f, -13.0f,  6.0f, 23.0f, 10.0f);
}

static void SheepModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Sheep_Head, true);

	IModel_DrawPart(&Sheep_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Sheep_LeftLegFront,  false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Sheep_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Sheep_LeftLegBack,   false);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Sheep_RightLegBack,  false);
	IModel_UpdateVB();

	if (entity->ModelIsSheepNoFur) return;
	Gfx_BindTexture(ModelCache_Textures[fur_Index].TexID);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Fur_Head, true);

	IModel_DrawPart(&Fur_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Fur_LeftLegFront,  false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Fur_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, &Fur_LeftLegBack,   false);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, &Fur_RightLegBack,  false);
	IModel_UpdateVB();
}

static struct IModel* SheepModel_GetInstance(void) {
	IModel_Init(&SheepModel);
	IModel_SetPointers(SheepModel);
	SheepModel.vertices = SheepModel_Vertices;
	SheepModel.NameYOffset = 1.48125f;

	String sheep_fur = String_FromConst("sheep_fur.png");
	fur_Index = ModelCache_GetTextureIndex(&sheep_fur);
	return &SheepModel;
}


/*########################################################################################################################*
*-------------------------------------------------------SkeletonModel-----------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Skeleton_Head, Skeleton_Torso, Skeleton_LeftLeg;
struct ModelPart Skeleton_RightLeg, Skeleton_LeftArm, Skeleton_RightArm;
struct ModelVertex SkeletonModel_Vertices[IMODEL_BOX_VERTICES * 6];
struct IModel SkeletonModel;

static void SkeletonModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-4,24,-4, 4,32,4),
		BOXDESC_ROT(0,24,0),
	}; BoxDesc_BuildBox(&Skeleton_Head, &head);

	static struct BoxDesc torso = {
		BOXDESC_TEX(16,16),
		BOXDESC_BOX(-4,12,-2, 4,24,2),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildBox(&Skeleton_Torso, &torso);

	static struct BoxDesc lLeg = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(-1,0,-1, -3,12,1),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildBox(&Skeleton_LeftLeg, &lLeg);

	static struct BoxDesc rLeg = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(1,0,-1, 3,12,1),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildBox(&Skeleton_RightLeg, &rLeg);

	static struct BoxDesc lArm = {
		BOXDESC_TEX(40,16),
		BOXDESC_BOX(-4,12,-1, -6,24,1),
		BOXDESC_ROT(-5,23,0),
	}; BoxDesc_BuildBox(&Skeleton_LeftArm, &lArm);

	static struct BoxDesc rArm = {
		BOXDESC_TEX(40,16),
		BOXDESC_BOX(4,12,-1, 6,24,1),
		BOXDESC_ROT(5,23,0),
	}; BoxDesc_BuildBox(&Skeleton_RightArm, &rArm);
}

static Real32 SkeletonModel_GetEyeY(struct Entity* entity) { return 26.0f / 16.0f; }
static void SkeletonModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(8.0f, 28.1f, 8.0f); }

static void SkeletonModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-4.0f, 0.0f, -4.0f,  4.0f, 32.0f, 4.0f);
}

static void SkeletonModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Skeleton_Head, true);

	IModel_DrawPart(&Skeleton_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0,                      &Skeleton_LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0,                      &Skeleton_RightLeg, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.LeftArmZ,  &Skeleton_LeftArm,  false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.RightArmZ, &Skeleton_RightArm, false);
	IModel_UpdateVB();
}

static void SkeletonModel_DrawArm(struct Entity* entity) {
	IModel_DrawArmPart(&Skeleton_RightArm);
	IModel_UpdateVB();
}

static struct IModel* SkeletonModel_GetInstance(void) {
	IModel_Init(&SkeletonModel);
	IModel_SetPointers(SkeletonModel);
	SkeletonModel.DrawArm = SkeletonModel_DrawArm;
	SkeletonModel.armX = 5;
	SkeletonModel.vertices = SkeletonModel_Vertices;
	SkeletonModel.NameYOffset = 2.075f;
	return &SkeletonModel;
}


/*########################################################################################################################*
*--------------------------------------------------------SpiderModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Spider_Head, Spider_Link, Spider_End;
struct ModelPart Spider_LeftLeg, Spider_RightLeg;
struct ModelVertex SpiderModel_Vertices[IMODEL_BOX_VERTICES * 5];
struct IModel SpiderModel;

static void SpiderModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(32,4),
		BOXDESC_BOX(-4,4,-11, 4,12,-3),
		BOXDESC_ROT(0,8,-3),
	}; BoxDesc_BuildBox(&Spider_Head, &head);

	static struct BoxDesc link = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-3,5,3, 3,11,-3),
		BOXDESC_ROT(0,5,0),
	}; BoxDesc_BuildBox(&Spider_Link, &link);

	static struct BoxDesc end = {
		BOXDESC_TEX(0,12),
		BOXDESC_BOX(-5,4,3, 5,12,15),
		BOXDESC_ROT(0,4,9),
	}; BoxDesc_BuildBox(&Spider_End, &end);

	static struct BoxDesc lLeg = {
		BOXDESC_TEX(18,0),
		BOXDESC_BOX(-19,7,-1, -3,9,1),
		BOXDESC_ROT(-3,8,0),
	}; BoxDesc_BuildBox(&Spider_LeftLeg, &lLeg);

	static struct BoxDesc rLeg = {
		BOXDESC_TEX(18,0),
		BOXDESC_BOX(3,7,-1, 19,9,1),
		BOXDESC_ROT(3,8,0),
	}; BoxDesc_BuildBox(&Spider_RightLeg, &rLeg);
}

static Real32 SpiderModel_GetEyeY(struct Entity* entity) { return 8.0f / 16.0f; }
static void SpiderModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(15.0f, 12.0f, 15.0f); }

static void SpiderModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-5.0f, 0.0f, -11.0f,  5.0f, 12.0f, 15.0f);
}

#define quarterPi (MATH_PI / 4.0f)
#define eighthPi  (MATH_PI / 8.0f)

static void SpiderModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Spider_Head, true);
	IModel_DrawPart(&Spider_Link);
	IModel_DrawPart(&Spider_End);

	Real32 rotX = Math_SinF(entity->Anim.WalkTime)     * entity->Anim.Swing * MATH_PI;
	Real32 rotZ = Math_CosF(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 16.0f;
	Real32 rotY = Math_SinF(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 32.0f;
	IModel_Rotation = ROTATE_ORDER_XZY;

	IModel_DrawRotate(rotX,  quarterPi  + rotY, eighthPi + rotZ, &Spider_LeftLeg, false);
	IModel_DrawRotate(-rotX,  eighthPi  + rotY, eighthPi + rotZ, &Spider_LeftLeg, false);
	IModel_DrawRotate(rotX,  -eighthPi  - rotY, eighthPi - rotZ, &Spider_LeftLeg, false);
	IModel_DrawRotate(-rotX, -quarterPi - rotY, eighthPi - rotZ, &Spider_LeftLeg, false);

	IModel_DrawRotate(rotX, -quarterPi + rotY, -eighthPi + rotZ, &Spider_RightLeg, false);
	IModel_DrawRotate(-rotX, -eighthPi + rotY, -eighthPi + rotZ, &Spider_RightLeg, false);
	IModel_DrawRotate(rotX,   eighthPi - rotY, -eighthPi - rotZ, &Spider_RightLeg, false);
	IModel_DrawRotate(-rotX, quarterPi - rotY, -eighthPi - rotZ, &Spider_RightLeg, false);

	IModel_Rotation = ROTATE_ORDER_ZYX;
	IModel_UpdateVB();
}

static struct IModel* SpiderModel_GetInstance(void) {
	IModel_Init(&SpiderModel);
	IModel_SetPointers(SpiderModel);
	SpiderModel.vertices = SpiderModel_Vertices;
	SpiderModel.NameYOffset = 1.0125f;
	return &SpiderModel;
}


/*########################################################################################################################*
*--------------------------------------------------------ZombieModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelPart Zombie_Head, Zombie_Hat, Zombie_Torso, Zombie_LeftLeg;
struct ModelPart Zombie_RightLeg, Zombie_LeftArm, Zombie_RightArm;
struct ModelVertex ZombieModel_Vertices[IMODEL_BOX_VERTICES * 7];
struct IModel ZombieModel;

static void ZombieModel_CreateParts(void) {
	static struct BoxDesc head = {
		BOXDESC_TEX(0,0),
		BOXDESC_BOX(-4,24,-4, 4,32,4),
		BOXDESC_ROT(0,24,0),
	}; BoxDesc_BuildBox(&Zombie_Head, &head);

	static struct BoxDesc hat = {
		BOXDESC_TEX(0,0),
		BOXDESC_DIMS(-4,24,-4, 4,32,4),
		BOXDESC_BOUNDS(-4.5f,23.5f,-4.5f, 4.5f,32.5f,4.5f),
		BOXDESC_ROT(0,24,0),
	}; BoxDesc_BuildBox(&Zombie_Hat, &hat);

	static struct BoxDesc torso = {
		BOXDESC_TEX(16,16),
		BOXDESC_BOX(-4,12,-2, 4,24,2),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildBox(&Zombie_Torso, &torso);

	static struct BoxDesc lLeg = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(0,0,-2, -4,12,2),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildBox(&Zombie_LeftLeg, &lLeg);

	static struct BoxDesc rLeg = {
		BOXDESC_TEX(0,16),
		BOXDESC_BOX(0,0,-2, 4,12,2),
		BOXDESC_ROT(0,12,0),
	}; BoxDesc_BuildBox(&Zombie_RightLeg, &rLeg);

	static struct BoxDesc lArm = {
		BOXDESC_TEX(40,16),
		BOXDESC_BOX(-4,12,-2, -8,24,2),
		BOXDESC_ROT(-6,22,0),
	}; BoxDesc_BuildBox(&Zombie_LeftArm, &lArm);

	static struct BoxDesc rArm = {
		BOXDESC_TEX(40,16),
		BOXDESC_BOX(4,12,-2, 8,24,2),
		BOXDESC_ROT(6,22,0),
	}; BoxDesc_BuildBox(&Zombie_RightArm, &rArm);
}

static Real32 ZombieModel_GetEyeY(struct Entity* entity) { return 26.0f / 16.0f; }
static void ZombieModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(8.6f, 28.1f, 8.6f); }
static void ZombieModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-4.0f, 0.0f, -4.0f,  4.0f, 32.0f, 4.0f);
}

static void ZombieModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Zombie_Head, true);

	IModel_DrawPart(&Zombie_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0,                      &Zombie_LeftLeg,  false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0,                      &Zombie_RightLeg, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.LeftArmZ,  &Zombie_LeftArm,  false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.RightArmZ, &Zombie_RightArm, false);

	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &Zombie_Hat, true);
	IModel_UpdateVB();
}

static void ZombieModel_DrawArm(struct Entity* entity) {
	IModel_DrawArmPart(&Zombie_RightArm);
	IModel_UpdateVB();
}

static struct IModel* ZombieModel_GetInstance(void) {
	IModel_Init(&ZombieModel);
	IModel_SetPointers(ZombieModel);
	ZombieModel.DrawArm = ZombieModel_DrawArm;
	ZombieModel.vertices = ZombieModel_Vertices;
	ZombieModel.NameYOffset = 2.075f;
	return &ZombieModel;
}


/*########################################################################################################################*
*---------------------------------------------------------HumanModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelSet {
	struct ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat,
		TorsoLayer, LeftLegLayer, RightLegLayer, LeftArmLayer, RightArmLayer;
};

struct BoxDesc head, torso, lLeg, rLeg, lArm, rArm;
Real32 offset;
static void HumanModel_CreateParts(struct ModelSet* models) {
	struct ModelSet* set     = &models[0];
	struct ModelSet* set64   = &models[1];
	struct ModelSet* setSlim = &models[2];

	BoxDesc_TexOrigin(&head, 0, 0);
	BoxDesc_BuildBox(&set->Head, &head);

	BoxDesc_TexOrigin(&torso, 16, 16);
	BoxDesc_BuildBox(&set->Torso, &torso);

	BoxDesc_TexOrigin(&head, 32, 0);
	BoxDesc_Expand(&head, offset);
	BoxDesc_BuildBox(&set->Hat, &head);

	BoxDesc_TexOrigin(&lLeg, 0, 16);
	BoxDesc_MirrorX(&lLeg);
	BoxDesc_BuildBox(&set->LeftLeg, &lLeg);

	BoxDesc_TexOrigin(&rLeg, 0, 16);
	BoxDesc_BuildBox(&set->RightLeg, &rLeg);

	BoxDesc_TexOrigin(&lArm, 40, 16);
	BoxDesc_MirrorX(&lArm);
	BoxDesc_BuildBox(&set->LeftArm, &lArm);

	BoxDesc_TexOrigin(&rArm, 40, 16);
	BoxDesc_BuildBox(&set->RightArm, &rArm);

	set64->Head = set->Head;
	set64->Torso = set->Torso;
	set64->Hat = set->Hat;

	BoxDesc_MirrorX(&lLeg);
	BoxDesc_TexOrigin(&lLeg, 16, 48);
	BoxDesc_BuildBox(&set64->LeftLeg, &lLeg);
	set64->RightLeg = set->RightLeg;

	BoxDesc_MirrorX(&lArm);
	BoxDesc_TexOrigin(&lArm, 32, 48);
	BoxDesc_BuildBox(&set64->LeftArm, &lArm);
	set64->RightArm = set->RightArm;

	BoxDesc_TexOrigin(&torso, 16, 32);
	BoxDesc_Expand(&torso, offset);
	BoxDesc_BuildBox(&set64->TorsoLayer, &torso);

	BoxDesc_TexOrigin(&lLeg, 0, 48);
	BoxDesc_Expand(&lLeg, offset);
	BoxDesc_BuildBox(&set64->LeftLegLayer, &lLeg);

	BoxDesc_TexOrigin(&rLeg, 0, 32);
	BoxDesc_Expand(&rLeg, offset);
	BoxDesc_BuildBox(&set64->RightLegLayer, &rLeg);

	BoxDesc_TexOrigin(&lArm, 48, 48);
	BoxDesc_Expand(&lArm, offset);
	BoxDesc_BuildBox(&set64->LeftArmLayer, &lArm);

	BoxDesc_TexOrigin(&rArm, 40, 32);
	BoxDesc_Expand(&rArm, offset);
	BoxDesc_BuildBox(&set64->RightArmLayer, &rArm);

	setSlim->Head     = set64->Head;
	setSlim->Torso    = set64->Torso;
	setSlim->Hat      = set64->Hat;
	setSlim->LeftLeg  = set64->LeftLeg;
	setSlim->RightLeg = set64->RightLeg;

	lArm.SizeX -= 1; lArm.X1 += (offset * 2.0f) / 16.0f;
	BoxDesc_TexOrigin(&lArm, 32, 48);
	BoxDesc_BuildBox(&setSlim->LeftArm, &lArm);

	rArm.SizeX -= 1; rArm.X2 -= (offset * 2.0f) / 16.0f;
	BoxDesc_TexOrigin(&rArm, 40, 16);
	BoxDesc_BuildBox(&setSlim->RightArm, &rArm);

	setSlim->TorsoLayer    = set64->TorsoLayer;
	setSlim->LeftLegLayer  = set64->LeftLegLayer;
	setSlim->RightLegLayer = set64->RightLegLayer;

	BoxDesc_TexOrigin(&lArm, 48, 48);
	BoxDesc_Expand(&lArm, offset);
	BoxDesc_BuildBox(&setSlim->LeftArmLayer, &lArm);

	BoxDesc_TexOrigin(&rArm, 40, 32);
	BoxDesc_Expand(&rArm, offset);
	BoxDesc_BuildBox(&setSlim->RightArmLayer, &rArm);
}

static void HumanModel_DrawModel(struct Entity* entity, struct ModelSet* models) {
	IModel_ApplyTexture(entity);
	Gfx_SetAlphaTest(false);

	UInt8 type = IModel_skinType;
	struct ModelSet* model = &models[type == SKIN_TYPE_64x64_SLIM ? 2 : (type == SKIN_TYPE_64x64 ? 1 : 0)];

	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &model->Head, true);
	IModel_DrawPart(&model->Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX,  0, entity->Anim.LeftLegZ,  &model->LeftLeg,  false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, entity->Anim.RightLegZ, &model->RightLeg, false);

	IModel_Rotation = ROTATE_ORDER_XZY;
	IModel_DrawRotate(entity->Anim.LeftArmX,  0, entity->Anim.LeftArmZ,  &model->LeftArm,  false);
	IModel_DrawRotate(entity->Anim.RightArmX, 0, entity->Anim.RightArmZ, &model->RightArm, false);
	IModel_Rotation = ROTATE_ORDER_ZYX;
	IModel_UpdateVB();

	Gfx_SetAlphaTest(true);
	IModel_ActiveModel->index = 0;
	if (IModel_skinType != SKIN_TYPE_64x32) {
		IModel_DrawPart(&model->TorsoLayer);
		IModel_DrawRotate(entity->Anim.LeftLegX,  0, entity->Anim.LeftLegZ,  &model->LeftLegLayer,  false);
		IModel_DrawRotate(entity->Anim.RightLegX, 0, entity->Anim.RightLegZ, &model->RightLegLayer, false);

		IModel_Rotation = ROTATE_ORDER_XZY;
		IModel_DrawRotate(entity->Anim.LeftArmX,  0, entity->Anim.LeftArmZ,  &model->LeftArmLayer,  false);
		IModel_DrawRotate(entity->Anim.RightArmX, 0, entity->Anim.RightArmZ, &model->RightArmLayer, false);
		IModel_Rotation = ROTATE_ORDER_ZYX;
	}
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &model->Hat, true);
	IModel_UpdateVB();
}

static void HumanModel_DrawArm(struct Entity* entity, struct ModelSet* models) {
	UInt8 type = IModel_skinType;
	struct ModelSet* model = &models[type == SKIN_TYPE_64x64_SLIM ? 2 : (type == SKIN_TYPE_64x64 ? 1 : 0)];

	IModel_DrawArmPart(&model->RightArm);
	if (IModel_skinType != SKIN_TYPE_64x32) {
		IModel_DrawArmPart(&model->RightArmLayer);
	}
	IModel_UpdateVB();
}


/*########################################################################################################################*
*-------------------------------------------------------HumanoidModel-----------------------------------------------------*
*#########################################################################################################################*/
struct ModelSet Humanoid_Set[3];
struct ModelVertex HumanoidModel_Vertices[IMODEL_BOX_VERTICES * (7 + 7 + 4)];
struct IModel HumanoidModel;

static void HumanoidModel_MakeBoxDescs(void) {
	BoxDesc_Box(&head, -4, 24, -4, 4, 32, 4);
	BoxDesc_RotOrigin(&head, 0, 24, 0);
	BoxDesc_Box(&torso, -4, 12, -2, 4, 24, 2);

	BoxDesc_Box(&lLeg, -4, 0, -2, 0, 12, 2);
	BoxDesc_RotOrigin(&lLeg, 0, 12, 0);
	BoxDesc_Box(&rLeg, 0, 0, -2, 4, 12, 2);
	BoxDesc_RotOrigin(&rLeg, 0, 12, 0);

	BoxDesc_Box(&lArm, -8, 12, -2, -4, 24, 2);
	BoxDesc_RotOrigin(&lArm, -5, 22, 0);
	BoxDesc_Box(&rArm, 4, 12, -2, 8, 24, 2);
	BoxDesc_RotOrigin(&rArm, 5, 22, 0);
}

static void HumanoidModel_CreateParts(void) {
	HumanoidModel_MakeBoxDescs();
	offset = 0.5f;
	HumanModel_CreateParts(Humanoid_Set);
}

static Real32 HumanoidModel_GetEyeY(struct Entity* entity) { return 26.0f / 16.0f; }
static void HumanoidModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(8.6f, 28.1f, 8.6f); }
static void HumanoidModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-8.0f, 0.0f, -4.0f,  8.0f, 32.0f, 4.0f);
}

static void HumanoidModel_DrawModel(struct Entity* entity) {
	HumanModel_DrawModel(entity, Humanoid_Set);
}

static void HumanoidModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArm(entity, Humanoid_Set);
}

static struct IModel* HumanoidModel_GetInstance(void) {
	IModel_Init(&HumanoidModel);
	IModel_SetPointers(HumanoidModel);
	HumanoidModel.DrawArm = HumanoidModel_DrawArm;
	HumanoidModel.vertices = HumanoidModel_Vertices;
	HumanoidModel.CalcHumanAnims = true;
	HumanoidModel.UsesHumanSkin  = true;
	HumanoidModel.NameYOffset = 32.5f / 16.0f;
	return &HumanoidModel;
}


/*########################################################################################################################*
*---------------------------------------------------------ChibiModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelSet Chibi_Set[3];
struct ModelVertex ChibiModel_Vertices[IMODEL_BOX_VERTICES * (7 + 7 + 4)];
struct IModel ChibiModel;
#define CHIBI_SIZE 0.5f

static void ChibiModel_MakeBoxDescs(void) {
	HumanoidModel_MakeBoxDescs();
	BoxDesc_Box(&head, -4, 12, -4, 4, 20, 4);
	BoxDesc_RotOrigin(&head, 0, 13, 0);

	BoxDesc_Scale(&torso, CHIBI_SIZE);
	BoxDesc_Scale(&lLeg, CHIBI_SIZE);
	BoxDesc_Scale(&rLeg, CHIBI_SIZE);
	BoxDesc_Scale(&lArm, CHIBI_SIZE);
	BoxDesc_Scale(&rArm, CHIBI_SIZE);
}

static void ChibiModel_CreateParts(void) {
	ChibiModel_MakeBoxDescs();
	offset = 0.5f * CHIBI_SIZE;
	HumanModel_CreateParts(Chibi_Set);
}

static Real32 ChibiModel_GetEyeY(struct Entity* entity) { return 14.0f / 16.0f; }
static void ChibiModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(4.6f, 20.1f, 4.6f); }
static void ChibiModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-4.0f, 0.0f, -4.0f,  4.0f, 16.0f, 4.0f);
}

static void ChibiModel_DrawModel(struct Entity* entity) {
	HumanModel_DrawModel(entity, Chibi_Set);
}

static void ChibiModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArm(entity, Chibi_Set);
}

static struct IModel* ChibiModel_GetInstance(void) {
	IModel_Init(&ChibiModel);
	IModel_SetPointers(ChibiModel);
	ChibiModel.DrawArm = ChibiModel_DrawArm;
	ChibiModel.armX = 3; ChibiModel.armY = 6;
	ChibiModel.vertices = ChibiModel_Vertices;
	ChibiModel.CalcHumanAnims = true;
	ChibiModel.UsesHumanSkin  = true;
	ChibiModel.MaxScale    = 3.0f;
	ChibiModel.ShadowScale = 0.5f;
	ChibiModel.NameYOffset = 20.2f / 16.0f;
	return &ChibiModel;
}


/*########################################################################################################################*
*--------------------------------------------------------SittingModel-----------------------------------------------------*
*#########################################################################################################################*/
struct IModel SittingModel;
#define SIT_OFFSET 10.0f
static void SittingModel_CreateParts(void) { }

static Real32 SittingModel_GetEyeY(struct Entity* entity) { return (26.0f - SIT_OFFSET) / 16.0f; }
static void SittingModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(8.6f, 28.1f - SIT_OFFSET, 8.6f); }
static void SittingModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-8.0f, 0.0f, -4.0f,  8.0f, 32.0f - SIT_OFFSET, 4.0f);
}

static void SittingModel_GetTransform(struct Entity* entity, Vector3 pos, struct Matrix* m) {
	pos.Y -= (SIT_OFFSET / 16.0f) * entity->ModelScale.Y;
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}

static void SittingModel_DrawModel(struct Entity* entity) {
	entity->Anim.LeftLegX = 1.5f;  entity->Anim.RightLegX = 1.5f;
	entity->Anim.LeftLegZ = -0.1f; entity->Anim.RightLegZ = 0.1f;
	HumanoidModel_DrawModel(entity);
}

static struct IModel* SittingModel_GetInstance(void) {
	IModel_Init(&SittingModel);
	IModel_SetPointers(SittingModel);
	SittingModel.DrawArm = HumanoidModel_DrawArm;
	SittingModel.vertices = HumanoidModel_Vertices;
	SittingModel.CalcHumanAnims = true;
	SittingModel.UsesHumanSkin  = true;
	SittingModel.ShadowScale  = 0.5f;
	SittingModel.GetTransform = SittingModel_GetTransform;
	SittingModel.NameYOffset  = 32.5f / 16.0f;
	return &SittingModel;
}


/*########################################################################################################################*
*--------------------------------------------------------CorpseModel------------------------------------------------------*
*#########################################################################################################################*/
struct IModel CorpseModel;
static void CorpseModel_CreateParts(void) { }
static void CorpseModel_DrawModel(struct Entity* entity) {
	entity->Anim.LeftLegX = 0.025f; entity->Anim.RightLegX = 0.025f;
	entity->Anim.LeftArmX = 0.025f; entity->Anim.RightArmX = 0.025f;
	entity->Anim.LeftLegZ = -0.15f; entity->Anim.RightLegZ =  0.15f;
	entity->Anim.LeftArmZ = -0.20f; entity->Anim.RightArmZ =  0.20f;
	HumanoidModel_DrawModel(entity);
}

static struct IModel* CorpseModel_GetInstance(void) {
	CorpseModel = HumanoidModel;
	CorpseModel.CreateParts = CorpseModel_CreateParts;
	CorpseModel.DrawModel   = CorpseModel_DrawModel;
	return &CorpseModel;
}


/*########################################################################################################################*
*---------------------------------------------------------HeadModel-------------------------------------------------------*
*#########################################################################################################################*/
struct IModel HeadModel;
static void HeadModel_CreateParts(void) { }

static Real32 HeadModel_GetEyeY(struct Entity* entity) { return 6.0f / 16.0f; }
static void HeadModel_GetCollisionSize(Vector3* size) { MODEL_RET_SIZE(7.9f, 7.9f, 7.9f); }
static void HeadModel_GetPickingBounds(struct AABB* bb) {
	MODEL_RET_AABB(-4.0f, 0.0f, -4.0f,  4.0f, 8.0f, 4.0f);
}

static void HeadModel_GetTransform(struct Entity* entity, Vector3 pos, struct Matrix* m) {
	pos.Y -= (24.0f / 16.0f) * entity->ModelScale.Y;
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}

static void HeadModel_DrawModel(struct Entity* entity) {
	IModel_ApplyTexture(entity);
	struct ModelPart part;

	part = Humanoid_Set[0].Head; part.RotY += 4.0f / 16.0f;
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &part, true);
	part = Humanoid_Set[0].Hat;  part.RotY += 4.0f / 16.0f;
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &part, true);

	IModel_UpdateVB();
}

static struct IModel* HeadModel_GetInstance(void) {
	IModel_Init(&HeadModel);
	IModel_SetPointers(HeadModel);
	HeadModel.vertices = HumanoidModel_Vertices;
	HeadModel.UsesHumanSkin  = true;
	HeadModel.Pushes         = false;
	HeadModel.GetTransform = HeadModel_GetTransform;
	HeadModel.NameYOffset  = 32.5f / 16.0f;
	return &HeadModel;
}


/*########################################################################################################################*
*---------------------------------------------------------BlockModel------------------------------------------------------*
*#########################################################################################################################*/
struct IModel BlockModel;
BlockID BlockModel_block = BLOCK_AIR;
Vector3 BlockModel_minBB, BlockModel_maxBB;
Int32 BlockModel_lastTexIndex = -1, BlockModel_texIndex;

static void BlockModel_CreateParts(void) { }

static Real32 BlockModel_GetEyeY(struct Entity* entity) {
	BlockID block = entity->ModelBlock;
	Real32 minY = Block_MinBB[block].Y;
	Real32 maxY = Block_MaxBB[block].Y;
	return block == BLOCK_AIR ? 1 : (minY + maxY) / 2.0f;
}

static void BlockModel_GetCollisionSize(Vector3* size) {
	Vector3_Sub(size, &BlockModel_maxBB, &BlockModel_minBB);
	/* to fit slightly inside */
	Vector3 sizeShrink = VECTOR3_CONST1(0.75f / 16.0f);
	Vector3_SubBy(size, &sizeShrink);
}

static void BlockModel_GetPickingBounds(struct AABB* bb) {
	static Vector3 offset = { -0.5f, 0.0f, -0.5f };
	Vector3_Add(&bb->Min, &BlockModel_minBB, &offset);
	Vector3_Add(&bb->Max, &BlockModel_maxBB, &offset);
}

static void BlockModel_RecalcProperties(struct Entity* p) {
	BlockID block = p->ModelBlock;
	Real32 height;

	if (Block_Draw[block] == DRAW_GAS) {
		Vector3 zero = Vector3_Zero; BlockModel_minBB = zero;
		Vector3 one  = Vector3_One;  BlockModel_maxBB = one;
		height = 1.0f;
	} else {
		BlockModel_minBB = Block_MinBB[block];
		BlockModel_maxBB = Block_MaxBB[block];
		height = BlockModel_maxBB.Y - BlockModel_minBB.Y;
	}
	BlockModel.NameYOffset = height + 0.075f;
}

static void BlockModel_Flush(void) {
	if (BlockModel_lastTexIndex != -1) {
		Gfx_BindTexture(Atlas1D_TexIds[BlockModel_lastTexIndex]);
		IModel_UpdateVB();
	}

	BlockModel_lastTexIndex = BlockModel_texIndex;
	BlockModel.index = 0;
}

#define BlockModel_FlushIfNotSame if (BlockModel_lastTexIndex != BlockModel_texIndex) { BlockModel_Flush(); }
static TextureLoc BlockModel_GetTex(Face face, VertexP3fT2fC4b** ptr) {
	TextureLoc texLoc = Block_GetTexLoc(BlockModel_block, face);
	BlockModel_texIndex = Atlas1D_Index(texLoc);
	BlockModel_FlushIfNotSame;

	/* Need to reload ptr, in case was flushed */
	*ptr = &ModelCache_Vertices[BlockModel.index];
	BlockModel.index += 4;
	return texLoc;
}

#define Block_Tint(col, block)\
if (Block_Tinted[block]) {\
	PackedCol tintCol = Block_FogCol[block];\
	col.R = (UInt8)(col.R * tintCol.R / 255);\
	col.G = (UInt8)(col.G * tintCol.G / 255);\
	col.B = (UInt8)(col.B * tintCol.B / 255);\
}

static void BlockModel_SpriteZQuad(bool firstPart, bool mirror) {
	TextureLoc texLoc = Block_GetTexLoc(BlockModel_block, FACE_ZMAX);
	struct TextureRec rec = Atlas1D_TexRec(texLoc, 1, &BlockModel_texIndex);
	BlockModel_FlushIfNotSame;

	PackedCol col = IModel_Cols[0];
	Block_Tint(col, BlockModel_block);

	Real32 p1 = 0.0f, p2 = 0.0f;
	if (firstPart) { /* Need to break into two quads for when drawing a sprite model in hand. */
		if (mirror) { rec.U1 = 0.5f; p1 = -5.5f / 16.0f; }
		else {        rec.U2 = 0.5f; p2 = -5.5f / 16.0f; }
	} else {
		if (mirror) { rec.U2 = 0.5f; p2 = 5.5f / 16.0f; }
		else {        rec.U1 = 0.5f; p1 = 5.5f / 16.0f; }
	}

	VertexP3fT2fC4b* ptr = &ModelCache_Vertices[BlockModel.index];
	VertexP3fT2fC4b v; v.Col = col;

	v.X = p1; v.Y = 0.0f; v.Z = p1; v.U = rec.U2; v.V = rec.V2; *ptr++ = v;
	          v.Y = 1.0f;                         v.V = rec.V1; *ptr++ = v;
	v.X = p2;             v.Z = p2; v.U = rec.U1;               *ptr++ = v;
	          v.Y = 0.0f;                         v.V = rec.V2; *ptr++ = v;
	BlockModel.index += 4;
}

static void BlockModel_SpriteXQuad(bool firstPart, bool mirror) {
	TextureLoc texLoc = Block_GetTexLoc(BlockModel_block, FACE_XMAX);
	struct TextureRec rec = Atlas1D_TexRec(texLoc, 1, &BlockModel_texIndex);
	BlockModel_FlushIfNotSame;

	PackedCol col = IModel_Cols[0];
	Block_Tint(col, BlockModel_block);

	Real32 x1 = 0.0f, x2 = 0.0f, z1 = 0.0f, z2 = 0.0f;
	if (firstPart) {
		if (mirror) { rec.U2 = 0.5f; x2 = -5.5f / 16.0f; z2 = 5.5f / 16.0f; }
		else {        rec.U1 = 0.5f; x1 = -5.5f / 16.0f; z1 = 5.5f / 16.0f; }
	} else {
		if (mirror) { rec.U1 = 0.5f; x1 = 5.5f / 16.0f; z1 = -5.5f / 16.0f; }
		else {        rec.U2 = 0.5f; x2 = 5.5f / 16.0f; z2 = -5.5f / 16.0f; }
	}

	VertexP3fT2fC4b* ptr = &ModelCache_Vertices[BlockModel.index];
	VertexP3fT2fC4b v; v.Col = col;

	v.X = x1; v.Y = 0.0f; v.Z = z1; v.U = rec.U2; v.V = rec.V2; *ptr++ = v;
	          v.Y = 1.0f;                         v.V = rec.V1; *ptr++ = v;
	v.X = x2;             v.Z = z2; v.U = rec.U1;               *ptr++ = v;
	          v.Y = 0.0f;                         v.V = rec.V2; *ptr++ = v;
	BlockModel.index += 4;
}

static void BlockModel_DrawParts(bool sprite) {
	if (sprite) {
		BlockModel_SpriteXQuad(false, false);
		BlockModel_SpriteXQuad(false, true);
		BlockModel_SpriteZQuad(false, false);
		BlockModel_SpriteZQuad(false, true);

		BlockModel_SpriteZQuad(true, false);
		BlockModel_SpriteZQuad(true, true);
		BlockModel_SpriteXQuad(true, false);
		BlockModel_SpriteXQuad(true, true);
	} else {
		Drawer_MinBB = Block_MinBB[BlockModel_block]; Drawer_MinBB.Y = 1.0f - Drawer_MinBB.Y;
		Drawer_MaxBB = Block_MaxBB[BlockModel_block]; Drawer_MaxBB.Y = 1.0f - Drawer_MaxBB.Y;

		Vector3 min = Block_RenderMinBB[BlockModel_block];
		Vector3 max = Block_RenderMaxBB[BlockModel_block];
		Drawer_X1 = min.X - 0.5f; Drawer_Y1 = min.Y; Drawer_Z1 = min.Z - 0.5f;
		Drawer_X2 = max.X - 0.5f; Drawer_Y2 = max.Y; Drawer_Z2 = max.Z - 0.5f;

		Drawer_Tinted = Block_Tinted[BlockModel_block];
		Drawer_TintColour = Block_FogCol[BlockModel_block];
		VertexP3fT2fC4b* ptr = NULL;
		TextureLoc loc;

		loc = BlockModel_GetTex(FACE_YMIN, &ptr); Drawer_YMin(1, IModel_Cols[1], loc, &ptr);
		loc = BlockModel_GetTex(FACE_ZMIN, &ptr); Drawer_ZMin(1, IModel_Cols[3], loc, &ptr);
		loc = BlockModel_GetTex(FACE_XMAX, &ptr); Drawer_XMax(1, IModel_Cols[5], loc, &ptr);
		loc = BlockModel_GetTex(FACE_ZMAX, &ptr); Drawer_ZMax(1, IModel_Cols[2], loc, &ptr);
		loc = BlockModel_GetTex(FACE_XMIN, &ptr); Drawer_XMin(1, IModel_Cols[4], loc, &ptr);
		loc = BlockModel_GetTex(FACE_YMAX, &ptr); Drawer_YMax(1, IModel_Cols[0], loc, &ptr);
	}
}

static void BlockModel_DrawModel(struct Entity* p) {
	BlockModel_block = p->ModelBlock;
	BlockModel_RecalcProperties(p);
	if (Block_Draw[BlockModel_block] == DRAW_GAS) return;

	if (Block_FullBright[BlockModel_block]) {
		UInt32 i;
		PackedCol white = PACKEDCOL_WHITE;
		for (i = 0; i < FACE_COUNT; i++) {
			IModel_Cols[i] = white;
		}
	}

	BlockModel_lastTexIndex = -1;
	bool sprite = Block_Draw[BlockModel_block] == DRAW_SPRITE;
	BlockModel_DrawParts(sprite);
	if (!BlockModel.index) return;

	if (sprite) Gfx_SetFaceCulling(true);
	BlockModel_lastTexIndex = BlockModel_texIndex;
	BlockModel_Flush();
	if (sprite) Gfx_SetFaceCulling(false);
}

static struct IModel* BlockModel_GetInstance(void) {
	IModel_Init(&BlockModel);
	IModel_SetPointers(BlockModel);
	BlockModel.Bobbing  = false;
	BlockModel.UsesSkin = false;
	BlockModel.Pushes   = false;
	BlockModel.RecalcProperties = BlockModel_RecalcProperties;
	return &BlockModel;
}


/*########################################################################################################################*
*---------------------------------------------------------ModelCache------------------------------------------------------*
*#########################################################################################################################*/
static void ModelCache_RegisterDefaultModels(void) {
	ModelCache_RegisterTexture("char.png");
	ModelCache_RegisterTexture("chicken.png");
	ModelCache_RegisterTexture("creeper.png");
	ModelCache_RegisterTexture("pig.png");
	ModelCache_RegisterTexture("sheep.png");
	ModelCache_RegisterTexture("sheep_fur.png");
	ModelCache_RegisterTexture("skeleton.png");
	ModelCache_RegisterTexture("spider.png");
	ModelCache_RegisterTexture("zombie.png");

	ModelCache_Register("humanoid", "char.png", HumanoidModel_GetInstance());
	ModelCache_InitModel(&HumanoidModel);

	ModelCache_Register("chicken", "chicken.png", ChickenModel_GetInstance());
	ModelCache_Register("creeper", "creeper.png", CreeperModel_GetInstance());
	ModelCache_Register("pig", "pig.png", PigModel_GetInstance());
	ModelCache_Register("sheep", "sheep.png", SheepModel_GetInstance());
	ModelCache_Register("sheep_nofur", "sheep.png", &SheepModel);
	ModelCache_Register("skeleton", "skeleton.png", SkeletonModel_GetInstance());
	ModelCache_Register("spider", "spider.png", SpiderModel_GetInstance());
	ModelCache_Register("zombie", "zombie.png", ZombieModel_GetInstance());

	ModelCache_Register("block", NULL, BlockModel_GetInstance());
	ModelCache_Register("chibi", "char.png", ChibiModel_GetInstance());
	ModelCache_Register("head", "char.png", HeadModel_GetInstance());
	ModelCache_Register("sit", "char.png", SittingModel_GetInstance());
	ModelCache_Register("sitting", "char.png", &SittingModel);
	ModelCache_Register("corpse", "char.png", CorpseModel_GetInstance());
}

void ModelCache_Init(void) {
	ModelCache_RegisterDefaultModels();
	ModelCache_ContextRecreated(NULL);

	Event_RegisterEntry(&TextureEvents_FileChanged, NULL, ModelCache_TextureChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,      NULL, ModelCache_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, NULL, ModelCache_ContextRecreated);
}

void ModelCache_Free(void) {
	UInt32 i;
	for (i = 0; i < ModelCache_texCount; i++) {
		struct CachedTexture* tex = &ModelCache_Textures[i];
		Gfx_DeleteTexture(&tex->TexID);
	}
	ModelCache_ContextLost(NULL);

	Event_UnregisterEntry(&TextureEvents_FileChanged, NULL, ModelCache_TextureChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,      NULL, ModelCache_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, NULL, ModelCache_ContextRecreated);
}
