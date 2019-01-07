#include "Model.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "Graphics.h"
#include "Entity.h"
#include "Camera.h"
#include "Event.h"
#include "ExtMath.h"
#include "TexturePack.h"
#include "Drawer.h"
#include "Block.h"
#include "Stream.h"
#include "Funcs.h"

struct _ModelsData Models;
VertexP3fT2fC4b Model_Vertices[MODEL_MAX_VERTICES];
struct Model* Human_ModelPtr;

#define UV_POS_MASK ((uint16_t)0x7FFF)
#define UV_MAX ((uint16_t)0x8000)
#define UV_MAX_SHIFT 15
#define AABB_Width(bb)  ((bb)->Max.X - (bb)->Min.X)
#define AABB_Height(bb) ((bb)->Max.Y - (bb)->Min.Y)
#define AABB_Length(bb) ((bb)->Max.Z - (bb)->Min.Z)

void ModelVertex_Init(struct ModelVertex* vertex, float x, float y, float z, int u, int v) {
	vertex->X = x; vertex->Y = y; vertex->Z = z;
	vertex->U = (uint16_t)u; vertex->V = (uint16_t)v;
}

void ModelPart_Init(struct ModelPart* part, int offset, int count, float rotX, float rotY, float rotZ) {
	part->Offset = offset; part->Count = count;
	part->RotX = rotX; part->RotY = rotY; part->RotZ = rotZ;
}


/*########################################################################################################################*
*------------------------------------------------------------Model--------------------------------------------------------*
*#########################################################################################################################*/
static void Model_GetTransform(struct Entity* entity, Vector3 pos, struct Matrix* m) {
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}
static void Model_NullFunc(struct Entity* entity) { }

void Model_Init(struct Model* model) {
	model->Bobbing  = true;
	model->UsesSkin = true;
	model->CalcHumanAnims = false;
	model->UsesHumanSkin  = false;
	model->Pushes = true;

	model->Gravity        = 0.08f;
	model->Drag           = Vector3_Create3(0.91f, 0.98f, 0.91f);
	model->GroundFriction = Vector3_Create3(0.6f, 1.0f, 0.6f);

	model->MaxScale    = 2.0f;
	model->ShadowScale = 1.0f;
	model->NameScale   = 1.0f;
	model->armX = 6; model->armY = 12;

	model->GetTransform     = Model_GetTransform;
	model->RecalcProperties = Model_NullFunc;
	model->DrawArm          = Model_NullFunc;
}

bool Model_ShouldRender(struct Entity* entity) {
	Vector3 pos = entity->Position;
	struct AABB bb;
	float bbWidth, bbHeight, bbLength;
	float maxYZ, maxXYZ;

	Entity_GetPickingBounds(entity, &bb);
	bbWidth  = AABB_Width(&bb);
	bbHeight = AABB_Height(&bb);
	bbLength = AABB_Length(&bb);

	maxYZ  = max(bbHeight, bbLength);
	maxXYZ = max(bbWidth,  maxYZ);
	pos.Y += bbHeight * 0.5f; /* Centre Y coordinate. */
	return FrustumCulling_SphereInFrustum(pos.X, pos.Y, pos.Z, maxXYZ);
}

static float Model_MinDist(float dist, float extent) {
	/* Compare min coord, centre coord, and max coord */
	float dMin = Math_AbsF(dist - extent), dMax = Math_AbsF(dist + extent);
	float dMinMax = min(dMin, dMax);
	return min(Math_AbsF(dist), dMinMax);
}

float Model_RenderDistance(struct Entity* entity) {
	Vector3 pos     = entity->Position;
	struct AABB* bb = &entity->ModelAABB;
	Vector3 camPos  = Camera.CurrentPos;
	float dx, dy, dz;

	/* X and Z are already at centre of model */
	/* Y is at feet, so needs to be moved up to centre */
	pos.Y += AABB_Height(bb) * 0.5f;

	dx = Model_MinDist(camPos.X - pos.X, AABB_Width(bb)  * 0.5f);
	dy = Model_MinDist(camPos.Y - pos.Y, AABB_Height(bb) * 0.5f);
	dz = Model_MinDist(camPos.Z - pos.Z, AABB_Length(bb) * 0.5f);
	return dx * dx + dy * dy + dz * dz;
}

void Model_Render(struct Model* model, struct Entity* entity) {
	struct Matrix m;
	Vector3 pos = entity->Position;
	if (model->Bobbing) pos.Y += entity->Anim.BobbingModel;

	Model_SetupState(model, entity);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);

	model->GetTransform(entity, pos, &entity->Transform);
	Matrix_Mul(&m, &entity->Transform, &Gfx_View);

	Gfx_LoadMatrix(MATRIX_VIEW, &m);
	model->DrawModel(entity);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx_View);
}

void Model_SetupState(struct Model* model, struct Entity* entity) {
	PackedCol col;
	bool _64x64;
	float yawDelta;

	model->index = 0;
	col = entity->VTABLE->GetCol(entity);

	_64x64  = entity->SkinType != SKIN_64x32;
	/* only apply when using humanoid skins */
	_64x64 &= model->UsesHumanSkin || entity->MobTextureId;

	Models.uScale = entity->uScale * 0.015625f;
	Models.vScale = entity->vScale * (_64x64 ? 0.015625f : 0.03125f);

	Models.Cols[0] = col;
	if (!entity->NoShade) {
		Models.Cols[1] = PackedCol_Scale(col, PACKEDCOL_SHADE_YMIN);
		Models.Cols[2] = PackedCol_Scale(col, PACKEDCOL_SHADE_Z);
		Models.Cols[4] = PackedCol_Scale(col, PACKEDCOL_SHADE_X);
	} else {
		Models.Cols[1] = col; Models.Cols[2] = col; Models.Cols[4] = col;
	}

	Models.Cols[3] = Models.Cols[2]; 
	Models.Cols[5] = Models.Cols[4];
	yawDelta = entity->HeadY - entity->RotY;

	Models.cosHead = (float)Math_Cos(yawDelta * MATH_DEG2RAD);
	Models.sinHead = (float)Math_Sin(yawDelta * MATH_DEG2RAD);
	Models.Active  = model;
}

void Model_UpdateVB(void) {
	struct Model* model = Models.Active;
	Gfx_UpdateDynamicVb_IndexedTris(Models.Vb, Model_Vertices, model->index);
	model->index = 0;
}

void Model_ApplyTexture(struct Entity* entity) {
	struct Model* model = Models.Active;
	struct ModelTex* data;
	GfxResourceID tex;
	bool _64x64;

	tex = model->UsesHumanSkin ? entity->TextureId : entity->MobTextureId;
	if (tex) {
		Models.skinType = entity->SkinType;
	} else {
		data = model->defaultTex;
		tex  = data->TexID;
		Models.skinType = data->SkinType;
	}

	Gfx_BindTexture(tex);
	_64x64 = Models.skinType != SKIN_64x32;

	Models.uScale = entity->uScale * 0.015625f;
	Models.vScale = entity->vScale * (_64x64 ? 0.015625f : 0.03125f);
}

void Model_DrawPart(struct ModelPart* part) {
	struct Model* model     = Models.Active;
	struct ModelVertex* src = &model->vertices[part->Offset];
	VertexP3fT2fC4b* dst    = &Model_Vertices[model->index];

	struct ModelVertex v;
	int i, count = part->Count;

	for (i = 0; i < count; i++) {
		v = *src;
		dst->X = v.X; dst->Y = v.Y; dst->Z = v.Z;
		dst->Col = Models.Cols[i >> 2];

		dst->U = (v.U & UV_POS_MASK) * Models.uScale - (v.U >> UV_MAX_SHIFT) * 0.01f * Models.uScale;
		dst->V = (v.V & UV_POS_MASK) * Models.vScale - (v.V >> UV_MAX_SHIFT) * 0.01f * Models.vScale;
		src++; dst++;
	}
	model->index += count;
}

#define Model_RotateX t = cosX * v.Y + sinX * v.Z; v.Z = -sinX * v.Y + cosX * v.Z; v.Y = t;
#define Model_RotateY t = cosY * v.X - sinY * v.Z; v.Z =  sinY * v.X + cosY * v.Z; v.X = t;
#define Model_RotateZ t = cosZ * v.X + sinZ * v.Y; v.Y = -sinZ * v.X + cosZ * v.Y; v.X = t;

