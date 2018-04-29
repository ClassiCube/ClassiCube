#include "IModel.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "ModelCache.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"
#include "Entity.h"

#define UV_POS_MASK ((UInt16)0x7FFF)
#define UV_MAX ((UInt16)0x8000)
#define UV_MAX_SHIFT 15
#define AABB_Width(bb)  (bb->Max.X - bb->Min.X)
#define AABB_Height(bb) (bb->Max.Y - bb->Min.Y)
#define AABB_Length(bb) (bb->Max.Z - bb->Min.Z)

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, Int32 u, Int32 v) {
	vertex->X = x; vertex->Y = y; vertex->Z = z;
	vertex->U = (UInt16)u; vertex->V = (UInt16)v;
}

void ModelPart_Init(ModelPart* part, Int32 offset, Int32 count, Real32 rotX, Real32 rotY, Real32 rotZ) {
	part->Offset = (UInt16)offset; part->Count = (UInt16)count;
	part->RotX = rotX; part->RotY = rotY; part->RotZ = rotZ;
}

void IModel_GetTransform(Entity* entity, Vector3 pos) {
	Entity_GetTransform(entity, pos, entity->ModelScale);
}

void IModel_RecalcProperties(Entity* entity) { }

void IModel_Init(IModel* model) {
	model->Bobbing = true;
	model->UsesSkin = true;
	model->CalcHumanAnims = false;
	model->UsesHumanSkin = false;
	model->SurvivalScore = 5;
	model->Pushes = true;

	model->Gravity = 0.08f;
	model->Drag = Vector3_Create3(0.91f, 0.98f, 0.91f);
	model->GroundFriction = Vector3_Create3(0.6f, 1.0f, 0.6f);

	model->MaxScale = 2.0f;
	model->ShadowScale = 1.0f;
	model->NameScale = 1.0f;

	model->GetTransform = IModel_GetTransform;
	model->RecalcProperties = IModel_RecalcProperties;
}

bool IModel_ShouldRender(Entity* entity) {
	Vector3 pos = entity->Position;
	AABB bb; Entity_GetPickingBounds(entity, &bb);

	AABB* bbPtr = &bb;
	Real32 bbWidth  = AABB_Width(bbPtr);
	Real32 bbHeight = AABB_Height(bbPtr);
	Real32 bbLength = AABB_Length(bbPtr);

	Real32 maxYZ = max(bbHeight, bbLength);
	Real32 maxXYZ = max(bbWidth, maxYZ);
	pos.Y += AABB_Height(bbPtr) * 0.5f; /* Centre Y coordinate. */
	return FrustumCulling_SphereInFrustum(pos.X, pos.Y, pos.Z, maxXYZ);
}

Real32 IModel_MinDist(Real32 dist, Real32 extent) {
	/* Compare min coord, centre coord, and max coord */
	Real32 dMin = Math_AbsF(dist - extent), dMax = Math_AbsF(dist + extent);
	Real32 dMinMax = min(dMin, dMax);
	return min(Math_AbsF(dist), dMinMax);
}

Real32 IModel_RenderDistance(Entity* entity) {
	Vector3 pos = entity->Position;
	AABB* bb = &entity->ModelAABB;
	pos.Y += AABB_Height(bb) * 0.5f; /* Centre Y coordinate. */
	Vector3 camPos = Game_CurrentCameraPos;

	Real32 dx = IModel_MinDist(camPos.X - pos.X, AABB_Width(bb) * 0.5f);
	Real32 dy = IModel_MinDist(camPos.Y - pos.Y, AABB_Height(bb) * 0.5f);
	Real32 dz = IModel_MinDist(camPos.Z - pos.Z, AABB_Length(bb) * 0.5f);
	return dx * dx + dy * dy + dz * dz;
}

Matrix IModel_transform;
void IModel_Render(IModel* model, Entity* entity) {
	Vector3 pos = entity->Position;
	if (model->Bobbing) pos.Y += entity->Anim.BobbingModel;
	IModel_SetupState(model, entity);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);

	model->GetTransform(entity, pos);
	Matrix m;
	Matrix_Mul(&m, &entity->Transform, &Gfx_View);

	Gfx_LoadMatrix(&m);
	model->DrawModel(entity);
	Gfx_LoadMatrix(&Gfx_View);
}

