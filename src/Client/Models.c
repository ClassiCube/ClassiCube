#include "IModel.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"
#include "ModelCache.h"

ModelPart Chicken_Head, Chicken_Head2, Chicken_Head3, Chicken_Torso;
ModelPart Chicken_LeftLeg, Chicken_RightLeg, Chicken_LeftWing, Chicken_RightWing;
ModelVertex ChickenModel_Vertices[IModel_BoxVertices * 6 + (IModel_QuadVertices * 2) * 2];
IModel ChickenModel;

ModelPart ChickenModel_MakeLeg(Int32 x1, Int32 x2, Int32 legX1, Int32 legX2) {
#define ch_y1 (1.0f  / 64.0f)
#define ch_y2 (5.0f  / 16.0f)
#define ch_z2 (1.0f  / 16.0f)
#define ch_z1 (-2.0f / 16.0f)

	IModel* m = &ChickenModel;
	BoxDesc_YQuad(m, 32, 0, 3, 3,
		x2 / 16.0f, x1 / 16.0f, ch_z1, ch_z2, ch_y1, false); /* bottom feet */
	BoxDesc_ZQuad(m, 36, 3, 1, 5,
		legX1 / 16.0f, legX2 / 16.0f, ch_y1, ch_y2, ch_z2, false); /* vertical part of leg */

	ModelPart part;
	ModelPart_Init(&part, m->index - IModel_QuadVertices * 2, IModel_QuadVertices * 2,
		0.0f / 16.0f, 5.0f / 16.0f, 1.0f / 16.0f);
	return part;
}

void ChickenModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -2, 9, -6, 2, 15, -3);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 9, -4);
	Chicken_Head = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, -1, 9, -7, 1, 11, -5);
	BoxDesc_TexOrigin(&desc, 14, 4); /* TODO: Find a more appropriate name. */
	BoxDesc_RotOrigin(&desc, 0, 9, -4);
	Chicken_Head2 = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, -2, 11, -8, 2, 13, -6);
	BoxDesc_TexOrigin(&desc, 14, 0);
	BoxDesc_RotOrigin(&desc, 0, 9, -4);
	Chicken_Head3 = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_RotatedBox(&desc, -3, 5, -4, 3, 11, 3);
	BoxDesc_TexOrigin(&desc, 0, 9);
	Chicken_Torso = BoxDesc_BuildRotatedBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, -4, 7, -3, -3, 11, 3);
	BoxDesc_TexOrigin(&desc, 24, 13);
	BoxDesc_RotOrigin(&desc, -3, 11, 0);
	Chicken_LeftWing = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, 3, 7, -3, 4, 11, 3);
	BoxDesc_TexOrigin(&desc, 24, 13);
	BoxDesc_RotOrigin(&desc, 3, 11, 0);
	Chicken_RightWing = BoxDesc_BuildBox(&ChickenModel, &desc);

	Chicken_LeftLeg = ChickenModel_MakeLeg(-3, 0, -2, -1);
	Chicken_RightLeg = ChickenModel_MakeLeg(0, 3, 1, 2);
}

Real32 ChickenModel_GetNameYOffset(void) { return 1.0125f; }
Real32 ChickenModel_GetEyeY(Entity* entity) { return 14.0f / 16.0f; }
Vector3 ChickenModel_GetCollisionSize(void) {
	return Vector3_Create3(8.0f / 16.0f, 12.0f / 16.0f, 8.0f / 16.0f);
}
void ChickenModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb, 
		-4.0f / 16.0f, 0.0f,         -8.0f / 16.0f,
		 4.0f / 16.0f, 15.0f / 16.0f, 4.0f / 16.0f);
}

void ChickenModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Chicken_Head, true);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Chicken_Head2, true);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Chicken_Head3, true);

	IModel_DrawPart(Chicken_Torso);
	IModel_DrawRotate(0, 0, -Math_AbsF(entity->Anim.LeftArmX), Chicken_LeftWing, false);
	IModel_DrawRotate(0, 0, Math_AbsF(entity->Anim.LeftArmX), Chicken_RightWing, false);

	PackedCol col = IModel_Cols[0];
	Int32 i;
	for (i = 0; i < Face_Count; i++) {
		IModel_Cols[i] = PackedCol_Scale(col, 0.7f);
	}

	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Chicken_LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Chicken_RightLeg, false);
	IModel_UpdateVB();
}

IModel* ChickenModel_GetInstance(void) {
	IModel_Init(&ChickenModel);
	IModel_SetPointers(ChickenModel);
	return &ChickenModel;
}


ModelPart Creeper_Head, Creeper_Torso, Creeper_LeftLegFront;
ModelPart Creeper_RightLegFront, Creeper_LeftLegBack, Creeper_RightLegBack;
ModelVertex CreeperModel_Vertices[IModel_BoxVertices * 6];
IModel CreeperModel;

void CreeperModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 18, -4, 4, 26, 4);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 18, 0);
	Creeper_Head = BoxDesc_BuildBox(&CreeperModel, &desc);

	BoxDesc_Box(&desc, -4, 6, -2, 4, 18, 2);
	BoxDesc_TexOrigin(&desc, 16, 16);
	Creeper_Torso = BoxDesc_BuildBox(&CreeperModel, &desc);

	BoxDesc_Box(&desc, -4, 0, -6, 0, 6, -2);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -2);
	Creeper_LeftLegFront = BoxDesc_BuildBox(&CreeperModel, &desc);

	BoxDesc_Box(&desc, 0, 0, -6, 4, 6, -2);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -2);
	Creeper_RightLegFront = BoxDesc_BuildBox(&CreeperModel, &desc);

	BoxDesc_Box(&desc, -4, 0, 2, 0, 6, 6);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 2);
	Creeper_LeftLegBack = BoxDesc_BuildBox(&CreeperModel, &desc);

	BoxDesc_Box(&desc, 0, 0, 2, 4, 6, 6);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 2);
	Creeper_RightLegBack = BoxDesc_BuildBox(&CreeperModel, &desc);
}

Real32 CreeperModel_GetNameYOffset(void) { return 1.7f; }
Real32 CreeperModel_GetEyeY(Entity* entity) { return 22.0f / 16.0f; }
Vector3 CreeperModel_GetCollisionSize(void) {
	return Vector3_Create3(8.0f / 16.0f, 26.0f / 16.0f, 8.0f / 16.0f);
}
void CreeperModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb, 
		-4.0f / 16.0f,          0.0f, -6.0f / 16.0f,
		 4.0f / 16.0f, 26.0f / 16.0f,  6.0f / 16.0f);
}

void CreeperModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Creeper_Head, true);

	IModel_DrawPart(Creeper_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, Creeper_LeftLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, Creeper_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, Creeper_LeftLegBack, false);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, Creeper_RightLegBack, false);
	IModel_UpdateVB();
}

IModel* CreeperModel_GetInstance(void) {
	IModel_Init(&CreeperModel);
	IModel_SetPointers(CreeperModel);
	CreeperModel.SurvivalScore = 200;
	return &CreeperModel;
}


ModelPart Pig_Head, Pig_Torso, Pig_LeftLegFront, Pig_RightLegFront;
ModelPart Pig_LeftLegBack, Pig_RightLegBack;
ModelVertex PigModel_Vertices[IModel_BoxVertices * 6];
IModel PigModel;

void PigModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 8, -14, 4, 16, -6);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 12, -6);
	Pig_Head = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_RotatedBox(&desc, -5, 6, -8, 5, 14, 8);
	BoxDesc_TexOrigin(&desc, 28, 8);
	Pig_Torso = BoxDesc_BuildRotatedBox(&PigModel, &desc);

	BoxDesc_Box(&desc, -5, 0, -7, -1, 6, -3);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -5);
	Pig_LeftLegFront = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_Box(&desc, 1, 0, -7, 5, 6, -3);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -5);
	Pig_RightLegFront = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_Box(&desc, -5, 0, 5, -1, 6, 9);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 7);
	Pig_LeftLegBack = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_Box(&desc, 1, 0, 5, 5, 6, 9);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 7);
	Pig_RightLegBack = BoxDesc_BuildBox(&PigModel, &desc);
}

Real32 PigModel_GetNameYOffset(void) { return 1.075f; }
Real32 PigModel_GetEyeY(Entity* entity) { return 12.0f / 16.0f; }
Vector3 PigModel_GetCollisionSize(void) {
	return Vector3_Create3(14.0f / 16.0f, 14.0f / 16.0f, 14.0f / 16.0f);
}

void PigModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb, 
		-5.0f / 16.0f,          0.0f, -14.0f / 16.0f,
		 5.0f / 16.0f, 16.0f / 16.0f,   9.0f / 16.0f);
}

void PigModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Pig_Head, true);

	IModel_DrawPart(Pig_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Pig_LeftLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Pig_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Pig_LeftLegBack, false);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Pig_RightLegBack, false);
	IModel_UpdateVB();
}

IModel* PigModel_GetInstance(void) {
	IModel_Init(&PigModel);
	IModel_SetPointers(PigModel);
	PigModel.SurvivalScore = 10;
	return &PigModel;
}


ModelPart Sheep_Head, Sheep_Torso, Sheep_LeftLegFront;
ModelPart Sheep_RightLegFront, Sheep_LeftLegBack, Sheep_RightLegBack;
ModelPart Fur_Head, Fur_Torso, Fur_LeftLegFront, Fur_RightLegFront;
ModelPart Fur_LeftLegBack, Fur_RightLegBack;
ModelVertex SheepModel_Vertices[IModel_BoxVertices * 6 * 2];
IModel SheepModel;
Int32 fur_Index;

void SheepModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -3, 16, -14, 3, 22, -6);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 18, -8);
	Sheep_Head = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_RotatedBox(&desc, -4, 12, -8, 4, 18, 8);
	BoxDesc_TexOrigin(&desc, 28, 8);
	Sheep_Torso = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_Box(&desc, -5, 0, -7, -1, 12, -3);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, -5);
	Sheep_LeftLegFront = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_Box(&desc, 1, 0, -7, 5, 12, -3);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, -5);
	Sheep_RightLegFront = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_Box(&desc, -5, 0, 5, -1, 12, 9);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 7);
	Sheep_LeftLegBack = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_Box(&desc, 1, 0, 5, 5, 12, 9);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 7);
	Sheep_RightLegBack = BoxDesc_BuildBox(&SheepModel, &desc);


	BoxDesc_Box(&desc, -3, -3, -3, 3, 3, 3);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_SetBounds(&desc, -3.5f, 15.5f, -12.5f, 3.5f, 22.5f, -5.5f);
	BoxDesc_RotOrigin(&desc, 0, 18, -8);
	Fur_Head = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_RotatedBox(&desc, -4, 12, -8, 4, 18, 8);
	BoxDesc_TexOrigin(&desc, 28, 8);
	BoxDesc_SetBounds(&desc, -6.0f, 10.5f, -10.0f, 6.0f, 19.5f, 10.0f);
	Fur_Torso = BoxDesc_BuildRotatedBox(&SheepModel, &desc);


	BoxDesc_Box(&desc, -2, -3, -2, 2, 3, 2);
	BoxDesc_TexOrigin(&desc, 0, 16);

	BoxDesc_SetBounds(&desc, -5.5f, 5.5f, -7.5f, -0.5f, 12.5f, -2.5f);
	BoxDesc_RotOrigin(&desc, 0, 12, -5);
	Fur_LeftLegFront = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_SetBounds(&desc, 0.5f, 5.5f, -7.5f, 5.5f, 12.5f, -2.5f);
	BoxDesc_RotOrigin(&desc, 0, 12, -5);
	Fur_RightLegFront = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_SetBounds(&desc, -5.5f, 5.5f, 4.5f, -0.5f, 12.5f, 9.5f);
	BoxDesc_RotOrigin(&desc, 0, 12, 7);
	Fur_LeftLegBack = BoxDesc_BuildBox(&SheepModel, &desc);

	BoxDesc_SetBounds(&desc, 0.5f, 5.5f, 4.5f, 5.5f, 12.5f, 9.5f);
	BoxDesc_RotOrigin(&desc, 0, 12, 7);
	Fur_RightLegBack = BoxDesc_BuildBox(&SheepModel, &desc);
}

Real32 SheepModel_GetNameYOffset(void) { return 1.48125f; }
Real32 SheepModel_GetEyeY(Entity* entity) { return 20.0f / 16.0f; }
Vector3 SheepModel_GetCollisionSize(void) {
	return Vector3_Create3(10.0f / 16.0f, 20.0f / 16.0f, 10.0f / 16.0f);
}
void SheepModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb,
		-6.0f / 16.0f,          0.0f, -13.0f / 16.0f, 
		6.0f  / 16.0f, 23.0f / 16.0f, 10.0f  / 16.0f);
}

void SheepModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Sheep_Head, true);

	IModel_DrawPart(Sheep_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Sheep_LeftLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Sheep_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Sheep_LeftLegBack, false);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Sheep_RightLegBack, false);
	IModel_UpdateVB();

	String sheep_nofur = String_FromConstant("sheep_nofur");
	if (String_CaselessEquals(&entity->ModelName, &sheep_nofur)) return;
	Gfx_BindTexture(ModelCache_Textures[fur_Index].TexID);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Fur_Head, true);

	IModel_DrawPart(Fur_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Fur_LeftLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Fur_RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, Fur_LeftLegBack, false);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, Fur_RightLegBack, false);
	IModel_UpdateVB();
}

IModel* SheepModel_GetInstance(void) {
	IModel_Init(&SheepModel);
	IModel_SetPointers(SheepModel);
	SheepModel.SurvivalScore = 10;

	String sheep_fur = String_FromConstant("sheep_fur.png");
	fur_Index = ModelCache_GetTextureIndex(&sheep_fur);
	return &SheepModel;
}


ModelPart Skeleton_Head, Skeleton_Torso, Skeleton_LeftLeg;
ModelPart Skeleton_RightLeg, Skeleton_LeftArm, Skeleton_RightArm;
ModelVertex SkeletonModel_Vertices[IModel_BoxVertices * 6];
IModel SkeletonModel;

void SkeletonModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 24, -4, 4, 32, 4);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 24, 0);
	Skeleton_Head = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, -4, 12, -2, 4, 24, 2);
	BoxDesc_TexOrigin(&desc, 16, 16);
	Skeleton_Torso = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, -1, 0, -1, -3, 12, 1);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 0);
	Skeleton_LeftLeg = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, 1, 0, -1, 3, 12, 1);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 0);
	Skeleton_RightLeg = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, -4, 12, -1, -6, 24, 1);
	BoxDesc_TexOrigin(&desc, 40, 16);
	BoxDesc_RotOrigin(&desc, -5, 23, 0);
	Skeleton_LeftArm = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, 4, 12, -1, 6, 24, 1);
	BoxDesc_TexOrigin(&desc, 40, 16);
	BoxDesc_RotOrigin(&desc, 5, 23, 0);
	Skeleton_RightArm = BoxDesc_BuildBox(&SkeletonModel, &desc);
}

Real32 SkeletonModel_GetNameYOffset(void) { return 2.075f; }
Real32 SkeletonModel_GetEyeY(Entity* entity) { return 26.0f / 16.0f; }
Vector3 SkeletonModel_GetCollisionSize(void) {
	return Vector3_Create3(8.0f / 16.0f, 30.0f / 16.0f, 8.0f / 16.0f);
}

void SkeletonModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb, 
		-4.0f / 16.0f,          0.0f, -4.0f / 16.0f,
		 4.0f / 16.0f, 32.0f / 16.0f,  4.0f / 16.0f);
}

void SkeletonModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Skeleton_Head, true);

	IModel_DrawPart(Skeleton_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, Skeleton_LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, Skeleton_RightLeg, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD, 0.0f, entity->Anim.LeftArmZ, Skeleton_LeftArm, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD, 0.0f, entity->Anim.RightArmZ, Skeleton_RightArm, false);
	IModel_UpdateVB();
}

IModel* SkeletonModel_GetInstance(void) {
	IModel_Init(&SkeletonModel);
	IModel_SetPointers(SkeletonModel);
	SkeletonModel.SurvivalScore = 120;
	return &SkeletonModel;
}


ModelPart Spider_Head, Spider_Link, Spider_End;
ModelPart Spider_LeftLeg, Spider_RightLeg;
ModelVertex SpiderModel_Vertices[IModel_BoxVertices * 5];
IModel SpiderModel;

void SpiderModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 4, -11, 4, 12, -3);
	BoxDesc_TexOrigin(&desc, 32, 4);
	BoxDesc_RotOrigin(&desc, 0, 8, -3);
	Spider_Head = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, -3, 5, 3, 3, 11, -3);
	BoxDesc_TexOrigin(&desc, 0, 0);
	Spider_Link = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, -5, 4, 3, 5, 12, 15);
	BoxDesc_TexOrigin(&desc, 0, 12);
	Spider_End = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, -19, 7, -1, -3, 9, 1);
	BoxDesc_TexOrigin(&desc, 18, 0);
	BoxDesc_RotOrigin(&desc, -3, 8, 0);
	Spider_LeftLeg = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, 3, 7, -1, 19, 9, 1);
	BoxDesc_TexOrigin(&desc, 18, 0);
	BoxDesc_RotOrigin(&desc, 3, 8, 0);
	Spider_RightLeg = BoxDesc_BuildBox(&SpiderModel, &desc);
}

Real32 SpiderModel_GetNameYOffset(void) { return 1.0125f; }
Real32 SpiderModel_GetEyeY(Entity* entity) { return 8.0f / 16.0f; }
Vector3 SpiderModel_GetCollisionSize(void) {
	return Vector3_Create3(15.0f / 16.0f, 12.0f / 16.0f, 15.0f / 16.0f);
}

void SpiderModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb, 
		-5.0f / 16.0f, 0.0f,         -11.0f / 16.0f,
		 5.0f / 16.0f, 12.0f / 16.0f, 15.0f / 16.0f);
}

#define quarterPi (MATH_PI / 4.0f)
#define eighthPi  (MATH_PI / 8.0f)

void SpiderModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Spider_Head, true);
	IModel_DrawPart(Spider_Link);
	IModel_DrawPart(Spider_End);

	Real32 rotX = Math_Sin(entity->Anim.WalkTime)     * entity->Anim.Swing * MATH_PI;
	Real32 rotZ = Math_Cos(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 16.0f;
	Real32 rotY = Math_Sin(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 32.0f;
	IModel_Rotation = RotateOrder_XZY;

	IModel_DrawRotate(rotX, quarterPi + rotY, eighthPi + rotZ, Spider_LeftLeg, false);
	IModel_DrawRotate(-rotX, eighthPi + rotY, eighthPi + rotZ, Spider_LeftLeg, false);
	IModel_DrawRotate(rotX, -eighthPi - rotY, eighthPi - rotZ, Spider_LeftLeg, false);
	IModel_DrawRotate(-rotX, -quarterPi - rotY, eighthPi - rotZ, Spider_LeftLeg, false);

	IModel_DrawRotate(rotX, -quarterPi + rotY, -eighthPi + rotZ, Spider_RightLeg, false);
	IModel_DrawRotate(-rotX, -eighthPi + rotY, -eighthPi + rotZ, Spider_RightLeg, false);
	IModel_DrawRotate(rotX, eighthPi - rotY, -eighthPi - rotZ, Spider_RightLeg, false);
	IModel_DrawRotate(-rotX, quarterPi - rotY, -eighthPi - rotZ, Spider_RightLeg, false);

	IModel_Rotation = RotateOrder_ZYX;
	IModel_UpdateVB();
}

IModel* SpiderModel_GetInstance(void) {
	IModel_Init(&SpiderModel);
	IModel_SetPointers(SpiderModel);
	SpiderModel.SurvivalScore = 105;
	return &SpiderModel;
}


ModelPart Zombie_Head, Zombie_Hat, Zombie_Torso, Zombie_LeftLeg;
ModelPart Zombie_RightLeg, Zombie_LeftArm, Zombie_RightArm;
ModelVertex ZombieModel_Vertices[IModel_BoxVertices * 7];
IModel ZombieModel;

void ZombieModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 24, -4, 4, 32, 4);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 24, 0);
	Zombie_Head = BoxDesc_BuildBox(&ZombieModel, &desc);

	BoxDesc_Box(&desc, -4, 24, -4, 4, 32, 4);
	BoxDesc_TexOrigin(&desc, 32, 0);
	BoxDesc_RotOrigin(&desc, 0, 24, 0);
	BoxDesc_Expand(&desc, 0.5f);
	Zombie_Hat = BoxDesc_BuildBox(&ZombieModel, &desc);

	BoxDesc_Box(&desc, -4, 12, -2, 4, 24, 2);
	BoxDesc_TexOrigin(&desc, 16, 16);
	Zombie_Torso = BoxDesc_BuildBox(&ZombieModel, &desc);

	BoxDesc_Box(&desc, 0, 0, -2, -4, 12, 2);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 0);
	Zombie_LeftLeg = BoxDesc_BuildBox(&ZombieModel, &desc);

	BoxDesc_Box(&desc, 0, 0, -2, 4, 12, 2);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 0);
	Zombie_RightLeg = BoxDesc_BuildBox(&ZombieModel, &desc);

	BoxDesc_Box(&desc, -4, 12, -2, -8, 24, 2);
	BoxDesc_TexOrigin(&desc, 40, 16);
	BoxDesc_RotOrigin(&desc, -6, 22, 0);
	Zombie_LeftArm = BoxDesc_BuildBox(&ZombieModel, &desc);

	BoxDesc_Box(&desc, 4, 12, -2, 8, 24, 2);
	BoxDesc_TexOrigin(&desc, 40, 16);
	BoxDesc_RotOrigin(&desc, 6, 22, 0);
	Zombie_RightArm = BoxDesc_BuildBox(&ZombieModel, &desc);
}

Real32 ZombieModel_GetNameYOffset(void) { return 2.075f; }
Real32 ZombieModel_GetEyeY(Entity* entity) { return 26.0f / 16.0f; }
Vector3 ZombieModel_GetCollisionSize(void) {
	return Vector3_Create3((8.0f + 0.6f) / 16.0f, 28.1f / 16.0f, (8.0f + 0.6f) / 16.0f);
}

void ZombieModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb, 
		-4.0f / 16.0f,          0.0f, -4.0f / 16.0f,
		 4.0f / 16.0f, 32.0f / 16.0f,  4.0f / 16.0f);
}

void ZombieModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Zombie_Head, true);

	IModel_DrawPart(Zombie_Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, Zombie_LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, Zombie_RightLeg, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD, 0.0f, entity->Anim.LeftArmZ, Zombie_LeftArm, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD, 0.0f, entity->Anim.RightArmZ, Zombie_RightArm, false);

	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Zombie_Hat, true);
	IModel_UpdateVB();
}

IModel* ZombieModel_GetInstance(void) {
	IModel_Init(&ZombieModel);
	IModel_SetPointers(ZombieModel);
	ZombieModel.SurvivalScore = 80;
	return &ZombieModel;
}


typedef struct ModelSet_ {
	ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat,
		TorsoLayer, LeftLegLayer, RightLegLayer, LeftArmLayer, RightArmLayer;
} ModelSet;