void Model_DrawRotate(float angleX, float angleY, float angleZ, struct ModelPart* part, bool head) {
	struct Model* model     = Models.Active;
	struct ModelVertex* src = &model->vertices[part->Offset];
	VertexP3fT2fC4b* dst    = &Model_Vertices[model->index];

	float cosX = Math_CosF(-angleX), sinX = Math_SinF(-angleX);
	float cosY = Math_CosF(-angleY), sinY = Math_SinF(-angleY);
	float cosZ = Math_CosF(-angleZ), sinZ = Math_SinF(-angleZ);
	float t, x = part->RotX, y = part->RotY, z = part->RotZ;
	
	struct ModelVertex v;
	int i, count = part->Count;

	for (i = 0; i < count; i++) {
		v = *src;
		v.X -= x; v.Y -= y; v.Z -= z;

		/* Rotate locally */
		if (Models.Rotation == ROTATE_ORDER_ZYX) {
			Model_RotateZ
			Model_RotateY
			Model_RotateX
		} else if (Models.Rotation == ROTATE_ORDER_XZY) {
			Model_RotateX
			Model_RotateZ
			Model_RotateY
		} else if (Models.Rotation == ROTATE_ORDER_YZX) {
			Model_RotateY
			Model_RotateZ
			Model_RotateX
		}

		/* Rotate globally (inlined RotY) */
		if (head) {
			t = Models.cosHead * v.X - Models.sinHead * v.Z; v.Z = Models.sinHead * v.X + Models.cosHead * v.Z; v.X = t;
		}
		dst->X = v.X + x; dst->Y = v.Y + y; dst->Z = v.Z + z;
		dst->Col = Models.Cols[i >> 2];

		dst->U = (v.U & UV_POS_MASK) * Models.uScale - (v.U >> UV_MAX_SHIFT) * 0.01f * Models.uScale;
		dst->V = (v.V & UV_POS_MASK) * Models.vScale - (v.V >> UV_MAX_SHIFT) * 0.01f * Models.vScale;
		src++; dst++;
	}
	model->index += count;
}

void Model_RenderArm(struct Model* model, struct Entity* entity) {
	struct Matrix m, translate;
	Vector3 pos = entity->Position;
	if (model->Bobbing) pos.Y += entity->Anim.BobbingModel;

	Model_SetupState(model, entity);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	Model_ApplyTexture(entity);

	if (Game_ClassicArmModel) {
		/* TODO: Position's not quite right. */
		/* Matrix_Translate(out m, -armX / 16f + 0.2f, -armY / 16f - 0.20f, 0); */
		/* is better, but that breaks the dig animation */
		Matrix_Translate(&translate, -model->armX / 16.0f,         -model->armY / 16.0f - 0.10f, 0);
	} else {
		Matrix_Translate(&translate, -model->armX / 16.0f + 0.10f, -model->armY / 16.0f - 0.26f, 0);
	}

	Entity_GetTransform(entity, pos, entity->ModelScale, &m);
	Matrix_Mul(&m, &m,         &Gfx_View);
	Matrix_Mul(&m, &translate, &m);

	Gfx_LoadMatrix(MATRIX_VIEW, &m);
	Models.Rotation = ROTATE_ORDER_YZX;
	model->DrawArm(entity);
	Models.Rotation = ROTATE_ORDER_ZYX;
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx_View);
}

void Model_DrawArmPart(struct ModelPart* part) {
	struct Model* model  = Models.Active;
	struct ModelPart arm = *part;
	arm.RotX = model->armX / 16.0f; 
	arm.RotY = (model->armY + model->armY / 2) / 16.0f;

	if (Game_ClassicArmModel) {
		Model_DrawRotate(0, -90 * MATH_DEG2RAD, 120 * MATH_DEG2RAD, &arm, false);
	} else {
		Model_DrawRotate(-20 * MATH_DEG2RAD, -70 * MATH_DEG2RAD, 135 * MATH_DEG2RAD, &arm, false);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------BoxDesc--------------------------------------------------------*
*#########################################################################################################################*/
void BoxDesc_BuildBox(struct ModelPart* part, const struct BoxDesc* desc) {
	int sidesW = desc->SizeZ, bodyW = desc->SizeX, bodyH = desc->SizeY;
	float x1 = desc->X1, y1 = desc->Y1, z1 = desc->Z1;
	float x2 = desc->X2, y2 = desc->Y2, z2 = desc->Z2;
	int x = desc->TexX, y = desc->TexY;
	struct Model* m = Models.Active;

	BoxDesc_YQuad(m, x + sidesW,                  y,          bodyW, sidesW, x1, x2, z2, z1, y2, true);  /* top */
	BoxDesc_YQuad(m, x + sidesW + bodyW,          y,          bodyW, sidesW, x2, x1, z2, z1, y1, false); /* bottom */
	BoxDesc_ZQuad(m, x + sidesW,                  y + sidesW, bodyW,  bodyH, x1, x2, y1, y2, z1, true);  /* front */
	BoxDesc_ZQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW,  bodyH, x2, x1, y1, y2, z2, true);  /* back */
	BoxDesc_XQuad(m, x,                           y + sidesW, sidesW, bodyH, z1, z2, y1, y2, x2, true);  /* left */
	BoxDesc_XQuad(m, x + sidesW + bodyW,          y + sidesW, sidesW, bodyH, z2, z1, y1, y2, x1, true);  /* right */

	ModelPart_Init(part, m->index - MODEL_BOX_VERTICES, MODEL_BOX_VERTICES,
		desc->RotX, desc->RotY, desc->RotZ);
}

void BoxDesc_BuildRotatedBox(struct ModelPart* part, const struct BoxDesc* desc) {
	int sidesW = desc->SizeY, bodyW = desc->SizeX, bodyH = desc->SizeZ;
	float x1 = desc->X1, y1 = desc->Y1, z1 = desc->Z1;
	float x2 = desc->X2, y2 = desc->Y2, z2 = desc->Z2;
	int x = desc->TexX, y = desc->TexY, i;
	struct Model* m = Models.Active;

	BoxDesc_YQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW,  bodyH, x1, x2, z1, z2, y2, false); /* top */
	BoxDesc_YQuad(m, x + sidesW,                  y + sidesW, bodyW,  bodyH, x2, x1, z1, z2, y1, false); /* bottom */
	BoxDesc_ZQuad(m, x + sidesW,                  y,          bodyW, sidesW, x2, x1, y1, y2, z1, false); /* front */
	BoxDesc_ZQuad(m, x + sidesW + bodyW,          y,          bodyW, sidesW, x1, x2, y2, y1, z2, false); /* back */
	BoxDesc_XQuad(m, x,                           y + sidesW, sidesW, bodyH, y2, y1, z2, z1, x2, false); /* left */
	BoxDesc_XQuad(m, x + sidesW + bodyW,          y + sidesW, sidesW, bodyH, y1, y2, z2, z1, x1, false); /* right */

	/* rotate left and right 90 degrees	*/
	for (i = m->index - 8; i < m->index; i++) {
		struct ModelVertex vertex = m->vertices[i];
		float z = vertex.Z; vertex.Z = vertex.Y; vertex.Y = z;
		m->vertices[i] = vertex;
	}

	ModelPart_Init(part, m->index - MODEL_BOX_VERTICES, MODEL_BOX_VERTICES,
		desc->RotX, desc->RotY, desc->RotZ);
}


void BoxDesc_XQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float z1, float z2, float y1, float y2, float x, bool swapU) {
	int u1 = texX, u2 = (texX + texWidth) | UV_MAX, tmp;
	if (swapU) { tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x, y1, z1, u1, (texY + texHeight) | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y2, z1, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y2, z2, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y1, z2, u2, (texY + texHeight) | UV_MAX); m->index++;
}

void BoxDesc_YQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float z1, float z2, float y, bool swapU) {
	int u1 = texX, u2 = (texX + texWidth) | UV_MAX, tmp;
	if (swapU) { tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x1, y, z2, u1, (texY + texHeight) | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x1, y, z1, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y, z1, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y, z2, u2, (texY + texHeight) | UV_MAX); m->index++;
}

void BoxDesc_ZQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float y1, float y2, float z, bool swapU) {
	int u1 = texX, u2 = (texX + texWidth) | UV_MAX, tmp;
	if (swapU) { tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x1, y1, z, u1, (texY + texHeight) | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x1, y2, z, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y2, z, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y1, z, u2, (texY + texHeight) | UV_MAX); m->index++;
}


/*########################################################################################################################*
*--------------------------------------------------------Models common----------------------------------------------------*
*#########################################################################################################################*/
static struct Model* models_head;
static struct ModelTex* textures_head;

#define Model_RetSize(x,y,z) static Vector3 P = { (x)/16.0f,(y)/16.0f,(z)/16.0f }; *size = P;
#define Model_RetAABB(x1,y1,z1, x2,y2,z2) static struct AABB BB = { (x1)/16.0f,(y1)/16.0f,(z1)/16.0f, (x2)/16.0f,(y2)/16.0f,(z2)/16.0f }; *bb = BB;

static void Models_ContextLost(void* obj) {
	Gfx_DeleteVb(&Models.Vb);
}

static void Models_ContextRecreated(void* obj) {
	Models.Vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, MODEL_MAX_VERTICES);
}

static void Model_Make(struct Model* model) {
	struct Model* active = Models.Active;
	Models.Active = model;
	model->CreateParts();

	model->initalised = true;
	model->index      = 0;
	Models.Active     = active;
}

struct Model* Model_Get(const String* name) {
	struct Model* model;

	for (model = models_head; model; model = model->Next) {
		if (!String_CaselessEqualsConst(name, model->Name)) continue;

		if (!model->initalised) Model_Make(model);
		return model;
	}
	return NULL;
}

