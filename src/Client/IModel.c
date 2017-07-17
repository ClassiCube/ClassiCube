#include "IModel.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "GameProps.h"
#include "ModelCache.h"
#include "GraphicsCommon.h"
#include "FrustumCulling.h"

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, UInt16 u, UInt16 v) {
	vertex->X = x; vertex->Y = y; vertex->Z = z;
	vertex->U = u; vertex->V = v;
}

void ModelPart_Init(ModelPart* part, Int32 offset, Int32 count, Real32 rotX, Real32 rotY, Real32 rotZ) {
	part->Offset = offset; part->Count = count;
	part->RotX = rotX; part->RotY = rotY; part->RotZ = rotZ;
}


void IModel_Init(IModel* model) {
	model->Rotation = RotateOrder_ZYX;
	model->Bobbing = true;
	model->UsesSkin = true;
	model->CalcHumanAnims = false;
	model->Gravity = 0.08f;
	model->Drag = Vector3_Create3(0.91f, 0.98f, 0.91f);
	model->GroundFriction = Vector3_Create3(0.6f, 1.0f, 0.6f);
	model->MaxScale = 2.0f;
	model->ShadowScale = 1.0f;
	model->NameScale = 1.0f;
}

bool IModel_ShouldRender(Entity* entity) {
	Vector3 pos = entity->Position;
	AABB bb = Entity_GetPickingBounds(entity);

	AABB* bbPtr = &bb;
	Real32 bbWidth = AABB_Width(bbPtr);
	Real32 bbHeight = AABB_Height(bbPtr);
	Real32 bbLength = AABB_Length(bbPtr);

	Real32 maxYZ = max(bbHeight, bbLength);
	Real32 maxXYZ = max(bbWidth, maxYZ);
	pos.Y += AABB_Height(bbPtr) * 0.5f; /* Centre Y coordinate. */
	return FrustumCulling_SphereInFrustum(pos.X, pos.Y, pos.Z, maxXYZ);
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

static Real32 IModel_MinDist(Real32 dist, Real32 extent) {
	/* Compare min coord, centre coord, and max coord */
	Real32 dMin = Math_AbsF(dist - extent), dMax = Math_AbsF(dist + extent);
	Real32 dMinMax = min(dMin, dMax);
	return min(Math_AbsF(dist), dMinMax);
}

void IModel_UpdateVB(void) {
	IModel* model = IModel_ActiveModel;
	GfxCommon_UpdateDynamicVb_IndexedTris(ModelCache_Vb, ModelCache_Vertices, model->index);
	model->index = 0;
}

GfxResourceID IModel_GetTexture(IModel* model, GfxResourceID pTex) {
	return pTex > 0 ? pTex : ModelCache_Textures[model->defaultTexIndex].TexID;
}

void IModel_DrawPart(ModelPart part) {
	IModel* model = IModel_ActiveModel;
	VertexP3fT2fC4b* dst = ModelCache_Vertices; dst += model->index;
	Int32 i;

	for (i = 0; i < part.Count; i++) {
		ModelVertex v = model->vertices[part.Offset + i];
		dst->X = v.X; dst->Y = v.Y; dst->Z = v.Z;
		dst->Colour = IModel_Cols[i >> 2];

		dst->U = v.U * IModel_uScale; dst->V = v.V * IModel_vScale;
		Int32 quadI = i & 3;
		if (quadI == 0 || quadI == 3) dst->V -= 0.01f * IModel_vScale;
		if (quadI == 2 || quadI == 3) dst->U -= 0.01f * IModel_uScale;

		dst++; model->index++;
	}
}

void IModel_DrawRotate(Real32 angleX, Real32 angleY, Real32 angleZ, ModelPart part, bool head) {
	IModel* model = IModel_ActiveModel;
	Real32 cosX = Math_Cos(-angleX), sinX = Math_Sin(-angleX);
	Real32 cosY = Math_Cos(-angleY), sinY = Math_Sin(-angleY);
	Real32 cosZ = Math_Cos(-angleZ), sinZ = Math_Sin(-angleZ);
	Real32 x = part.RotX, y = part.RotY, z = part.RotZ;
	VertexP3fT2fC4b* dst = ModelCache_Vertices; dst += model->index;
	Int32 i;

	for (i = 0; i < part.Count; i++) {
		ModelVertex v = model->vertices[part.Offset + i];
		v.X -= x; v.Y -= y; v.Z -= z;
		Real32 t = 0;

		/* Rotate locally */
		if (model->Rotation == RotateOrder_ZYX) {
			t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t; /* Inlined RotZ */
			t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t;  /* Inlined RotY */
			t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t; /* Inlined RotX */
		} else if (model->Rotation == RotateOrder_XZY) {
			t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t; /* Inlined RotX */
			t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t; /* Inlined RotZ */
			t = cosY * v.X - sinY * v.Z; v.Z = sinY * v.X + cosY * v.Z; v.X = t;  /* Inlined RotY */
		}

		/* Rotate globally */
		if (head) {
			t = IModel_cosHead * v.X - IModel_sinHead * v.Z; v.Z = IModel_sinHead * v.X + IModel_cosHead * v.Z; v.X = t; /* Inlined RotY */
		}
		dst->X = v.X + x; dst->Y = v.Y + y; dst->Z = v.Z + z;
		dst->Colour = IModel_Cols[i >> 2];

		dst->U = v.U * IModel_uScale; dst->V = v.V * IModel_vScale;
		Int32 quadI = i & 3;
		if (quadI == 0 || quadI == 3) dst->V -= 0.01f * IModel_vScale;
		if (quadI == 2 || quadI == 3) dst->U -= 0.01f * IModel_uScale;

		dst++; model->index++;
	}
}