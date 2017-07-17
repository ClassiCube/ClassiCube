#include "IModel.h"
#include "ModelBuilder.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"

ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
ModelVertex CreeperModel_Vertices[IModel_BoxVertices * 6];
IModel CreeperModel;

void CreeperModel_CreateParts(void) {
	BoxDesc desc = BoxDesc_Box(-4, 18, -4, 4, 26, 4);
	BoxDesc_TexOrigin(&desc, 0, 0);
	BoxDesc_RotOrigin(&desc, 0, 18, 0);
	Head = BoxDesc_BuildBox(&CreeperModel, &desc);

	desc = BoxDesc_Box(-4, 6, -2, 4, 18, 2);
	BoxDesc_TexOrigin(&desc, 16, 16);
	Torso = BoxDesc_BuildBox(&CreeperModel, &desc);

	desc = BoxDesc_Box(-4, 0, -6, 0, 6, -2);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -2);
	LeftLegFront = BoxDesc_BuildBox(&CreeperModel, &desc);

	desc = BoxDesc_Box(0, 0, -6, 4, 6, -2);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, -2);
	RightLegFront = BoxDesc_BuildBox(&CreeperModel, &desc);

	desc = BoxDesc_Box(-4, 0, 2, 0, 6, 6);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 2);
	LeftLegBack = BoxDesc_BuildBox(&CreeperModel, &desc);

	desc = BoxDesc_Box(0, 0, 2, 4, 6, 6);
	BoxDesc_TexOrigin(&desc, 0, 16);
	BoxDesc_RotOrigin(&desc, 0, 6, 2);
	RightLegBack = BoxDesc_BuildBox(&CreeperModel, &desc);
}

Real32 CreeperModel_GetNameYOffset(void) { return 1.7f; }
Real32 CreeperModel_GetEyeY(Entity* entity) { return 22.0f / 16.0f; }
Vector3 CreeperModel_GetCollisionSize(void) { return Vector3_Create3(8.0f / 16.0f, 26.0f / 16.0f, 8.0f / 16.0f); }

AABB CreeperModel_GetPickingBounds(void) {
	AABB bb;
	AABB_FromCoords6(&bb, -4.0f / 16.0f, 0.0f, -6.0f / 16.0f, 4.0f / 16.0f, 26.0f / 16.0f, 6.0f / 16.0f);
	return bb;
}

void CreeperModel_DrawModel(Entity* entity) {
	Gfx_BindTexture(IModel_GetTexture(&CreeperModel, entity->MobTextureId));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0.0f, 0.0f, Head, true);

	IModel_DrawPart(Torso);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, LeftLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, RightLegFront, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0.0f, 0.0f, LeftLegBack, false);
	IModel_DrawRotate(entity->Anim.LeftLegX, 0.0f, 0.0f, RightLegBack, false);
	IModel_UpdateVB();
}

IModel* CreeperModel_GetInstance(void) {
	IModel_Init(&CreeperModel);
	IModel_SetFuncPointers(CreeperModel);
	return &CreeperModel;
}