struct ModelTex* Model_GetTexture(const String* name) {
	struct ModelTex* tex;

	for (tex = textures_head; tex; tex = tex->Next) {
		if (String_CaselessEqualsConst(name, tex->Name)) return tex;
	}
	return NULL;
}

void Model_Register(struct Model* model) {
	model->Next = models_head;
	models_head = model;
}

void Model_RegisterTexture(struct ModelTex* tex) {
	tex->Next     = textures_head;
	textures_head = tex;
}

static void Models_TextureChanged(void* obj, struct Stream* stream, const String* name) {
	struct ModelTex* tex;

	for (tex = textures_head; tex; tex = tex->Next) {
		if (!String_CaselessEqualsConst(name, tex->Name)) continue;

		Game_UpdateTexture(&tex->TexID, stream, name, &tex->SkinType);
		return;
	}
}


/*########################################################################################################################*
*---------------------------------------------------------HumanModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelLimbs {
	struct ModelPart LeftLeg, RightLeg, LeftArm, RightArm, LeftLegLayer, RightLegLayer, LeftArmLayer, RightArmLayer;
};
struct ModelSet {
	struct ModelPart Head, Torso, Hat, TorsoLayer;
	struct ModelLimbs Limbs[3];
};

static void HumanModel_DrawModelSet(struct Entity* entity, struct ModelSet* model) {
	struct ModelLimbs* set;
	int type;

	Model_ApplyTexture(entity);
	Gfx_SetAlphaTest(false);

	type = Models.skinType;
	set  = &model->Limbs[type & 0x3];

	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &model->Head, true);
	Model_DrawPart(&model->Torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, entity->Anim.LeftLegZ,  &set->LeftLeg,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, entity->Anim.RightLegZ, &set->RightLeg, false);

	Models.Rotation = ROTATE_ORDER_XZY;
	Model_DrawRotate(entity->Anim.LeftArmX,  0, entity->Anim.LeftArmZ,  &set->LeftArm,  false);
	Model_DrawRotate(entity->Anim.RightArmX, 0, entity->Anim.RightArmZ, &set->RightArm, false);
	Models.Rotation = ROTATE_ORDER_ZYX;
	Model_UpdateVB();

	Gfx_SetAlphaTest(true);
	if (type != SKIN_64x32) {
		Model_DrawPart(&model->TorsoLayer);
		Model_DrawRotate(entity->Anim.LeftLegX,  0, entity->Anim.LeftLegZ,  &set->LeftLegLayer,  false);
		Model_DrawRotate(entity->Anim.RightLegX, 0, entity->Anim.RightLegZ, &set->RightLegLayer, false);

		Models.Rotation = ROTATE_ORDER_XZY;
		Model_DrawRotate(entity->Anim.LeftArmX,  0, entity->Anim.LeftArmZ,  &set->LeftArmLayer,  false);
		Model_DrawRotate(entity->Anim.RightArmX, 0, entity->Anim.RightArmZ, &set->RightArmLayer, false);
		Models.Rotation = ROTATE_ORDER_ZYX;
	}
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &model->Hat, true);
	Model_UpdateVB();
}

static void HumanModel_DrawArmSet(struct Entity* entity, struct ModelSet* model) {
	struct ModelLimbs* set;
	int type;

	type = Models.skinType;
	set  = &model->Limbs[type & 0x3];

	Model_DrawArmPart(&set->RightArm);
	if (type != SKIN_64x32) Model_DrawArmPart(&set->RightArmLayer);
	Model_UpdateVB();
}


static struct ModelSet human_set;
static struct ModelVertex human_vertices[MODEL_BOX_VERTICES * (7 + 7 + 4)];
static struct ModelTex human_tex = { "char.png" };
static struct Model  human_model = { "humanoid", human_vertices, &human_tex };

static void HumanModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,24,-4, 4,32,4),
		BoxDesc_Rot(0,24,0),
	}; 
	static struct BoxDesc torso = {
		BoxDesc_Tex(16,16),
		BoxDesc_Box(-4,12,-2, 4,24,2),
		BoxDesc_Rot(0,12,0),
	}; 
	static struct BoxDesc hat = {
		BoxDesc_Tex(32,0),
		BoxDesc_Dims(-4,24,-4, 4,32,4),
		BoxDesc_Bounds(-4.5f,23.5f,-4.5f, 4.5f,32.5f,4.5f),
		BoxDesc_Rot(0,24,0),
	}; 
	static struct BoxDesc torsoL = {
		BoxDesc_Tex(16,32),
		BoxDesc_Dims(-4,12,-2, 4,24,2),
		BoxDesc_Bounds(-4.5f,11.5f,-2.5f, 4.5f,24.5f,2.5f),
		BoxDesc_Rot(0,12,0),
	};

	static struct BoxDesc lArm = {
		BoxDesc_Tex(40,16),
		BoxDesc_Box(-4,12,-2, -8,24,2),
		BoxDesc_Rot(-5,22,0),
	};
	static struct BoxDesc rArm = {
		BoxDesc_Tex(40,16),
		BoxDesc_Box(4,12,-2, 8,24,2),
		BoxDesc_Rot(5,22,0),
	};
	static struct BoxDesc lLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,-2, -4,12,2),
		BoxDesc_Rot(0,12,0),
	};
	static struct BoxDesc rLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,-2, 4,12,2),
		BoxDesc_Rot(0,12,0),
	};

	static struct BoxDesc lArm64 = {
		BoxDesc_Tex(32,48),
		BoxDesc_Box(-8,12,-2, -4,24,2),
		BoxDesc_Rot(-5,22,0),
	};
	static struct BoxDesc lLeg64 = {
		BoxDesc_Tex(16,48),
		BoxDesc_Box(-4,0,-2, 0,12,2),
		BoxDesc_Rot(0,12,0),
	};
	static struct BoxDesc lArmL = {
		BoxDesc_Tex(48,48),
		BoxDesc_Dims(-8,12,-2, -4,24,2),
		BoxDesc_Bounds(-8.5f,11.5f,-2.5f, -3.5f,24.5f,2.5f),
		BoxDesc_Rot(-5,22,0),
	};
	static struct BoxDesc rArmL = {
		BoxDesc_Tex(40,32),
		BoxDesc_Dims(4,12,-2, 8,24,2),
		BoxDesc_Bounds(3.5f,11.5f,-2.5f, 8.5f,24.5f,2.5f),
		BoxDesc_Rot(5,22,0),
	};
	static struct BoxDesc lLegL = {
		BoxDesc_Tex(0,48),
		BoxDesc_Dims(-4,0,-2, 0,12,2),
		BoxDesc_Bounds(-4.5f,-0.5f,-2.5f, 0.5f,12.5f,2.5f),
		BoxDesc_Rot(0,12,0),
	};
	static struct BoxDesc rLegL = {
		BoxDesc_Tex(0,32),
		BoxDesc_Dims(0,0,-2, 4,12,2),
		BoxDesc_Bounds(-0.5f,-0.5f,-2.5f, 4.5f,12.5f,2.5f),
		BoxDesc_Rot(0,12,0),
	};

	struct ModelLimbs* set     = &human_set.Limbs[0];
	struct ModelLimbs* set64   = &human_set.Limbs[1];
	struct ModelLimbs* setSlim = &human_set.Limbs[2];

	BoxDesc_BuildBox(&human_set.Head,  &head);
	BoxDesc_BuildBox(&human_set.Torso, &torso);
	BoxDesc_BuildBox(&human_set.Hat,   &hat);
	BoxDesc_BuildBox(&human_set.TorsoLayer, &torsoL);

	BoxDesc_BuildBox(&set->LeftLeg,  &lLeg);
	BoxDesc_BuildBox(&set->RightLeg, &rLeg);
	BoxDesc_BuildBox(&set->LeftArm,  &lArm);
	BoxDesc_BuildBox(&set->RightArm, &rArm);

	/* 64x64 arms */
	BoxDesc_BuildBox(&set64->LeftLeg, &lLeg64);
	set64->RightLeg = set->RightLeg;
	BoxDesc_BuildBox(&set64->LeftArm, &lArm64);
	set64->RightArm = set->RightArm;

	lArm64.SizeX -= 1; lArm64.X1 += 1.0f/16.0f;
	rArm.SizeX   -= 1; rArm.X2   -= 1.0f/16.0f;

	setSlim->LeftLeg  = set64->LeftLeg;
	setSlim->RightLeg = set64->RightLeg;
	BoxDesc_BuildBox(&setSlim->LeftArm,  &lArm64);
	BoxDesc_BuildBox(&setSlim->RightArm, &rArm);

	/* 64x64 legs */
	BoxDesc_BuildBox(&set64->LeftLegLayer,  &lLegL);
	BoxDesc_BuildBox(&set64->RightLegLayer, &rLegL);
	BoxDesc_BuildBox(&set64->LeftArmLayer,  &lArmL);
	BoxDesc_BuildBox(&set64->RightArmLayer, &rArmL);

	lArmL.SizeX -= 1; lArmL.X1 += 1.0f/16.0f;
	rArmL.SizeX -= 1; rArmL.X2 -= 1.0f/16.0f;

	setSlim->LeftLegLayer  = set64->LeftLegLayer;
	setSlim->RightLegLayer = set64->RightLegLayer;
	BoxDesc_BuildBox(&setSlim->LeftArmLayer,  &lArmL);
	BoxDesc_BuildBox(&setSlim->RightArmLayer, &rArmL);
}