void IModel_SetupState(IModel* model, Entity* entity) {
	model->index = 0;
	PackedCol col = entity->VTABLE->GetCol(entity);

	bool _64x64 = entity->SkinType != SKIN_TYPE_64x32;
	/* only apply when using humanoid skins */
	_64x64 &= model->UsesHumanSkin || entity->MobTextureId != NULL;

	IModel_uScale = entity->uScale * 0.015625f;
	IModel_vScale = entity->vScale * (_64x64 ? 0.015625f : 0.03125f);

	IModel_Cols[0] = col;
	if (!entity->NoShade) {
		IModel_Cols[1] = PackedCol_Scale(col, PACKEDCOL_SHADE_YMIN);
		IModel_Cols[2] = PackedCol_Scale(col, PACKEDCOL_SHADE_Z);
		IModel_Cols[4] = PackedCol_Scale(col, PACKEDCOL_SHADE_X);
	} else {
		IModel_Cols[1] = col; IModel_Cols[2] = col; IModel_Cols[4] = col;
	}
	IModel_Cols[3] = IModel_Cols[2]; 
	IModel_Cols[5] = IModel_Cols[4];

	Real32 yawDelta = entity->HeadY - entity->RotY;
	IModel_cosHead = Math_CosF(yawDelta * MATH_DEG2RAD);
	IModel_sinHead = Math_SinF(yawDelta * MATH_DEG2RAD);
	IModel_ActiveModel = model;
}

void IModel_UpdateVB(void) {
	IModel* model = IModel_ActiveModel;
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, ModelCache_Vertices, model->index);
	model->index = 0;
}

GfxResourceID IModel_GetTexture(Entity* entity) {
	IModel* model = IModel_ActiveModel;
	GfxResourceID pTex = model->UsesHumanSkin ? entity->TextureId : entity->MobTextureId;
	return pTex != NULL ? pTex : ModelCache_Textures[model->defaultTexIndex].TexID;
}

void IModel_DrawPart(ModelPart part) {
	IModel* model = IModel_ActiveModel;
	ModelVertex* src = &model->vertices[part.Offset];
	VertexP3fT2fC4b* dst = &ModelCache_Vertices[model->index];
	Int32 i;

	for (i = 0; i < part.Count; i++) {
		ModelVertex v = *src;
		dst->X = v.X; dst->Y = v.Y; dst->Z = v.Z;
		dst->Col = IModel_Cols[i >> 2];

		dst->U = (v.U & UV_POS_MASK) * IModel_uScale - (v.U >> UV_MAX_SHIFT) * 0.01f * IModel_uScale;
		dst->V = (v.V & UV_POS_MASK) * IModel_vScale - (v.V >> UV_MAX_SHIFT) * 0.01f * IModel_vScale;
		src++; dst++;
	}
	model->index += part.Count;
}

#define IModel_RotateX t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t;
#define IModel_RotateY t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t;
#define IModel_RotateZ t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t;

