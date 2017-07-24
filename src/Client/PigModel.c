#include "IModel.h"
#include "ModelBuilder.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"

ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
ModelVertex PigModel_Vertices[IModel_BoxVertices * 6];
IModel PigModel;

void PigModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 8, -14, 4, 16, -6);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 12, -6);
	Head = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_RotatedBox(&desc, -5, 6, -8, 5, 14, 8);
	BoxDesc_TexOrigin(&desc, 28, 8);
	Torso = BoxDesc_BuildRotatedBox(&PigModel, &desc);

	BoxDesc_Box(&desc, -5, 0, -7, -1, 6, -3);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -5);
	LeftLegFront = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_Box(&desc, 1, 0, -7, 5, 6, -3);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -5);
	RightLegFront = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_Box(&desc, -5, 0, 5, -1, 6, 9);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 7);
	LeftLegBack = BoxDesc_BuildBox(&PigModel, &desc);

	BoxDesc_Box(&desc, 1, 0, 5, 5, 6, 9);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 7);
	RightLegBack = BoxDesc_BuildBox(&PigModel, &desc);
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
	Gfx_BindTexture(IModel_GetTexture(&PigModel, entity->MobTextureId));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Head, true);

	IModel_DrawPart(Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, LeftLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, LeftLegBack, false);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0, 0, RightLegBack, false);
	IModel_UpdateVB();
}

IModel* PigModel_GetInstance(void) {
	IModel_Init(&PigModel);
	IModel_SetPointers(PigModel);
	return &PigModel;
}