static float HumanModel_GetEyeY(struct Entity* entity)   { return 26.0f/16.0f; }
static void HumanModel_GetCollisionSize(Vector3* size)   { Model_RetSize(8.6f,28.1f,8.6f); }
static void HumanModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-8,0,-4, 8,32,4); }

static void HumanModel_DrawModel(struct Entity* entity) {
	HumanModel_DrawModelSet(entity, &human_set);
}

static void HumanModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArmSet(entity, &human_set);
}

static struct Model* HumanoidModel_GetInstance(void) {
	Model_Init(&human_model);
	Model_SetPointers(human_model, HumanModel);
	human_model.DrawArm  = HumanModel_DrawArm;
	human_model.CalcHumanAnims = true;
	human_model.UsesHumanSkin  = true;
	human_model.NameYOffset = 32.5f/16.0f;
	return &human_model;
}


/*########################################################################################################################*
*---------------------------------------------------------ChibiModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelSet chibi_set;
static struct ModelVertex chibi_vertices[MODEL_BOX_VERTICES * (7 + 7 + 4)];
static struct Model chibi_model = { "chibi", chibi_vertices, &human_tex };

CC_NOINLINE static void ChibiModel_ScalePart(struct ModelPart* dst, struct ModelPart* src) {
	struct ModelVertex v;
	int i;

	*dst = *src;
	dst->RotX *= 0.5f; dst->RotY *= 0.5f; dst->RotZ *= 0.5f;
	
	for (i = src->Offset; i < src->Offset + src->Count; i++) {
		v = human_model.vertices[i];
		v.X *= 0.5f; v.Y *= 0.5f; v.Z *= 0.5f;
		chibi_model.vertices[i] = v;
	}
}

CC_NOINLINE static void ChibiModel_ScaleLimbs(struct ModelLimbs* dst, struct ModelLimbs* src) {
	ChibiModel_ScalePart(&dst->LeftLeg,  &src->LeftLeg);
	ChibiModel_ScalePart(&dst->RightLeg, &src->RightLeg);
	ChibiModel_ScalePart(&dst->LeftArm,  &src->LeftArm);
	ChibiModel_ScalePart(&dst->RightArm, &src->RightArm);

	ChibiModel_ScalePart(&dst->LeftLegLayer,  &src->LeftLegLayer);
	ChibiModel_ScalePart(&dst->RightLegLayer, &src->RightLegLayer);
	ChibiModel_ScalePart(&dst->LeftArmLayer,  &src->LeftArmLayer);
	ChibiModel_ScalePart(&dst->RightArmLayer, &src->RightArmLayer);
}

static void ChibiModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,12,-4, 4,20,4),
		BoxDesc_Rot(0,13,0),
	};
	static struct BoxDesc hat = {
		BoxDesc_Tex(32,0),
		BoxDesc_Dims(-4,12,-4, 4,20,4),
		BoxDesc_Bounds(-4.25f,11.75f,-4.25f, 4.25f,20.25f,4.25f),
		BoxDesc_Rot(0,13,0),
	}; 

	/* Chibi is mostly just half scale humanoid */
	ChibiModel_ScalePart(&chibi_set.Torso,      &human_set.Torso);
	ChibiModel_ScalePart(&chibi_set.TorsoLayer, &human_set.TorsoLayer);
	ChibiModel_ScaleLimbs(&chibi_set.Limbs[0], &human_set.Limbs[0]);
	ChibiModel_ScaleLimbs(&chibi_set.Limbs[1], &human_set.Limbs[1]);
	ChibiModel_ScaleLimbs(&chibi_set.Limbs[2], &human_set.Limbs[2]);

	/* But head is at normal size */
	chibi_model.index = human_set.Head.Offset;
	BoxDesc_BuildBox(&chibi_set.Head, &head);
	chibi_model.index = human_set.Hat.Offset;
	BoxDesc_BuildBox(&chibi_set.Hat,  &hat);
}

static float ChibiModel_GetEyeY(struct Entity* entity)   { return 14.0f/16.0f; }
static void ChibiModel_GetCollisionSize(Vector3* size)   { Model_RetSize(4.6f,20.1f,4.6f); }
static void ChibiModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-4,0,-4, 4,16,4); }

static void ChibiModel_DrawModel(struct Entity* entity) {
	HumanModel_DrawModelSet(entity, &chibi_set);
}

static void ChibiModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArmSet(entity, &chibi_set);
}

static struct Model* ChibiModel_GetInstance(void) {
	Model_Init(&chibi_model);
	Model_SetPointers(chibi_model, ChibiModel);
	chibi_model.DrawArm  = ChibiModel_DrawArm;
	chibi_model.armX = 3; chibi_model.armY = 6;
	chibi_model.CalcHumanAnims = true;
	chibi_model.UsesHumanSkin  = true;
	chibi_model.MaxScale    = 3.0f;
	chibi_model.ShadowScale = 0.5f;
	chibi_model.NameYOffset = 20.2f/16.0f;
	return &chibi_model;
}


/*########################################################################################################################*
*--------------------------------------------------------SittingModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct Model sitting_model = { "sit", human_vertices, &human_tex };
#define SIT_OFFSET 10.0f
static void SittingModel_CreateParts(void) { }

static float SittingModel_GetEyeY(struct Entity* entity)   { return (26.0f - SIT_OFFSET) / 16.0f; }
static void SittingModel_GetCollisionSize(Vector3* size)   { Model_RetSize(8.6f,28.1f - SIT_OFFSET,8.6f); }
static void SittingModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-8,0,-4, 8,32 - SIT_OFFSET,4); }

static void SittingModel_GetTransform(struct Entity* entity, Vector3 pos, struct Matrix* m) {
	pos.Y -= (SIT_OFFSET / 16.0f) * entity->ModelScale.Y;
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}

static void SittingModel_DrawModel(struct Entity* entity) {
	entity->Anim.LeftLegX = 1.5f;  entity->Anim.RightLegX = 1.5f;
	entity->Anim.LeftLegZ = -0.1f; entity->Anim.RightLegZ = 0.1f;
	HumanModel_DrawModel(entity);
}

static struct Model* SittingModel_GetInstance(void) {
	Model_Init(&sitting_model);
	Model_SetPointers(sitting_model, SittingModel);
	sitting_model.DrawArm  = HumanModel_DrawArm;
	sitting_model.CalcHumanAnims = true;
	sitting_model.UsesHumanSkin  = true;
	sitting_model.ShadowScale  = 0.5f;
	sitting_model.GetTransform = SittingModel_GetTransform;
	sitting_model.NameYOffset  = 32.5f/16.0f;
	return &sitting_model;
}


/*########################################################################################################################*
*--------------------------------------------------------CorpseModel------------------------------------------------------*
*#########################################################################################################################*/
static struct Model corpse_model;
static void CorpseModel_CreateParts(void) { }

static void CorpseModel_DrawModel(struct Entity* entity) {
	entity->Anim.LeftLegX = 0.025f; entity->Anim.RightLegX = 0.025f;
	entity->Anim.LeftArmX = 0.025f; entity->Anim.RightArmX = 0.025f;
	entity->Anim.LeftLegZ = -0.15f; entity->Anim.RightLegZ =  0.15f;
	entity->Anim.LeftArmZ = -0.20f; entity->Anim.RightArmZ =  0.20f;
	HumanModel_DrawModel(entity);
}

static struct Model* CorpseModel_GetInstance(void) {
	corpse_model      = human_model;
	corpse_model.Name = "corpse";
	corpse_model.CreateParts = CorpseModel_CreateParts;
	corpse_model.DrawModel   = CorpseModel_DrawModel;
	return &corpse_model;
}


/*########################################################################################################################*
*---------------------------------------------------------HeadModel-------------------------------------------------------*
*#########################################################################################################################*/
static struct Model head_model = { "head", human_vertices, &human_tex };
static void HeadModel_CreateParts(void) { }

static float HeadModel_GetEyeY(struct Entity* entity)   { return 6.0f/16.0f; }
static void HeadModel_GetCollisionSize(Vector3* size)   { Model_RetSize(7.9f,7.9f,7.9f); }
static void HeadModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-4,0,-4, 4,8,4); }

static void HeadModel_GetTransform(struct Entity* entity, Vector3 pos, struct Matrix* m) {
	pos.Y -= (24.0f/16.0f) * entity->ModelScale.Y;
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}

static void HeadModel_DrawModel(struct Entity* entity) {
	struct ModelPart part;
	Model_ApplyTexture(entity);

	part = human_set.Head; part.RotY += 4.0f/16.0f;
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &part, true);
	part = human_set.Hat;  part.RotY += 4.0f/16.0f;
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &part, true);

	Model_UpdateVB();
}