void IModel_DrawRotate(Real32 angleX, Real32 angleY, Real32 angleZ, ModelPart part, bool head) {
	IModel* model = IModel_ActiveModel;
	Real32 cosX = Math_CosF(-angleX), sinX = Math_SinF(-angleX);
	Real32 cosY = Math_CosF(-angleY), sinY = Math_SinF(-angleY);
	Real32 cosZ = Math_CosF(-angleZ), sinZ = Math_SinF(-angleZ);
	Real32 x = part.RotX, y = part.RotY, z = part.RotZ;

	ModelVertex* src = &model->vertices[part.Offset];
	VertexP3fT2fC4b* dst = &ModelCache_Vertices[model->index];
	Int32 i;
	for (i = 0; i < part.Count; i++) {
		ModelVertex v = *src;
		v.X -= x; v.Y -= y; v.Z -= z;
		Real32 t = 0;

		/* Rotate locally */
		if (IModel_Rotation == ROTATE_ORDER_ZYX) {
			IModel_RotateZ
			IModel_RotateY
			IModel_RotateX
		} else if (IModel_Rotation == ROTATE_ORDER_XZY) {
			IModel_RotateX
			IModel_RotateZ
			IModel_RotateY
		} else if (IModel_Rotation == ROTATE_ORDER_YZX) {
			IModel_RotateY
			IModel_RotateZ
			IModel_RotateX
		}

		/* Rotate globally */
		if (head) {
			t = IModel_cosHead * v.X - IModel_sinHead * v.Z; v.Z = IModel_sinHead * v.X + IModel_cosHead * v.Z; v.X = t; /* Inlined RotY */
		}
		dst->X = v.X + x; dst->Y = v.Y + y; dst->Z = v.Z + z;
		dst->Col = IModel_Cols[i >> 2];

		dst->U = (v.U & UV_POS_MASK) * IModel_uScale - (v.U >> UV_MAX_SHIFT) * 0.01f * IModel_uScale;
		dst->V = (v.V & UV_POS_MASK) * IModel_vScale - (v.V >> UV_MAX_SHIFT) * 0.01f * IModel_vScale;
		src++; dst++;
	}
	model->index += part.Count;
}

void BoxDesc_TexOrigin(BoxDesc* desc, Int32 x, Int32 y) {
	desc->TexX = x; desc->TexY = y;
}

void BoxDesc_SetBounds(BoxDesc* desc, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	desc->X1 = x1 / 16.0f; desc->X2 = x2 / 16.0f;
	desc->Y1 = y1 / 16.0f; desc->Y2 = y2 / 16.0f;
	desc->Z1 = z1 / 16.0f; desc->Z2 = z2 / 16.0f;
}

void BoxDesc_Expand(BoxDesc* desc, Real32 amount) {
	amount /= 16.0f;
	desc->X1 -= amount; desc->X2 += amount;
	desc->Y1 -= amount; desc->Y2 += amount;
	desc->Z1 -= amount; desc->Z2 += amount;
}

void BoxDesc_Scale(BoxDesc* desc, Real32 scale) {
	desc->X1 *= scale; desc->X2 *= scale;
	desc->Y1 *= scale; desc->Y2 *= scale;
	desc->Z1 *= scale; desc->Z2 *= scale;
	desc->RotX *= scale; desc->RotY *= scale; desc->RotZ *= scale;
}

void BoxDesc_RotOrigin(BoxDesc* desc, Int8 x, Int8 y, Int8 z) {
	desc->RotX = (Real32)x / 16.0f;
	desc->RotY = (Real32)y / 16.0f;
	desc->RotZ = (Real32)z / 16.0f;
}

void BoxDesc_MirrorX(BoxDesc* desc) {
	Real32 temp = desc->X1; desc->X1 = desc->X2; desc->X2 = temp;
}


#define BoxDesc_InitBox(desc)\
BoxDesc_TexOrigin(desc, 0, 0);\
BoxDesc_RotOrigin(desc, 0, 0, 0);\
BoxDesc_SetBounds(desc, (Real32)x1, (Real32)y1, (Real32)z1, (Real32)x2, (Real32)y2, (Real32)z2);\

void BoxDesc_Box(BoxDesc* desc, Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2) {
	BoxDesc_InitBox(desc);
	desc->SidesW = Math_AbsI(z2 - z1);
	desc->BodyW = Math_AbsI(x2 - x1);
	desc->BodyH = Math_AbsI(y2 - y1);
}

void BoxDesc_RotatedBox(BoxDesc* desc, Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2) {
	BoxDesc_InitBox(desc);
	desc->SidesW = Math_AbsI(y2 - y1);
	desc->BodyW = Math_AbsI(x2 - x1);
	desc->BodyH = Math_AbsI(z2 - z1);
}


