#include "IModel.h"
#include "ModelBuilder.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"

ModelPart Head, Link, End, LeftLeg, RightLeg;
ModelVertex SpiderModel_Vertices[IModel_BoxVertices * 5];
IModel SpiderModel;

void SpiderModel_CreateParts(void) {
	BoxDesc desc;

	BoxDesc_Box(&desc, -4, 4, -11, 4, 12, -3);
	BoxDesc_TexOrigin(&desc, 32, 4);
	BoxDesc_RotOrigin(&desc, 0, 8, -3);
	Head = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, -3, 5, 3, 3, 11, -3);
	BoxDesc_TexOrigin(&desc, 0, 0);
	Link = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, -5, 4, 3, 5, 12, 15);
	BoxDesc_TexOrigin(&desc, 0, 12);
	End = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, -19, 7, -1, -3, 9, 1);
	BoxDesc_TexOrigin(&desc, 18, 0);
	BoxDesc_RotOrigin(&desc, -3, 8, 0);
	LeftLeg = BoxDesc_BuildBox(&SpiderModel, &desc);

	BoxDesc_Box(&desc, 3, 7, -1, 19, 9, 1);
	BoxDesc_TexOrigin(&desc, 18, 0);
	BoxDesc_RotOrigin(&desc, 3, 8, 0);
	RightLeg = BoxDesc_BuildBox(&SpiderModel, &desc);
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
	Gfx_BindTexture(IModel_GetTexture(&SpiderModel, entity->MobTextureId));
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Head, true);
	IModel_DrawPart(Link);
	IModel_DrawPart(End);

	Real32 rotX = Math_Sin(entity->Anim.WalkTime)     * entity->Anim.Swing * MATH_PI;
	Real32 rotZ = Math_Cos(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 16.0f;
	Real32 rotY = Math_Sin(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 32.0f;
	IModel_ActiveModel->Rotation = RotateOrder_XZY;

	IModel_DrawRotate(rotX, quarterPi + rotY, eighthPi + rotZ, LeftLeg, false);
	IModel_DrawRotate(-rotX, eighthPi + rotY, eighthPi + rotZ, LeftLeg, false);
	IModel_DrawRotate(rotX, -eighthPi - rotY, eighthPi - rotZ, LeftLeg, false);
	IModel_DrawRotate(-rotX, -quarterPi - rotY, eighthPi - rotZ, LeftLeg, false);

	IModel_DrawRotate(rotX, -quarterPi + rotY, -eighthPi + rotZ, RightLeg, false);
	IModel_DrawRotate(-rotX, -eighthPi + rotY, -eighthPi + rotZ, RightLeg, false);
	IModel_DrawRotate(rotX, eighthPi - rotY, -eighthPi - rotZ, RightLeg, false);
	IModel_DrawRotate(-rotX, quarterPi - rotY, -eighthPi - rotZ, RightLeg, false);

	IModel_ActiveModel->Rotation = RotateOrder_ZYX;
	IModel_UpdateVB();
}

IModel* SpiderModel_GetInstance(void) {
	IModel_Init(&SpiderModel);
	IModel_SetPointers(SpiderModel);
	return &SpiderModel;
}