static struct Model* HeadModel_GetInstance(void) {
	Model_Init(&head_model);
	Model_SetPointers(head_model, HeadModel);
	head_model.UsesHumanSkin = true;
	head_model.Pushes        = false;
	head_model.GetTransform  = HeadModel_GetTransform;
	head_model.NameYOffset   = 32.5f/16.0f;
	return &head_model;
}


/*########################################################################################################################*
*--------------------------------------------------------ChickenModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart chicken_head, chicken_head2, chicken_head3, chicken_torso;
static struct ModelPart chicken_leftLeg, chicken_rightLeg, chicken_leftWing, Chicken_RightWing;
static struct ModelVertex chicken_vertices[MODEL_BOX_VERTICES * 6 + (MODEL_QUAD_VERTICES * 2) * 2];
static struct ModelTex chicken_tex = { "chicken.png" };
static struct Model chicken_model = { "chicken", chicken_vertices, &chicken_tex };

static void ChickenModel_MakeLeg(struct ModelPart* part, int x1, int x2, int legX1, int legX2) {
#define ch_y1 (1.0f  / 64.0f)
#define ch_y2 (5.0f  / 16.0f)
#define ch_z2 (1.0f  / 16.0f)
#define ch_z1 (-2.0f / 16.0f)

	struct Model* m = &chicken_model;
	BoxDesc_YQuad(m, 32, 0, 3, 3,
		x2 / 16.0f, x1 / 16.0f, ch_z1, ch_z2, ch_y1, false); /* bottom feet */
	BoxDesc_ZQuad(m, 36, 3, 1, 5,
		legX1 / 16.0f, legX2 / 16.0f, ch_y1, ch_y2, ch_z2, false); /* vertical part of leg */

	ModelPart_Init(part, m->index - MODEL_QUAD_VERTICES * 2, MODEL_QUAD_VERTICES * 2,
		0.0f / 16.0f, 5.0f / 16.0f, 1.0f / 16.0f);
}

static void ChickenModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-2,9,-6, 2,15,-3),
		BoxDesc_Rot(0,9,-4),
	};
	static struct BoxDesc head2 = { /* TODO: Find a more appropriate name. */
		BoxDesc_Tex(14,4),
		BoxDesc_Box(-1,9,-7, 1,11,-5),
		BoxDesc_Rot(0,9,-4),
	};
	static struct BoxDesc head3 = {
		BoxDesc_Tex(14,0),
		BoxDesc_Box(-2,11,-8, 2,13,-6),
		BoxDesc_Rot(0,9,-4),
	};
	static struct BoxDesc torso = {
		BoxDesc_Tex(0,9),
		BoxDesc_Box(-3,5,-4, 3,11,3),
		BoxDesc_Rot(0,5,0),
	};
	static struct BoxDesc lWing = {
		BoxDesc_Tex(24,13),
		BoxDesc_Box(-4,7,-3, -3,11,3),
		BoxDesc_Rot(-3,11,0),
	}; 
	static struct BoxDesc rWing = {
		BoxDesc_Tex(24,13),
		BoxDesc_Box(3,7,-3, 4,11,3),
		BoxDesc_Rot(3,11,0),
	}; 
	
	BoxDesc_BuildBox(&chicken_head,  &head);
	BoxDesc_BuildBox(&chicken_head2, &head2);
	BoxDesc_BuildBox(&chicken_head3, &head3);
	BoxDesc_BuildRotatedBox(&chicken_torso, &torso);
	BoxDesc_BuildBox(&chicken_leftWing,     &lWing);
	BoxDesc_BuildBox(&Chicken_RightWing,    &rWing);

	ChickenModel_MakeLeg(&chicken_leftLeg, -3, 0, -2, -1);
	ChickenModel_MakeLeg(&chicken_rightLeg, 0, 3, 1, 2);
}

static float ChickenModel_GetEyeY(struct Entity* entity)   { return 14.0f/16.0f; }
static void ChickenModel_GetCollisionSize(Vector3* size)   { Model_RetSize(8.0f,12.0f,8.0f); }
static void ChickenModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-4,0,-8, 4,15,4); }

static void ChickenModel_DrawModel(struct Entity* entity) {
	PackedCol col = Models.Cols[0];
	int i;
	Model_ApplyTexture(entity);

	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &chicken_head,  true);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &chicken_head2, true);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &chicken_head3, true);

	Model_DrawPart(&chicken_torso);
	Model_DrawRotate(0, 0, -Math_AbsF(entity->Anim.LeftArmX), &chicken_leftWing,  false);
	Model_DrawRotate(0, 0,  Math_AbsF(entity->Anim.LeftArmX), &Chicken_RightWing, false);

	for (i = 0; i < FACE_COUNT; i++) {
		Models.Cols[i] = PackedCol_Scale(col, 0.7f);
	}

	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &chicken_leftLeg, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &chicken_rightLeg, false);
	Model_UpdateVB();
}

static struct Model* ChickenModel_GetInstance(void) {
	Model_Init(&chicken_model);
	Model_SetPointers(chicken_model, ChickenModel);
	chicken_model.NameYOffset = 1.0125f;
	return &chicken_model;
}