void BoxDesc_BuildBox(ModelPart* part, IModel* m, BoxDesc* desc) {
	Int32 sidesW = desc->SidesW, bodyW = desc->BodyW, bodyH = desc->BodyH;
	Real32 x1 = desc->X1, y1 = desc->Y1, z1 = desc->Z1;
	Real32 x2 = desc->X2, y2 = desc->Y2, z2 = desc->Z2;
	Int32 x = desc->TexX, y = desc->TexY;

	BoxDesc_YQuad(m, x + sidesW, y, bodyW, sidesW, x1, x2, z2, z1, y2, true); /* top */
	BoxDesc_YQuad(m, x + sidesW + bodyW, y, bodyW, sidesW, x2, x1, z2, z1, y1, false); /* bottom */
	BoxDesc_ZQuad(m, x + sidesW, y + sidesW, bodyW, bodyH, x1, x2, y1, y2, z1, true); /* front */
	BoxDesc_ZQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW, bodyH, x2, x1, y1, y2, z2, true); /* back */
	BoxDesc_XQuad(m, x, y + sidesW, sidesW, bodyH, z1, z2, y1, y2, x2, true); /* left */
	BoxDesc_XQuad(m, x + sidesW + bodyW, y + sidesW, sidesW, bodyH, z2, z1, y1, y2, x1, true); /* right */

	ModelPart_Init(part, m->index - IMODEL_BOX_VERTICES, IMODEL_BOX_VERTICES,
		desc->RotX, desc->RotY, desc->RotZ);
}

void BoxDesc_BuildRotatedBox(ModelPart* part, IModel* m, BoxDesc* desc) {
	Int32 sidesW = desc->SidesW, bodyW = desc->BodyW, bodyH = desc->BodyH;
	Real32 x1 = desc->X1, y1 = desc->Y1, z1 = desc->Z1;
	Real32 x2 = desc->X2, y2 = desc->Y2, z2 = desc->Z2;
	Int32 x = desc->TexX, y = desc->TexY;

	BoxDesc_YQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW, bodyH, x1, x2, z1, z2, y2, false); /* top */
	BoxDesc_YQuad(m, x + sidesW, y + sidesW, bodyW, bodyH, x2, x1, z1, z2, y1, false); /* bottom */
	BoxDesc_ZQuad(m, x + sidesW, y, bodyW, sidesW, x2, x1, y1, y2, z1, false); /* front */
	BoxDesc_ZQuad(m, x + sidesW + bodyW, y, bodyW, sidesW, x1, x2, y2, y1, z2, false); /* back */
	BoxDesc_XQuad(m, x, y + sidesW, sidesW, bodyH, y2, y1, z2, z1, x2, false); /* left */
	BoxDesc_XQuad(m, x + sidesW + bodyW, y + sidesW, sidesW, bodyH, y1, y2, z2, z1, x1, false); /* right */

	/* rotate left and right 90 degrees	*/
	Int32 i;
	for (i = m->index - 8; i < m->index; i++) {
		ModelVertex vertex = m->vertices[i];
		Real32 z = vertex.Z; vertex.Z = vertex.Y; vertex.Y = z;
		m->vertices[i] = vertex;
	}

	ModelPart_Init(part, m->index - IMODEL_BOX_VERTICES, IMODEL_BOX_VERTICES,
		desc->RotX, desc->RotY, desc->RotZ);
}


void BoxDesc_XQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 z1, Real32 z2, Real32 y1, Real32 y2, Real32 x, bool swapU) {
	Int32 u1 = texX, u2 = texX + texWidth | UV_MAX;
	if (swapU) { Int32 tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x, y1, z1, u1, texY + texHeight | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y2, z1, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y2, z2, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y1, z2, u2, texY + texHeight | UV_MAX); m->index++;
}

void BoxDesc_YQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 z1, Real32 z2, Real32 y, bool swapU) {
	Int32 u1 = texX, u2 = texX + texWidth | UV_MAX;
	if (swapU) { Int32 tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x1, y, z2, u1, texY + texHeight | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x1, y, z1, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y, z1, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y, z2, u2, texY + texHeight | UV_MAX); m->index++;
}

void BoxDesc_ZQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 y1, Real32 y2, Real32 z, bool swapU) {
	Int32 u1 = texX, u2 = texX + texWidth | UV_MAX;
	if (swapU) { Int32 tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x1, y1, z, u1, texY + texHeight | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x1, y2, z, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y2, z, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y1, z, u2, texY + texHeight | UV_MAX); m->index++;
}