BoxDesc head, torso, lLeg, rLeg, lArm, rArm;
Real32 offset;
void HumanModel_CreateParts(IModel* m, ModelSet* set, ModelSet* set64, ModelSet* setSlim) {
	BoxDesc_TexOrigin(&head, 0, 0);
	set->Head = BoxDesc_BuildBox(m, &head);

	BoxDesc_TexOrigin(&torso, 16, 16);
	set->Torso = BoxDesc_BuildBox(m, &torso);

	BoxDesc_TexOrigin(&head, 32, 0);
	BoxDesc_Expand(&head, offset);
	set->Hat = BoxDesc_BuildBox(m, &head);

	BoxDesc_TexOrigin(&lLeg, 0, 16);
	BoxDesc_MirrorX(&lLeg);
	set->LeftLeg = BoxDesc_BuildBox(m, &lLeg);

	BoxDesc_TexOrigin(&rLeg, 0, 16);
	set->RightLeg = BoxDesc_BuildBox(m, &rLeg);
	
	BoxDesc_TexOrigin(&lArm, 40, 16);
	BoxDesc_MirrorX(&lArm);
	set->LeftArm = BoxDesc_BuildBox(m, &lArm);

	BoxDesc_TexOrigin(&rArm, 40, 16);
	set->RightArm = BoxDesc_BuildBox(m, &rArm);
	
	set64->Head = set->Head;
	set64->Torso = set->Torso;
	set64->Hat = set->Hat;

	BoxDesc_MirrorX(&lLeg);
	BoxDesc_TexOrigin(&lLeg, 16, 48);
	set64->LeftLeg = BoxDesc_BuildBox(m, &lLeg);
	set64->RightLeg = set->RightLeg;

	BoxDesc_MirrorX(&lArm);
	BoxDesc_TexOrigin(&lArm, 32, 48);
	set64->LeftArm = BoxDesc_BuildBox(m, &lArm);
	set64->RightArm = set->RightArm;

	BoxDesc_TexOrigin(&torso, 16, 32);
	BoxDesc_Expand(&torso, offset);
	set64->TorsoLayer = BoxDesc_BuildBox(m, &torso);

	BoxDesc_TexOrigin(&lLeg, 0, 48);
	BoxDesc_Expand(&lLeg, offset);
	set64->LeftLegLayer = BoxDesc_BuildBox(m, &lLeg);

	BoxDesc_TexOrigin(&rLeg, 0, 32);
	BoxDesc_Expand(&rLeg, offset);
	set64->RightLegLayer = BoxDesc_BuildBox(m, &rLeg);

	BoxDesc_TexOrigin(&lArm, 48, 48);
	BoxDesc_Expand(&lArm, offset);
	set64->LeftArmLayer = BoxDesc_BuildBox(m, &lArm);

	BoxDesc_TexOrigin(&rArm, 40, 32);
	BoxDesc_Expand(&rArm, offset);
	set64->RightArmLayer = BoxDesc_BuildBox(m, &rArm);

	setSlim->Head = set64->Head;
	setSlim->Torso = set64->Torso;
	setSlim->Hat = set64->Hat;
	setSlim->LeftLeg = set64->LeftLeg;
	setSlim->RightLeg = set64->RightLeg;	

	lArm.BodyW -= 1; lArm.X1 += (offset * 2.0f) / 16.0f;
	BoxDesc_TexOrigin(&lArm, 32, 48);
	setSlim->LeftArm = BoxDesc_BuildBox(m, &lArm);

	rArm.BodyW -= 1; rArm.X2 -= (offset * 2.0f) / 16.0f;
	BoxDesc_TexOrigin(&rArm, 40, 16);
	setSlim->RightArm = BoxDesc_BuildBox(m, &rArm);

	setSlim->TorsoLayer = set64->TorsoLayer;
	setSlim->LeftLegLayer = set64->LeftLegLayer;
	setSlim->RightLegLayer = set64->RightLegLayer;

	BoxDesc_TexOrigin(&lArm, 48, 48);
	BoxDesc_Expand(&lArm, offset);
	setSlim->LeftArmLayer = BoxDesc_BuildBox(m, &lArm);

	BoxDesc_TexOrigin(&rArm, 40, 32);
	BoxDesc_Expand(&rArm, offset);
	setSlim->RightArmLayer = BoxDesc_BuildBox(m, &rArm);
}

void HumanModel_SetupState(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	Gfx_SetAlphaTest(false);

	bool _64x64 = entity->SkinType != SkinType_64x32;
	IModel_uScale = entity->uScale / 64.0f;
	IModel_vScale = entity->vScale / (_64x64 ? 64.0f : 32.0f);
}