/*########################################################################################################################*
*--------------------------------------------------------CreeperModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart creeper_head, creeper_torso, creeper_leftLegFront;
static struct ModelPart creeper_rightLegFront, creeper_leftLegBack, creeper_rightLegBack;
static struct ModelVertex creeper_vertices[MODEL_BOX_VERTICES * 6];
static struct ModelTex creeper_tex = { "creeper.png" };
static struct Model creeper_model  = { "creeper", creeper_vertices, &creeper_tex };

static void CreeperModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,18,-4, 4,26,4),
		BoxDesc_Rot(0,18,0),
	};
	static struct BoxDesc torso = {
		BoxDesc_Tex(16,16),
		BoxDesc_Box(-4,6,-2, 4,18,2),
		BoxDesc_Rot(0,6,0),
	}; 
	static struct BoxDesc lFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-4,0,-6, 0,6,-2),
		BoxDesc_Rot(0,6,-2),
	}; 
	static struct BoxDesc rFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,-6, 4,6,-2),
		BoxDesc_Rot(0,6,-2),
	}; 
	static struct BoxDesc lBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-4,0,2, 0,6,6),
		BoxDesc_Rot(0,6,2),
	}; 
	static struct BoxDesc rBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,2, 4,6,6),
		BoxDesc_Rot(0,6,2),
	};
	
	BoxDesc_BuildBox(&creeper_head,  &head);
	BoxDesc_BuildBox(&creeper_torso, &torso);
	BoxDesc_BuildBox(&creeper_leftLegFront,  &lFront);
	BoxDesc_BuildBox(&creeper_rightLegFront, &rFront);
	BoxDesc_BuildBox(&creeper_leftLegBack,   &lBack);
	BoxDesc_BuildBox(&creeper_rightLegBack,  &rBack);
}

static float CreeperModel_GetEyeY(struct Entity* entity)   { return 22.0f/16.0f; }
static void CreeperModel_GetCollisionSize(Vector3* size)   { Model_RetSize(8.0f,26.0f,8.0f); }
static void CreeperModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-4,0,-6, 4,26,6); }

static void CreeperModel_DrawModel(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &creeper_head, true);

	Model_DrawPart(&creeper_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &creeper_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &creeper_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &creeper_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &creeper_rightLegBack,  false);
	Model_UpdateVB();
}

static struct Model* CreeperModel_GetInstance(void) {
	Model_Init(&creeper_model);
	Model_SetPointers(creeper_model, CreeperModel);
	creeper_model.NameYOffset = 1.7f;
	return &creeper_model;
}


/*########################################################################################################################*
*----------------------------------------------------------PigModel-------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart pig_head, pig_torso, pig_leftLegFront, pig_rightLegFront;
static struct ModelPart pig_leftLegBack, pig_rightLegBack;
static struct ModelVertex pig_vertices[MODEL_BOX_VERTICES * 6];
static struct ModelTex pig_tex = { "pig.png" };
static struct Model pig_model  = { "pig", pig_vertices, &pig_tex };

static void PigModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,8,-14, 4,16,-6),
		BoxDesc_Rot(0,12,-6),
	}; 
	static struct BoxDesc torso = {
		BoxDesc_Tex(28,8),
		BoxDesc_Box(-5,6,-8, 5,14,8),
		BoxDesc_Rot(0,6,0),
	}; 
	static struct BoxDesc lFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,-7, -1,6,-3),
		BoxDesc_Rot(0,6,-5),
	}; 
	static struct BoxDesc rFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,-7, 5,6,-3),
		BoxDesc_Rot(0,6,-5),
	}; 
	static struct BoxDesc lBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,5, -1,6,9),
		BoxDesc_Rot(0,6,7),
	}; 
	static struct BoxDesc rBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,5, 5,6,9),
		BoxDesc_Rot(0,6,7),
	}; 
	
	BoxDesc_BuildBox(&pig_head,          &head);
	BoxDesc_BuildRotatedBox(&pig_torso,  &torso);
	BoxDesc_BuildBox(&pig_leftLegFront,  &lFront);
	BoxDesc_BuildBox(&pig_rightLegFront, &rFront);
	BoxDesc_BuildBox(&pig_leftLegBack,   &lBack);
	BoxDesc_BuildBox(&pig_rightLegBack,  &rBack);
}

static float PigModel_GetEyeY(struct Entity* entity)   { return 12.0f/16.0f; }
static void PigModel_GetCollisionSize(Vector3* size)   { Model_RetSize(14.0f,14.0f,14.0f); }
static void PigModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-5,0,-14, 5,16,9); }

static void PigModel_DrawModel(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &pig_head, true);

	Model_DrawPart(&pig_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &pig_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &pig_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &pig_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &pig_rightLegBack,  false);
	Model_UpdateVB();
}

static struct Model* PigModel_GetInstance(void) {
	Model_Init(&pig_model);
	Model_SetPointers(pig_model, PigModel);
	pig_model.NameYOffset = 1.075f;
	return &pig_model;
}


/*########################################################################################################################*
*---------------------------------------------------------SheepModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart sheep_head, sheep_torso, sheep_leftLegFront;
static struct ModelPart sheep_rightLegFront, sheep_leftLegBack, sheep_rightLegBack;
static struct ModelPart fur_head, fur_torso, fur_leftLegFront, fur_rightLegFront;
static struct ModelPart fur_leftLegBack, fur_rightLegBack;
static struct ModelVertex sheep_vertices[MODEL_BOX_VERTICES * 6 * 2];
static struct ModelTex sheep_tex = { "sheep.png" };
static struct ModelTex fur_tex   = { "sheep_fur.png" };
static struct Model sheep_model  = { "sheep", sheep_vertices, &sheep_tex };
static struct Model nofur_model  = { "sheep_nofur", sheep_vertices, &sheep_tex };

static void SheepModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-3,16,-14, 3,22,-6),
		BoxDesc_Rot(0,18,-8),
	};
	static struct BoxDesc torso = {
		BoxDesc_Tex(28,8),
		BoxDesc_Box(-4,12,-8, 4,18,8),
		BoxDesc_Rot(0,12,0),
	}; 	
	static struct BoxDesc lFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,-7, -1,12,-3),
		BoxDesc_Rot(0,12,-5),
	}; 
	static struct BoxDesc rFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,-7, 5,12,-3),
		BoxDesc_Rot(0,12,-5),
	}; 
	static struct BoxDesc lBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,5, -1,12,9),
		BoxDesc_Rot(0,12,7),
	}; 
	static struct BoxDesc rBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,5, 5,12,9),
		BoxDesc_Rot(0,12,7),
	};

	static struct BoxDesc fHead = {
		BoxDesc_Tex(0,0),
		BoxDesc_Dims(-3,16,-12, 3,22,-6),
		BoxDesc_Bounds(-3.5f,15.5f,-12.5f, 3.5f,22.5f,-5.5f),
		BoxDesc_Rot(0,18,-8),
	}; 
	static struct BoxDesc fTorso = {
		BoxDesc_Tex(28,8),
		BoxDesc_Dims(-4,12,-8, 4,18,8),
		BoxDesc_Bounds(-6.0f,10.5f,-10.0f, 6.0f,19.5f,10.0f),
		BoxDesc_Rot(0,12,0),
	};
	static struct BoxDesc flFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(-5,6,-7, -1,12,-3),
		BoxDesc_Bounds(-5.5f,5.5f,-7.5f, -0.5f,12.5f,-2.5f),
		BoxDesc_Rot(0,12,-5),
	};
	static struct BoxDesc frFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(1,6,-7, 5,12,-3),
		BoxDesc_Bounds(0.5f,5.5f,-7.5f, 5.5f,12.5f,-2.5f),
		BoxDesc_Rot(0,12,-5),
	};
	static struct BoxDesc flBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(-5,6,5, -1,12,9),
		BoxDesc_Bounds(-5.5f,5.5f,4.5f, -0.5f,12.5f,9.5f),
		BoxDesc_Rot(0,12,7),
	}; 
	static struct BoxDesc frBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(1,6,5, 5,12,9),
		BoxDesc_Bounds(0.5f,5.5f,4.5f, 5.5f,12.5f,9.5f),
		BoxDesc_Rot(0,12,7),
	}; 

	BoxDesc_BuildBox(&sheep_head,          &head);
	BoxDesc_BuildRotatedBox(&sheep_torso,  &torso);
	BoxDesc_BuildBox(&sheep_leftLegFront,  &lFront);
	BoxDesc_BuildBox(&sheep_rightLegFront, &rFront);
	BoxDesc_BuildBox(&sheep_leftLegBack,   &lBack);
	BoxDesc_BuildBox(&sheep_rightLegBack,  &rBack);
	
	BoxDesc_BuildBox(&fur_head,          &fHead);
	BoxDesc_BuildRotatedBox(&fur_torso,  &fTorso);
	BoxDesc_BuildBox(&fur_leftLegFront,  &flFront);
	BoxDesc_BuildBox(&fur_rightLegFront, &frFront);
	BoxDesc_BuildBox(&fur_leftLegBack,   &flBack);
	BoxDesc_BuildBox(&fur_rightLegBack,  &frBack);
}

static float SheepModel_GetEyeY(struct Entity* entity)   { return 20.0f/16.0f; }
static void SheepModel_GetCollisionSize(Vector3* size)   { Model_RetSize(10.0f,20.0f,10.0f); }
static void SheepModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-6,0,-13, 6,23,10); }

static void NoFurModel_DrawModel(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &sheep_head, true);

	Model_DrawPart(&sheep_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &sheep_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &sheep_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &sheep_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &sheep_rightLegBack,  false);
	Model_UpdateVB();
}

static void SheepModel_DrawModel(struct Entity* entity) {
	NoFurModel_DrawModel(entity);
	Gfx_BindTexture(fur_tex.TexID);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &fur_head, true);

	Model_DrawPart(&fur_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &fur_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &fur_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &fur_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &fur_rightLegBack,  false);
	Model_UpdateVB();
}

static struct Model* SheepModel_GetInstance(void) {
	Model_Init(&sheep_model);
	Model_SetPointers(sheep_model, SheepModel);
	sheep_model.NameYOffset = 1.48125f;
	return &sheep_model;
}

static struct Model* NoFurModel_GetInstance(void) {
	Model_Init(&nofur_model);
	Model_SetPointers(nofur_model, SheepModel);
	nofur_model.DrawModel   = NoFurModel_DrawModel;
	nofur_model.NameYOffset = 1.48125f;
	return &nofur_model;
}


/*########################################################################################################################*
*-------------------------------------------------------SkeletonModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart skeleton_head, skeleton_torso, skeleton_leftLeg;
static struct ModelPart skeleton_rightLeg, skeleton_leftArm, skeleton_rightArm;
static struct ModelVertex skeleton_vertices[MODEL_BOX_VERTICES * 6];
static struct ModelTex skeleton_tex = { "skeleton.png" };
static struct Model skeleton_model  = { "skeleton", skeleton_vertices, &skeleton_tex };

static void SkeletonModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,24,-4, 4,32,4),
		BoxDesc_Rot(0,24,0),
	};
	static struct BoxDesc torso = {
		BoxDesc_Tex(16,16),
		BoxDesc_Box(-4,12,-2, 4,24,2),
		BoxDesc_Rot(0,12,0),
	}; 
	static struct BoxDesc lLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-1,0,-1, -3,12,1),
		BoxDesc_Rot(0,12,0),
	}; 
	static struct BoxDesc rLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,-1, 3,12,1),
		BoxDesc_Rot(0,12,0),
	}; 
	static struct BoxDesc lArm = {
		BoxDesc_Tex(40,16),
		BoxDesc_Box(-4,12,-1, -6,24,1),
		BoxDesc_Rot(-5,23,0),
	}; 
	static struct BoxDesc rArm = {
		BoxDesc_Tex(40,16),
		BoxDesc_Box(4,12,-1, 6,24,1),
		BoxDesc_Rot(5,23,0),
	}; 

	BoxDesc_BuildBox(&skeleton_head,  &head);
	BoxDesc_BuildBox(&skeleton_torso, &torso);
	BoxDesc_BuildBox(&skeleton_leftLeg,  &lLeg);
	BoxDesc_BuildBox(&skeleton_rightLeg, &rLeg);
	BoxDesc_BuildBox(&skeleton_leftArm,  &lArm);
	BoxDesc_BuildBox(&skeleton_rightArm, &rArm);
}

static float SkeletonModel_GetEyeY(struct Entity* entity)   { return 26.0f/16.0f; }
static void SkeletonModel_GetCollisionSize(Vector3* size)   { Model_RetSize(8.0f,28.1f,8.0f); }
static void SkeletonModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-4,0,-4, 4,32,4); }

static void SkeletonModel_DrawModel(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &skeleton_head, true);

	Model_DrawPart(&skeleton_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0,                      &skeleton_leftLeg,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0,                      &skeleton_rightLeg, false);
	Model_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.LeftArmZ,  &skeleton_leftArm,  false);
	Model_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.RightArmZ, &skeleton_rightArm, false);
	Model_UpdateVB();
}

static void SkeletonModel_DrawArm(struct Entity* entity) {
	Model_DrawArmPart(&skeleton_rightArm);
	Model_UpdateVB();
}

static struct Model* SkeletonModel_GetInstance(void) {
	Model_Init(&skeleton_model);
	Model_SetPointers(skeleton_model, SkeletonModel);
	skeleton_model.DrawArm  = SkeletonModel_DrawArm;
	skeleton_model.armX = 5;
	skeleton_model.NameYOffset = 2.075f;
	return &skeleton_model;
}


/*########################################################################################################################*
*--------------------------------------------------------SpiderModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart spider_head, spider_link, spider_end;
static struct ModelPart spider_leftLeg, spider_rightLeg;
static struct ModelVertex spider_vertices[MODEL_BOX_VERTICES * 5];
static struct ModelTex spider_tex = { "spider.png" };
static struct Model spider_model  = { "spider", spider_vertices, &spider_tex };

static void SpiderModel_CreateParts(void) {
	static struct BoxDesc head = {
		BoxDesc_Tex(32,4),
		BoxDesc_Box(-4,4,-11, 4,12,-3),
		BoxDesc_Rot(0,8,-3),
	};
	static struct BoxDesc link = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-3,5,3, 3,11,-3),
		BoxDesc_Rot(0,5,0),
	}; 
	static struct BoxDesc end = {
		BoxDesc_Tex(0,12),
		BoxDesc_Box(-5,4,3, 5,12,15),
		BoxDesc_Rot(0,4,9),
	}; 
	static struct BoxDesc lLeg = {
		BoxDesc_Tex(18,0),
		BoxDesc_Box(-19,7,-1, -3,9,1),
		BoxDesc_Rot(-3,8,0),
	}; 
	static struct BoxDesc rLeg = {
		BoxDesc_Tex(18,0),
		BoxDesc_Box(3,7,-1, 19,9,1),
		BoxDesc_Rot(3,8,0),
	};
	
	BoxDesc_BuildBox(&spider_head, &head);
	BoxDesc_BuildBox(&spider_link, &link);
	BoxDesc_BuildBox(&spider_end,  &end);
	BoxDesc_BuildBox(&spider_leftLeg,  &lLeg);
	BoxDesc_BuildBox(&spider_rightLeg, &rLeg);
}

static float SpiderModel_GetEyeY(struct Entity* entity)   { return 8.0f/16.0f; }
static void SpiderModel_GetCollisionSize(Vector3* size)   { Model_RetSize(15.0f,12.0f,15.0f); }
static void SpiderModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-5,0,-11, 5,12,15); }

#define quarterPi (MATH_PI / 4.0f)
#define eighthPi  (MATH_PI / 8.0f)

static void SpiderModel_DrawModel(struct Entity* entity) {
	float rotX, rotY, rotZ;
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &spider_head, true);
	Model_DrawPart(&spider_link);
	Model_DrawPart(&spider_end);

	rotX = Math_SinF(entity->Anim.WalkTime)     * entity->Anim.Swing * MATH_PI;
	rotZ = Math_CosF(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 16.0f;
	rotY = Math_SinF(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 32.0f;
	Models.Rotation = ROTATE_ORDER_XZY;

	Model_DrawRotate(rotX,  quarterPi  + rotY, eighthPi + rotZ, &spider_leftLeg, false);
	Model_DrawRotate(-rotX,  eighthPi  + rotY, eighthPi + rotZ, &spider_leftLeg, false);
	Model_DrawRotate(rotX,  -eighthPi  - rotY, eighthPi - rotZ, &spider_leftLeg, false);
	Model_DrawRotate(-rotX, -quarterPi - rotY, eighthPi - rotZ, &spider_leftLeg, false);

	Model_DrawRotate(rotX, -quarterPi + rotY, -eighthPi + rotZ, &spider_rightLeg, false);
	Model_DrawRotate(-rotX, -eighthPi + rotY, -eighthPi + rotZ, &spider_rightLeg, false);
	Model_DrawRotate(rotX,   eighthPi - rotY, -eighthPi - rotZ, &spider_rightLeg, false);
	Model_DrawRotate(-rotX, quarterPi - rotY, -eighthPi - rotZ, &spider_rightLeg, false);

	Models.Rotation = ROTATE_ORDER_ZYX;
	Model_UpdateVB();
}

static struct Model* SpiderModel_GetInstance(void) {
	Model_Init(&spider_model);
	Model_SetPointers(spider_model, SpiderModel);
	spider_model.NameYOffset = 1.0125f;
	return &spider_model;
}


/*########################################################################################################################*
*--------------------------------------------------------ZombieModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelTex zombie_tex = { "zombie.png" };
static struct Model zombie_model  = { "zombie", human_vertices, &zombie_tex };

static void ZombieModel_CreateParts(void) { }
static float ZombieModel_GetEyeY(struct Entity* entity)   { return 26.0f/16.0f; }
static void ZombieModel_GetCollisionSize(Vector3* size)   { Model_RetSize(8.6f,28.1f,8.6f); }
static void ZombieModel_GetPickingBounds(struct AABB* bb) { Model_RetAABB(-4,0,-4, 4,32,4); }

static void ZombieModel_DrawModel(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &human_set.Head, true);

	Model_DrawPart(&human_set.Torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0,                      &human_set.Limbs[0].LeftLeg,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0,                      &human_set.Limbs[0].RightLeg, false);
	Model_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.LeftArmZ,  &human_set.Limbs[0].LeftArm,  false);
	Model_DrawRotate(90.0f * MATH_DEG2RAD,   0, entity->Anim.RightArmZ, &human_set.Limbs[0].RightArm, false);

	Model_DrawRotate(-entity->HeadX * MATH_DEG2RAD, 0, 0, &human_set.Hat, true);
	Model_UpdateVB();
}

static void ZombieModel_DrawArm(struct Entity* entity) {
	Model_DrawArmPart(&human_set.Limbs[0].RightArm);
	Model_UpdateVB();
}

static struct Model* ZombieModel_GetInstance(void) {
	Model_Init(&zombie_model);
	Model_SetPointers(zombie_model, ZombieModel);
	zombie_model.DrawArm  = ZombieModel_DrawArm;
	zombie_model.NameYOffset = 2.075f;
	return &zombie_model;
}


/*########################################################################################################################*
*---------------------------------------------------------BlockModel------------------------------------------------------*
*#########################################################################################################################*/
static struct Model block_model = { "block", NULL, &human_tex };
static BlockID bModel_block = BLOCK_AIR;
static Vector3 bModel_minBB, bModel_maxBB;
static int bModel_lastTexIndex = -1, bModel_texIndex;

