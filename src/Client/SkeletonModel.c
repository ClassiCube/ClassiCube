#include "IModel.h"
#include "ModelBuilder.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"

ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm;
ModelVertex SkeletonModel_Vertices[IModel_BoxVertices * 6];
IModel SkeletonModel;

void SkeletonModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 24, -4, 4, 32, 4);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 24, 0);
	Head = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, -4, 12, -2, 4, 24, 2);
	BoxDesc_TexOrigin(&desc, 16, 16);
	Torso = BoxDesc_BuildBox(&SkeletonModel, &desc);
		
	BoxDesc_Box(&desc, -1, 0, -1, -3, 12, 1);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 0);
	LeftLeg = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, 1, 0, -1, 3, 12, 1);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 12, 0);
	RightLeg = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, -4, 12, -1, -6, 24, 1);
	BoxDesc_TexOrigin(&desc, 40, 16);
	BoxDesc_RotOrigin(&desc, -5, 23, 0);
	LeftArm = BoxDesc_BuildBox(&SkeletonModel, &desc);

	BoxDesc_Box(&desc, 4, 12, -1, 6, 24, 1);
	BoxDesc_TexOrigin(&desc, 40, 16);
	BoxDesc_RotOrigin(&desc, 5, 23, 0);
	RightArm = BoxDesc_BuildBox(&SkeletonModel, &desc);
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
	Gfx_BindTexture(IModel_GetTexture(&SkeletonModel, entity->MobTextureId));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Head, true);

	IModel_DrawPart(Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, RightLeg, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD, 0.0f, entity->Anim.LeftArmZ, LeftArm, false);
	IModel_DrawRotate(90.0f * MATH_DEG2RAD, 0.0f, entity->Anim.RightArmZ, RightArm, false);
	IModel_UpdateVB();
}

IModel* SkeletonModel_GetInstance(void) {
	IModel_Init(&SkeletonModel);
	IModel_SetPointers(SkeletonModel);
	return &SkeletonModel;
}