void HumanModel_DrawModel(Entity* p, ModelSet* model) {
	SkinType skinType = p->SkinType;
	IModel_DrawRotate(-p->HeadX * MATH_DEG2RAD, 0, 0, model->Head, true);
	IModel_DrawPart(model->Torso);
	IModel_DrawRotate(p->Anim.LeftLegX, 0, p->Anim.LeftLegZ, model->LeftLeg, false);
	IModel_DrawRotate(p->Anim.RightLegX, 0, p->Anim.RightLegZ, model->RightLeg, false);

	IModel_Rotation = RotateOrder_XZY;
	IModel_DrawRotate(p->Anim.LeftArmX, 0, p->Anim.LeftArmZ, model->LeftArm, false);
	IModel_DrawRotate(p->Anim.RightArmX, 0, p->Anim.RightArmZ, model->RightArm, false);
	IModel_Rotation = RotateOrder_ZYX;
	IModel_UpdateVB();

	Gfx_SetAlphaTest(true);
	IModel_ActiveModel->index = 0;
	if (skinType != SkinType_64x32) {
		IModel_DrawPart(model->TorsoLayer);
		IModel_DrawRotate(p->Anim.LeftLegX, 0, p->Anim.LeftLegZ, model->LeftLegLayer, false);
		IModel_DrawRotate(p->Anim.RightLegX, 0, p->Anim.RightLegZ, model->RightLegLayer, false);

		IModel_Rotation = RotateOrder_XZY;
		IModel_DrawRotate(p->Anim.LeftArmX, 0, p->Anim.LeftArmZ, model->LeftArmLayer, false);
		IModel_DrawRotate(p->Anim.RightArmX, 0, p->Anim.RightArmZ, model->RightArmLayer, false);
		IModel_Rotation = RotateOrder_ZYX;
	}
	IModel_DrawRotate(-p->HeadX * MATH_DEG2RAD, 0, 0, model->Hat, true);
	IModel_UpdateVB();
}


ModelSet Humanoid_Set, Humanoid_Set64, Humanoid_SetSlim;
ModelVertex HumanoidModel_Vertices[IModel_BoxVertices * (7 + 7 + 4)];
IModel HumanoidModel;

void HumanoidModel_MakeBoxDescs(void) {
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

void HumanoidModel_CreateParts(void) {
	HumanoidModel_MakeBoxDescs();
	offset = 0.5f;
	HumanModel_CreateParts(&HumanoidModel, &Humanoid_Set, 
		&Humanoid_Set64, &Humanoid_SetSlim);
}

Real32 HumanoidModel_GetNameYOffset(void) { return 32.0f / 16.0f + 0.5f / 16.0f; }
Real32 HumanoidModel_GetEyeY(Entity* entity) { return 26.0f / 16.0f; }
Vector3 HumanoidModel_GetCollisionSize(void) {
	return Vector3_Create3((8.0f + 0.6f) / 16.0f, 28.1f / 16.0f, (8.0f + 0.6f) / 16.0f);
}
void HumanoidModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb,
		-8.0f / 16.0f, 0.0f,         -4.0f / 16.0f,
		 8.0f / 16.0f, 32.0f / 16.0f, 4.0f / 16.0f);
}

void HumanoidModel_DrawModel(Entity* entity) {
	HumanModel_SetupState(entity);
	SkinType skinType = entity->SkinType;
	ModelSet* model =
		skinType == SkinType_64x64Slim ? &Humanoid_SetSlim :
		(skinType == SkinType_64x64 ? &Humanoid_Set64 : &Humanoid_Set);
	HumanModel_DrawModel(entity, model);
}

IModel* HumanoidModel_GetInstance(void) {
	IModel_Init(&HumanoidModel);
	IModel_SetPointers(HumanoidModel);
	HumanoidModel.CalcHumanAnims = true;
	HumanoidModel.UsesHumanSkin = true;
	return &HumanoidModel;
}


ModelSet Chibi_Set, Chibi_Set64, Chibi_SetSlim;
ModelVertex ChibiModel_Vertices[IModel_BoxVertices * (7 + 7 + 4)];
IModel ChibiModel;
#define chibi_size 0.5f

void ChibiModel_MakeBoxDescs(void) {
	HumanoidModel_MakeBoxDescs();
	BoxDesc_Box(&head, -4, 12, -4, 4, 20, 4);
	BoxDesc_RotOrigin(&head, 0, 13, 0);

	BoxDesc_Scale(&torso, chibi_size);
	BoxDesc_Scale(&lLeg, chibi_size);
	BoxDesc_Scale(&rLeg, chibi_size);
	BoxDesc_Scale(&lArm, chibi_size);
	BoxDesc_Scale(&rArm, chibi_size);
}

void ChibiModel_CreateParts(void) {
	ChibiModel_MakeBoxDescs();
	offset = 0.5f * chibi_size;
	HumanModel_CreateParts(&ChibiModel, &Chibi_Set,
		&Chibi_Set64, &Chibi_SetSlim);
}