static void BlockModel_CreateParts(void) { }

static float BlockModel_GetEyeY(struct Entity* entity) {
	BlockID block = entity->ModelBlock;
	float minY = Blocks.MinBB[block].Y;
	float maxY = Blocks.MaxBB[block].Y;
	return block == BLOCK_AIR ? 1 : (minY + maxY) / 2.0f;
}

static void BlockModel_GetCollisionSize(Vector3* size) {
	static Vector3 shrink = { 0.75f/16.0f, 0.75f/16.0f, 0.75f/16.0f };
	Vector3_Sub(size, &bModel_maxBB, &bModel_minBB);

	/* to fit slightly inside */
	Vector3_SubBy(size, &shrink);
	/* fix for 0 size blocks */
	size->X = max(size->X, 0.125f/16.0f);
	size->Y = max(size->Y, 0.125f/16.0f);
	size->Z = max(size->Z, 0.125f/16.0f);
}

static void BlockModel_GetPickingBounds(struct AABB* bb) {
	static Vector3 offset = { -0.5f, 0.0f, -0.5f };
	Vector3_Add(&bb->Min, &bModel_minBB, &offset);
	Vector3_Add(&bb->Max, &bModel_maxBB, &offset);
}

static void BlockModel_RecalcProperties(struct Entity* p) {
	BlockID block = p->ModelBlock;
	float height;

	if (Blocks.Draw[block] == DRAW_GAS) {
		bModel_minBB = Vector3_Zero();
		bModel_maxBB = Vector3_One();
		height = 1.0f;
	} else {
		bModel_minBB = Blocks.MinBB[block];
		bModel_maxBB = Blocks.MaxBB[block];
		height = bModel_maxBB.Y - bModel_minBB.Y;
	}
	block_model.NameYOffset = height + 0.075f;
}

static void BlockModel_Flush(void) {
	if (bModel_lastTexIndex != -1) {
		Gfx_BindTexture(Atlas1D_TexIds[bModel_lastTexIndex]);
		Model_UpdateVB();
	}

	bModel_lastTexIndex = bModel_texIndex;
	block_model.index = 0;
}

