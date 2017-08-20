#if 0
#include "IModel.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"

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

AABB ChickenModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -4.0f / 16.0f, 0.0f, -8.0f / 16.0f,
		4.0f / 16.0f, 15.0f / 16.0f, 4.0f / 16.0f);
	return bb;
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

AABB CreeperModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -4.0f / 16.0f, 0.0f, -6.0f / 16.0f,
		4.0f / 16.0f, 26.0f / 16.0f, 6.0f / 16.0f);
	return bb;
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

AABB PigModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -5.0f / 16.0f, 0.0f, -14.0f / 16.0f,
		5.0f / 16.0f, 16.0f / 16.0f, 9.0f / 16.0f);
	return bb;
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
	return &PigModel;
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

AABB SkeletonModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -4.0f / 16.0f, 0.0f, -4.0f / 16.0f,
		4.0f / 16.0f, 32.0f / 16.0f, 4.0f / 16.0f);
	return bb;
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

AABB SpiderModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -5.0f / 16.0f, 0.0f, -11.0f / 16.0f,
		5.0f / 16.0f, 12.0f / 16.0f, 15.0f / 16.0f);
	return bb;
}

#define quarterPi (MATH_PI / 4.0f)
#define eighthPi  (MATH_PI / 8.0f)

void SpiderModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(entity));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Head, true);
	IModel_DrawPart(Spider_Link);
	IModel_DrawPart(Spider_End);

	Real32 rotX = Math_Sin(entity->Anim.WalkTime)     * entity->Anim.Swing * MATH_PI;
	Real32 rotZ = Math_Cos(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 16.0f;
	Real32 rotY = Math_Sin(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 32.0f;
	IModel_ActiveModel->Rotation = RotateOrder_XZY;

	IModel_DrawRotate(rotX, quarterPi + rotY, eighthPi + rotZ, Spider_LeftLeg, false);
	IModel_DrawRotate(-rotX, eighthPi + rotY, eighthPi + rotZ, Spider_LeftLeg, false);
	IModel_DrawRotate(rotX, -eighthPi - rotY, eighthPi - rotZ, Spider_LeftLeg, false);
	IModel_DrawRotate(-rotX, -quarterPi - rotY, eighthPi - rotZ, Spider_LeftLeg, false);

	IModel_DrawRotate(rotX, -quarterPi + rotY, -eighthPi + rotZ, Spider_RightLeg, false);
	IModel_DrawRotate(-rotX, -eighthPi + rotY, -eighthPi + rotZ, Spider_RightLeg, false);
	IModel_DrawRotate(rotX, eighthPi - rotY, -eighthPi - rotZ, Spider_RightLeg, false);
	IModel_DrawRotate(-rotX, quarterPi - rotY, -eighthPi - rotZ, Spider_RightLeg, false);

	IModel_ActiveModel->Rotation = RotateOrder_ZYX;
	IModel_UpdateVB();
}

IModel* SpiderModel_GetInstance(void) {
	IModel_Init(&SpiderModel);
	IModel_SetPointers(SpiderModel);
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

AABB ZombieModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -4.0f / 16.0f, 0.0f, -4.0f / 16.0f,
		4.0f / 16.0f, 32.0f / 16.0f, 4.0f / 16.0f);
	return bb;
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
	return &ZombieModel;
}
\
#endif