Real32 ChibiModel_GetNameYOffset(void) { return 20.2f / 16.0f; }
Real32 ChibiModel_GetEyeY(Entity* entity) { return 14.0f / 16.0f; }
Vector3 ChibiModel_GetCollisionSize(void) {
	return Vector3_Create3((4.0f + 0.6f) / 16.0f, 20.1f / 16.0f, (4.0f + 0.6f) / 16.0f);
}
void ChibiModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb,
		-4.0f / 16.0f,          0.0f, -4.0f / 16.0f, 
		 4.0f / 16.0f, 16.0f / 16.0f,  4.0f / 16.0f);
}

void ChibiModel_DrawModel(Entity* entity) {
	HumanModel_SetupState(entity);
	SkinType skinType = entity->SkinType;
	ModelSet* model =
		skinType == SkinType_64x64Slim ? &Chibi_SetSlim :
		(skinType == SkinType_64x64 ? &Chibi_Set64 : &Chibi_Set);
	HumanModel_DrawModel(entity, model);
}

IModel* ChibiModel_GetInstance(void) {
	IModel_Init(&ChibiModel);
	IModel_SetPointers(ChibiModel);
	ChibiModel.CalcHumanAnims = true;
	ChibiModel.UsesHumanSkin = true;
	ChibiModel.MaxScale = 3.0f;
	ChibiModel.ShadowScale = 0.5f;
	return &ChibiModel;
}


IModel SittingModel;
#define SIT_OFFSET 10.0f
void SittingModel_CreateParts(void) { }

Real32 SittingModel_GetNameYOffset(void) { return (32.0f + 0.5f) / 16.0f; }
Real32 SittingModel_GetEyeY(Entity* entity) { return (26.0f - SIT_OFFSET) / 16.0f; }
Vector3 SittingModel_GetCollisionSize(void) {
	return Vector3_Create3((8.0f + 0.6f) / 16.0f, (28.1f - SIT_OFFSET) / 16.0f, (8.0f + 0.6f) / 16.0f);
}
void SittingModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb,
		-8.0f / 16.0f,                         0.0f, -4.0f / 16.0f,
		 8.0f / 16.0f, (32.0f - SIT_OFFSET) / 16.0f,  4.0f / 16.0f);
}

void SittingModel_GetTransform(Matrix* m, Entity* p, Vector3 pos) {
	pos.Y -= (SIT_OFFSET / 16.0f) * p->ModelScale.Y;
	Entity_GetTransform(p, pos, p->ModelScale, m);
}

void SittingModel_DrawModel(Entity* p) {
	p->Anim.LeftLegX = 1.5f;  p->Anim.RightLegX = 1.5f;
	p->Anim.LeftLegZ = -0.1f; p->Anim.RightLegZ = 0.1f;
	IModel_SetupState(&HumanoidModel, p);
	IModel_Render(&HumanoidModel, p);
}

IModel* SittingModel_GetInstance(void) {
	IModel_Init(&SittingModel);
	IModel_SetPointers(SittingModel);
	SittingModel.CalcHumanAnims = true;
	SittingModel.UsesHumanSkin = true;
	SittingModel.ShadowScale = 0.5f;
	return &SittingModel;
}


IModel HeadModel;
void HeadModel_CreateParts(void) { }

Real32 HeadModel_GetNameYOffset(void) { return (32.0f + 0.5f) / 16.0f; }
Real32 HeadModel_GetEyeY(Entity* entity) { return 6.0f / 16.0f; }
Vector3 HeadModel_GetCollisionSize(void) {
	return Vector3_Create3(7.9f / 16.0f, 7.9f / 16.0f, 7.9f / 16.0f);
}
void HeadModel_GetPickingBounds(AABB* bb) {
	AABB_FromCoords6(bb,
		-4.0f / 16.0f,         0.0f, -4.0f / 16.0f,
		 4.0f / 16.0f, 8.0f / 16.0f,  4.0f / 16.0f);
}

void HeadModel_GetTransform(Matrix* m, Entity* p, Vector3 pos) {
	pos.Y -= (24.0f / 16.0f) * p->ModelScale.Y;
	Entity_GetTransform(p, pos, p->ModelScale, m);
}

void HeadModel_DrawModel(Entity* p) {
	HumanModel_SetupState(p);

	ModelPart part = Humanoid_Set.Head; part.RotY += 4.0f / 16.0f;
	IModel_DrawRotate(-p->HeadX * MATH_DEG2RAD, 0, 0, part, true);
	IModel_UpdateVB();

	Gfx_SetAlphaTest(true);
	IModel_ActiveModel->index = 0;
	part = Humanoid_Set.Hat; part.RotY += 4.0f / 16.0f;
	IModel_DrawRotate(-p->HeadX * MATH_DEG2RAD, 0, 0, part, true);
	IModel_UpdateVB();
}

IModel* HeadModel_GetInstance(void) {
	IModel_Init(&HeadModel);
	IModel_SetPointers(HeadModel);
	HeadModel.CalcHumanAnims = true;
	HeadModel.UsesHumanSkin = true;
	return &HeadModel;
}