#define BlockModel_FlushIfNotSame if (bModel_lastTexIndex != bModel_texIndex) { BlockModel_Flush(); }
static TextureLoc BlockModel_GetTex(Face face, VertexP3fT2fC4b** ptr) {
	TextureLoc texLoc   = Block_GetTex(bModel_block, face);
	bModel_texIndex = Atlas1D_Index(texLoc);
	BlockModel_FlushIfNotSame;

	/* Need to reload ptr, in case was flushed */
	*ptr = &Model_Vertices[block_model.index];
	block_model.index += 4;
	return texLoc;
}

static void BlockModel_SpriteZQuad(bool firstPart, bool mirror) {
	VertexP3fT2fC4b* ptr, v;
	PackedCol col;
	float xz1, xz2;
	TextureLoc loc = Block_GetTex(bModel_block, FACE_ZMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &bModel_texIndex);

	BlockModel_FlushIfNotSame;
	col = Models.Cols[0];
	Block_Tint(col, bModel_block);

	xz1 = 0.0f; xz2 = 0.0f;
	if (firstPart) { /* Need to break into two quads for when drawing a sprite model in hand. */
		if (mirror) { rec.U1 = 0.5f; xz1 = -5.5f/16.0f; }
		else {        rec.U2 = 0.5f; xz2 = -5.5f/16.0f; }
	} else {
		if (mirror) { rec.U2 = 0.5f; xz2 =  5.5f/16.0f; }
		else {        rec.U1 = 0.5f; xz1 =  5.5f/16.0f; }
	}

	ptr   = &Model_Vertices[block_model.index];
	v.Col = col;

	v.X = xz1; v.Y = 0.0f; v.Z = xz1; v.U = rec.U2; v.V = rec.V2; *ptr++ = v;
	           v.Y = 1.0f;                          v.V = rec.V1; *ptr++ = v;
	v.X = xz2;             v.Z = xz2; v.U = rec.U1;               *ptr++ = v;
	           v.Y = 0.0f;                          v.V = rec.V2; *ptr++ = v;
	block_model.index += 4;
}

static void BlockModel_SpriteXQuad(bool firstPart, bool mirror) {
	VertexP3fT2fC4b* ptr, v;
	PackedCol col;
	float x1, x2, z1, z2;
	TextureLoc loc = Block_GetTex(bModel_block, FACE_XMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &bModel_texIndex);

	BlockModel_FlushIfNotSame;
	col = Models.Cols[0];
	Block_Tint(col, bModel_block);

	x1 = 0.0f; x2 = 0.0f; z1 = 0.0f; z2 = 0.0f;
	if (firstPart) {
		if (mirror) { rec.U2 = 0.5f; x2 = -5.5f/16.0f; z2 =  5.5f/16.0f; }
		else {        rec.U1 = 0.5f; x1 = -5.5f/16.0f; z1 =  5.5f/16.0f; }
	} else {
		if (mirror) { rec.U1 = 0.5f; x1 =  5.5f/16.0f; z1 = -5.5f/16.0f; }
		else {        rec.U2 = 0.5f; x2 =  5.5f/16.0f; z2 = -5.5f/16.0f; }
	}

	ptr   = &Model_Vertices[block_model.index];
	v.Col = col;

	v.X = x1; v.Y = 0.0f; v.Z = z1; v.U = rec.U2; v.V = rec.V2; *ptr++ = v;
	          v.Y = 1.0f;                         v.V = rec.V1; *ptr++ = v;
	v.X = x2;             v.Z = z2; v.U = rec.U1;               *ptr++ = v;
	          v.Y = 0.0f;                         v.V = rec.V2; *ptr++ = v;
	block_model.index += 4;
}

static void BlockModel_DrawParts(bool sprite) {
	Vector3 min, max;
	TextureLoc loc;
	VertexP3fT2fC4b* ptr = NULL;

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
		Drawer.MinBB = Blocks.MinBB[bModel_block]; Drawer.MinBB.Y = 1.0f - Drawer.MinBB.Y;
		Drawer.MaxBB = Blocks.MaxBB[bModel_block]; Drawer.MaxBB.Y = 1.0f - Drawer.MaxBB.Y;
		Drawer.Tinted  = Blocks.Tinted[bModel_block];
		Drawer.TintCol = Blocks.FogCol[bModel_block];

		min = Blocks.RenderMinBB[bModel_block];
		max = Blocks.RenderMaxBB[bModel_block];
		Drawer.X1 = min.X - 0.5f; Drawer.Y1 = min.Y; Drawer.Z1 = min.Z - 0.5f;
		Drawer.X2 = max.X - 0.5f; Drawer.Y2 = max.Y; Drawer.Z2 = max.Z - 0.5f;		

		loc = BlockModel_GetTex(FACE_YMIN, &ptr); Drawer_YMin(1, Models.Cols[1], loc, &ptr);
		loc = BlockModel_GetTex(FACE_ZMIN, &ptr); Drawer_ZMin(1, Models.Cols[3], loc, &ptr);
		loc = BlockModel_GetTex(FACE_XMAX, &ptr); Drawer_XMax(1, Models.Cols[5], loc, &ptr);
		loc = BlockModel_GetTex(FACE_ZMAX, &ptr); Drawer_ZMax(1, Models.Cols[2], loc, &ptr);
		loc = BlockModel_GetTex(FACE_XMIN, &ptr); Drawer_XMin(1, Models.Cols[4], loc, &ptr);
		loc = BlockModel_GetTex(FACE_YMAX, &ptr); Drawer_YMax(1, Models.Cols[0], loc, &ptr);
	}
}

static void BlockModel_DrawModel(struct Entity* p) {
	PackedCol white = PACKEDCOL_WHITE;
	bool sprite;
	int i;

	bModel_block = p->ModelBlock;
	BlockModel_RecalcProperties(p);
	if (Blocks.Draw[bModel_block] == DRAW_GAS) return;

	if (Blocks.FullBright[bModel_block]) {
		for (i = 0; i < FACE_COUNT; i++) {
			Models.Cols[i] = white;
		}
	}
	sprite = Blocks.Draw[bModel_block] == DRAW_SPRITE;

	bModel_lastTexIndex = -1;	
	BlockModel_DrawParts(sprite);
	if (!block_model.index) return;

	if (sprite) Gfx_SetFaceCulling(true);
	bModel_lastTexIndex = bModel_texIndex;
	BlockModel_Flush();
	if (sprite) Gfx_SetFaceCulling(false);
}

static struct Model* BlockModel_GetInstance(void) {
	Model_Init(&block_model);
	Model_SetPointers(block_model, BlockModel);
	block_model.Bobbing  = false;
	block_model.UsesSkin = false;
	block_model.Pushes   = false;
	block_model.RecalcProperties = BlockModel_RecalcProperties;
	return &block_model;
}


/*########################################################################################################################*
*-------------------------------------------------------Model component---------------------------------------------------*
*#########################################################################################################################*/
static void Model_RegisterDefaultModels(void) {
	Model_RegisterTexture(&human_tex);
	Model_RegisterTexture(&chicken_tex);
	Model_RegisterTexture(&creeper_tex);
	Model_RegisterTexture(&pig_tex);
	Model_RegisterTexture(&sheep_tex);
	Model_RegisterTexture(&fur_tex);
	Model_RegisterTexture(&skeleton_tex);
	Model_RegisterTexture(&spider_tex);
	Model_RegisterTexture(&zombie_tex);

	Model_Register(HumanoidModel_GetInstance());
	Model_Make(&human_model);
	Human_ModelPtr = &human_model;

	Model_Register(ChickenModel_GetInstance());
	Model_Register(CreeperModel_GetInstance());
	Model_Register(PigModel_GetInstance());
	Model_Register(SheepModel_GetInstance());
	Model_Register(NoFurModel_GetInstance());
	Model_Register(SkeletonModel_GetInstance());
	Model_Register(SpiderModel_GetInstance());
	Model_Register(ZombieModel_GetInstance());

	Model_Register(BlockModel_GetInstance());
	Model_Register(ChibiModel_GetInstance());
	Model_Register(HeadModel_GetInstance());
	Model_Register(SittingModel_GetInstance());
	Model_Register(CorpseModel_GetInstance());
}

void Models_Init(void) {
	Model_RegisterDefaultModels();
	Models_ContextRecreated(NULL);

	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, Models_TextureChanged);
	Event_RegisterVoid(&GfxEvents.ContextLost,      NULL, Models_ContextLost);
	Event_RegisterVoid(&GfxEvents.ContextRecreated, NULL, Models_ContextRecreated);
}

void Models_Free(void) {
	struct ModelTex* tex;

	for (tex = textures_head; tex; tex = tex->Next) {
		Gfx_DeleteTexture(&tex->TexID);
	}
	Models_ContextLost(NULL);

	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, Models_TextureChanged);
	Event_UnregisterVoid(&GfxEvents.ContextLost,      NULL, Models_ContextLost);
	Event_UnregisterVoid(&GfxEvents.ContextRecreated, NULL, Models_ContextRecreated);
}

struct IGameComponent Models_Component = {
	Models_Init, /* Init  */
	Models_Free, /* Free  */
};
