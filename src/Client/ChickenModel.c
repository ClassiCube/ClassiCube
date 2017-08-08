#if 0
#include "IModel.h"
#include "ModelBuilder.h"
#include "GraphicsAPI.h"
#include "ExtMath.h"

ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
ModelVertex ChickenModel_Vertices[IModel_BoxVertices * 6 + (IModel_QuadVertices * 2) * 2];
IModel ChickenModel;

ModelPart ChickenModel_MakeLeg(Int32 x1, Int32 x2, Int32 legX1, Int32 legX2) {
	#define y1 (1.0f  / 64.0f)
	#define y2 (5.0f  / 16.0f)
	#define z2 (1.0f  / 16.0f)
	#define z1 (-2.0f / 16.0f)

	IModel* m = &ChickenModel;
	BoxDesc_YQuad(m, 32, 0, 3, 3, 
		x2 / 16.0f, x1 / 16.0f, z1, z2, y1); /* bottom feet */
	BoxDesc_ZQuad(m, 36, 3, 1, 5, 
		legX1 / 16.0f, legX2 / 16.0f, y1, y2, z2); /* vertical part of leg */

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
	Head = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, -1, 9, -7, 1, 11, -5);
	BoxDesc_TexOrigin(&desc, 14, 4); /* TODO: Find a more appropriate name. */
	BoxDesc_RotOrigin(&desc, 0, 9, -4);
	Head2 = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, -2, 11, -8, 2, 13, -6);
	BoxDesc_TexOrigin(&desc, 14, 0);
	BoxDesc_RotOrigin(&desc, 0, 9, -4);
	Head3 = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_RotatedBox(&desc, -3, 5, -4, 3, 11, 3);
	BoxDesc_TexOrigin(&desc, 0, 9);
	Torso = BoxDesc_BuildRotatedBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, -4, 7, -3, -3, 11, 3);
	BoxDesc_TexOrigin(&desc, 24, 13);
	BoxDesc_RotOrigin(&desc, -3, 11, 0);
	LeftWing = BoxDesc_BuildBox(&ChickenModel, &desc);

	BoxDesc_Box(&desc, 3, 7, -3, 4, 11, 3);
	BoxDesc_TexOrigin(&desc, 24, 13);
	BoxDesc_RotOrigin(&desc, 3, 11, 0);
	RightWing = BoxDesc_BuildBox(&ChickenModel, &desc);

	LeftLeg = ChickenModel_MakeLeg(-3, 0, -2, -1);
	RightLeg = ChickenModel_MakeLeg(0, 3, 1, 2);
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
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Head, true);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Head2, true);
	IModel_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, Head3, true);

	IModel_DrawPart(Torso);
	IModel_DrawRotate(0, 0, -Math_AbsF(entity->Anim.LeftArmX), LeftWing, false);
	IModel_DrawRotate(0, 0,  Math_AbsF(entity->Anim.LeftArmX), RightWing, false);

	PackedCol col = IModel_Cols[0];
	Int32 i;
	for (i = 0; i < Face_Count; i++) {
		IModel_Cols[i] = PackedCol_Scale(col, 0.7f);
	}

	IModel_DrawRotate(entity->Anim.LeftLegX,  0, 0, LeftLeg, false);
	IModel_DrawRotate(entity->Anim.RightLegX, 0, 0, RightLeg, false);
	IModel_UpdateVB();
}

IModel* ChickenModel_GetInstance(void) {
	IModel_Init(&ChickenModel);
	IModel_SetPointers(ChickenModel);
	return &ChickenModel;
}
#endif