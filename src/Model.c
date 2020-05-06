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
#include "Options.h"

struct _ModelsData Models;

#define UV_POS_MASK ((cc_uint16)0x7FFF)
#define UV_MAX ((cc_uint16)0x8000)
#define UV_MAX_SHIFT 15
#define AABB_Width(bb)  ((bb)->Max.X - (bb)->Min.X)
#define AABB_Height(bb) ((bb)->Max.Y - (bb)->Min.Y)
#define AABB_Length(bb) ((bb)->Max.Z - (bb)->Min.Z)


/*########################################################################################################################*
*------------------------------------------------------------Model--------------------------------------------------------*
*#########################################################################################################################*/
static void Model_GetTransform(struct Entity* entity, Vec3 pos, struct Matrix* m) {
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}
static void Model_NullFunc(struct Entity* entity) { }
static void Model_NoParts(void) { }

void Model_Init(struct Model* model) {
	model->bobbing  = true;
	model->usesSkin = true;
	model->calcHumanAnims = false;
	model->usesHumanSkin  = false;
	model->pushes = true;

	model->gravity     = 0.08f;
	Vec3_Set(model->drag,           0.91f, 0.98f, 0.91f);
	Vec3_Set(model->groundFriction, 0.6f,   1.0f,  0.6f);

	model->maxScale    = 2.0f;
	model->shadowScale = 1.0f;
	model->nameScale   = 1.0f;
	model->armX = 6; model->armY = 12;

	model->GetTransform     = Model_GetTransform;
	model->DrawArm          = Model_NullFunc;
}

cc_bool Model_ShouldRender(struct Entity* entity) {
	Vec3 pos = entity->Position;
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
	Vec3 pos     = entity->Position;
	struct AABB* bb = &entity->ModelAABB;
	Vec3 camPos  = Camera.CurrentPos;
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
	Vec3 pos = entity->Position;
	if (model->bobbing) pos.Y += entity->Anim.BobbingModel;

	Model_SetupState(model, entity);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);

	model->GetTransform(entity, pos, &entity->Transform);
	Matrix_Mul(&m, &entity->Transform, &Gfx.View);

	Gfx_LoadMatrix(MATRIX_VIEW, &m);
	model->Draw(entity);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
}

void Model_SetupState(struct Model* model, struct Entity* entity) {
	PackedCol col;
	cc_bool _64x64;
	float yawDelta;

	model->index = 0;
	col = entity->VTABLE->GetCol(entity);

	_64x64  = entity->SkinType != SKIN_64x32;
	/* only apply when using humanoid skins */
	_64x64 &= model->usesHumanSkin || entity->MobTextureId;

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
	yawDelta = entity->Yaw - entity->RotY;

	Models.cosHead = (float)Math_Cos(yawDelta * MATH_DEG2RAD);
	Models.sinHead = (float)Math_Sin(yawDelta * MATH_DEG2RAD);
	Models.Active  = model;
}

void Model_UpdateVB(void) {
	struct Model* model = Models.Active;
	Gfx_UpdateDynamicVb_IndexedTris(Models.Vb, Models.Vertices, model->index);
	model->index = 0;
}

void Model_ApplyTexture(struct Entity* entity) {
	struct Model* model = Models.Active;
	struct ModelTex* data;
	GfxResourceID tex;
	cc_bool _64x64;

	tex = model->usesHumanSkin ? entity->TextureId : entity->MobTextureId;
	if (tex) {
		Models.skinType = entity->SkinType;
	} else {
		data = model->defaultTex;
		tex  = data->texID;
		Models.skinType = data->skinType;
	}

	Gfx_BindTexture(tex);
	_64x64 = Models.skinType != SKIN_64x32;

	Models.uScale = entity->uScale * 0.015625f;
	Models.vScale = entity->vScale * (_64x64 ? 0.015625f : 0.03125f);
}

void Model_DrawPart(struct ModelPart* part) {
	struct Model* model     = Models.Active;
	struct ModelVertex* src = &model->vertices[part->offset];
	VertexP3fT2fC4b* dst    = &Models.Vertices[model->index];

	struct ModelVertex v;
	int i, count = part->count;

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

void Model_DrawRotate(float angleX, float angleY, float angleZ, struct ModelPart* part, cc_bool head) {
	struct Model* model     = Models.Active;
	struct ModelVertex* src = &model->vertices[part->offset];
	VertexP3fT2fC4b* dst    = &Models.Vertices[model->index];

	float cosX = (float)Math_Cos(-angleX), sinX = (float)Math_Sin(-angleX);
	float cosY = (float)Math_Cos(-angleY), sinY = (float)Math_Sin(-angleY);
	float cosZ = (float)Math_Cos(-angleZ), sinZ = (float)Math_Sin(-angleZ);
	float t, x = part->rotX, y = part->rotY, z = part->rotZ;
	
	struct ModelVertex v;
	int i, count = part->count;

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
	Vec3 pos = entity->Position;
	if (model->bobbing) pos.Y += entity->Anim.BobbingModel;

	Model_SetupState(model, entity);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Model_ApplyTexture(entity);

	if (Models.ClassicArms) {
		/* TODO: Position's not quite right. */
		/* Matrix_Translate(out m, -armX / 16f + 0.2f, -armY / 16f - 0.20f, 0); */
		/* is better, but that breaks the dig animation */
		Matrix_Translate(&translate, -model->armX / 16.0f,         -model->armY / 16.0f - 0.10f, 0);
	} else {
		Matrix_Translate(&translate, -model->armX / 16.0f + 0.10f, -model->armY / 16.0f - 0.26f, 0);
	}

	Entity_GetTransform(entity, pos, entity->ModelScale, &m);
	Matrix_Mul(&m, &m,         &Gfx.View);
	Matrix_Mul(&m, &translate, &m);

	Gfx_LoadMatrix(MATRIX_VIEW, &m);
	Models.Rotation = ROTATE_ORDER_YZX;
	model->DrawArm(entity);
	Models.Rotation = ROTATE_ORDER_ZYX;
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
}

void Model_DrawArmPart(struct ModelPart* part) {
	struct Model* model  = Models.Active;
	struct ModelPart arm = *part;
	arm.rotX = model->armX / 16.0f; 
	arm.rotY = (model->armY + model->armY / 2) / 16.0f;

	if (Models.ClassicArms) {
		Model_DrawRotate(0, -90 * MATH_DEG2RAD, 120 * MATH_DEG2RAD, &arm, false);
	} else {
		Model_DrawRotate(-20 * MATH_DEG2RAD, -70 * MATH_DEG2RAD, 135 * MATH_DEG2RAD, &arm, false);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------BoxDesc--------------------------------------------------------*
*#########################################################################################################################*/
void BoxDesc_BuildBox(struct ModelPart* part, const struct BoxDesc* desc) {
	int sidesW = desc->sizeZ, bodyW = desc->sizeX, bodyH = desc->sizeY;
	float x1 = desc->x1, y1 = desc->y1, z1 = desc->z1;
	float x2 = desc->x2, y2 = desc->y2, z2 = desc->z2;
	int x = desc->texX, y = desc->texY;
	struct Model* m = Models.Active;

	BoxDesc_YQuad(m, x + sidesW,                  y,          bodyW, sidesW, x1, x2, z2, z1, y2, true);  /* top */
	BoxDesc_YQuad(m, x + sidesW + bodyW,          y,          bodyW, sidesW, x2, x1, z2, z1, y1, false); /* bottom */
	BoxDesc_ZQuad(m, x + sidesW,                  y + sidesW, bodyW,  bodyH, x1, x2, y1, y2, z1, true);  /* front */
	BoxDesc_ZQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW,  bodyH, x2, x1, y1, y2, z2, true);  /* back */
	BoxDesc_XQuad(m, x,                           y + sidesW, sidesW, bodyH, z1, z2, y1, y2, x2, true);  /* left */
	BoxDesc_XQuad(m, x + sidesW + bodyW,          y + sidesW, sidesW, bodyH, z2, z1, y1, y2, x1, true);  /* right */

	ModelPart_Init(part, m->index - MODEL_BOX_VERTICES, MODEL_BOX_VERTICES,
		desc->rotX, desc->rotY, desc->rotZ);
}

void BoxDesc_BuildRotatedBox(struct ModelPart* part, const struct BoxDesc* desc) {
	int sidesW = desc->sizeY, bodyW = desc->sizeX, bodyH = desc->sizeZ;
	float x1 = desc->x1, y1 = desc->y1, z1 = desc->z1;
	float x2 = desc->x2, y2 = desc->y2, z2 = desc->z2;
	int x = desc->texX, y = desc->texY, i;
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
		desc->rotX, desc->rotY, desc->rotZ);
}


void BoxDesc_XQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float z1, float z2, float y1, float y2, float x, cc_bool swapU) {
	int u1 = texX, u2 = (texX + texWidth) | UV_MAX, tmp;
	if (swapU) { tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x, y1, z1, u1, (texY + texHeight) | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y2, z1, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y2, z2, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x, y1, z2, u2, (texY + texHeight) | UV_MAX); m->index++;
}

void BoxDesc_YQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float z1, float z2, float y, cc_bool swapU) {
	int u1 = texX, u2 = (texX + texWidth) | UV_MAX, tmp;
	if (swapU) { tmp = u1; u1 = u2; u2 = tmp; }

	ModelVertex_Init(&m->vertices[m->index], x1, y, z2, u1, (texY + texHeight) | UV_MAX); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x1, y, z1, u1, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y, z1, u2, texY); m->index++;
	ModelVertex_Init(&m->vertices[m->index], x2, y, z2, u2, (texY + texHeight) | UV_MAX); m->index++;
}

void BoxDesc_ZQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float y1, float y2, float z, cc_bool swapU) {
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
static struct Model* models_tail;
static struct ModelTex* textures_head;
static struct ModelTex* textures_tail;

#define Model_RetSize(x,y,z) static Vec3 P = { (x)/16.0f,(y)/16.0f,(z)/16.0f }; e->Size = P;
#define Model_RetAABB(x1,y1,z1, x2,y2,z2) static struct AABB BB = { (x1)/16.0f,(y1)/16.0f,(z1)/16.0f, (x2)/16.0f,(y2)/16.0f,(z2)/16.0f }; e->ModelAABB = BB;

static void Models_ContextLost(void* obj) {
	Gfx_DeleteDynamicVb(&Models.Vb);
}

static void Models_ContextRecreated(void* obj) {
	Models.Vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, Models.MaxVertices);
}

static void MakeModel(struct Model* model) {
	struct Model* active = Models.Active;
	Models.Active = model;
	model->MakeParts();

	model->initalised = true;
	model->index      = 0;
	Models.Active     = active;
}

struct Model* Model_Get(const String* name) {
	struct Model* model;

	for (model = models_head; model; model = model->next) {
		if (!String_CaselessEqualsConst(name, model->name)) continue;

		if (!model->initalised) MakeModel(model);
		return model;
	}
	return NULL;
}

struct ModelTex* Model_GetTexture(const String* name) {
	struct ModelTex* tex;

	for (tex = textures_head; tex; tex = tex->next) {
		if (String_CaselessEqualsConst(name, tex->name)) return tex;
	}
	return NULL;
}

void Model_Register(struct Model* model) {
	LinkedList_Add(model, models_head, models_tail);
}

void Model_RegisterTexture(struct ModelTex* tex) {
	LinkedList_Add(tex, textures_head, textures_tail);
}

static void Models_TextureChanged(void* obj, struct Stream* stream, const String* name) {
	struct ModelTex* tex;

	for (tex = textures_head; tex; tex = tex->next) {
		if (!String_CaselessEqualsConst(name, tex->name)) continue;

		Game_UpdateTexture(&tex->texID, stream, name, &tex->skinType);
		return;
	}
}


/*########################################################################################################################*
*---------------------------------------------------------HumanModel------------------------------------------------------*
*#########################################################################################################################*/
struct ModelLimbs {
	struct ModelPart leftLeg, rightLeg, leftArm, rightArm, leftLegLayer, rightLegLayer, leftArmLayer, rightArmLayer;
};
struct ModelSet {
	struct ModelPart head, torso, hat, torsoLayer;
	struct ModelLimbs limbs[3];
};

static void HumanModel_DrawCore(struct Entity* entity, struct ModelSet* model, cc_bool opaque) {
	struct ModelLimbs* set;
	int type;

	Model_ApplyTexture(entity);
	/* human model draws the body opaque so players can't have invisible skins */
	if (opaque) Gfx_SetAlphaTest(false);

	type = Models.skinType;
	set  = &model->limbs[type & 0x3];

	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &model->head, true);
	Model_DrawPart(&model->torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, entity->Anim.LeftLegZ,  &set->leftLeg,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, entity->Anim.RightLegZ, &set->rightLeg, false);

	Models.Rotation = ROTATE_ORDER_XZY;
	Model_DrawRotate(entity->Anim.LeftArmX,  0, entity->Anim.LeftArmZ,  &set->leftArm,  false);
	Model_DrawRotate(entity->Anim.RightArmX, 0, entity->Anim.RightArmZ, &set->rightArm, false);
	Models.Rotation = ROTATE_ORDER_ZYX;

	/* have to seperately draw these vertices without alpha testing */
	if (opaque) {
		Model_UpdateVB();
		Gfx_SetAlphaTest(true);
	}

	if (type != SKIN_64x32) {
		Model_DrawPart(&model->torsoLayer);
		Model_DrawRotate(entity->Anim.LeftLegX,  0, entity->Anim.LeftLegZ,  &set->leftLegLayer,  false);
		Model_DrawRotate(entity->Anim.RightLegX, 0, entity->Anim.RightLegZ, &set->rightLegLayer, false);

		Models.Rotation = ROTATE_ORDER_XZY;
		Model_DrawRotate(entity->Anim.LeftArmX,  0, entity->Anim.LeftArmZ,  &set->leftArmLayer,  false);
		Model_DrawRotate(entity->Anim.RightArmX, 0, entity->Anim.RightArmZ, &set->rightArmLayer, false);
		Models.Rotation = ROTATE_ORDER_ZYX;
	}
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &model->hat, true);
	Model_UpdateVB();
}

static void HumanModel_DrawArmCore(struct Entity* entity, struct ModelSet* model) {
	struct ModelLimbs* set;
	int type;

	type = Models.skinType;
	set  = &model->limbs[type & 0x3];

	Model_DrawArmPart(&set->rightArm);
	if (type != SKIN_64x32) Model_DrawArmPart(&set->rightArmLayer);
	Model_UpdateVB();
}


static struct ModelSet human_set;
static void HumanModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,24,-4, 4,32,4),
		BoxDesc_Rot(0,24,0),
	}; 
	static const struct BoxDesc torso = {
		BoxDesc_Tex(16,16),
		BoxDesc_Box(-4,12,-2, 4,24,2),
		BoxDesc_Rot(0,12,0),
	}; 
	static const struct BoxDesc hat = {
		BoxDesc_Tex(32,0),
		BoxDesc_Dims(-4,24,-4, 4,32,4),
		BoxDesc_Bounds(-4.5f,23.5f,-4.5f, 4.5f,32.5f,4.5f),
		BoxDesc_Rot(0,24,0),
	}; 
	static const struct BoxDesc torsoL = {
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
	static const struct BoxDesc lLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,-2, -4,12,2),
		BoxDesc_Rot(0,12,0),
	};
	static const struct BoxDesc rLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,-2, 4,12,2),
		BoxDesc_Rot(0,12,0),
	};

	static struct BoxDesc lArm64 = {
		BoxDesc_Tex(32,48),
		BoxDesc_Box(-8,12,-2, -4,24,2),
		BoxDesc_Rot(-5,22,0),
	};
	static const struct BoxDesc lLeg64 = {
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
	static const struct BoxDesc lLegL = {
		BoxDesc_Tex(0,48),
		BoxDesc_Dims(-4,0,-2, 0,12,2),
		BoxDesc_Bounds(-4.5f,-0.5f,-2.5f, 0.5f,12.5f,2.5f),
		BoxDesc_Rot(0,12,0),
	};
	static const struct BoxDesc rLegL = {
		BoxDesc_Tex(0,32),
		BoxDesc_Dims(0,0,-2, 4,12,2),
		BoxDesc_Bounds(-0.5f,-0.5f,-2.5f, 4.5f,12.5f,2.5f),
		BoxDesc_Rot(0,12,0),
	};

	struct ModelLimbs* set     = &human_set.limbs[0];
	struct ModelLimbs* set64   = &human_set.limbs[1];
	struct ModelLimbs* setSlim = &human_set.limbs[2];

	BoxDesc_BuildBox(&human_set.head,  &head);
	BoxDesc_BuildBox(&human_set.torso, &torso);
	BoxDesc_BuildBox(&human_set.hat,   &hat);
	BoxDesc_BuildBox(&human_set.torsoLayer, &torsoL);

	BoxDesc_BuildBox(&set->leftLeg,  &lLeg);
	BoxDesc_BuildBox(&set->rightLeg, &rLeg);
	BoxDesc_BuildBox(&set->leftArm,  &lArm);
	BoxDesc_BuildBox(&set->rightArm, &rArm);

	/* 64x64 arms */
	BoxDesc_BuildBox(&set64->leftLeg, &lLeg64);
	set64->rightLeg = set->rightLeg;
	BoxDesc_BuildBox(&set64->leftArm, &lArm64);
	set64->rightArm = set->rightArm;

	lArm64.sizeX -= 1; lArm64.x1 += 1.0f/16.0f;
	rArm.sizeX   -= 1; rArm.x2   -= 1.0f/16.0f;

	setSlim->leftLeg  = set64->leftLeg;
	setSlim->rightLeg = set64->rightLeg;
	BoxDesc_BuildBox(&setSlim->leftArm,  &lArm64);
	BoxDesc_BuildBox(&setSlim->rightArm, &rArm);

	/* 64x64 legs */
	BoxDesc_BuildBox(&set64->leftLegLayer,  &lLegL);
	BoxDesc_BuildBox(&set64->rightLegLayer, &rLegL);
	BoxDesc_BuildBox(&set64->leftArmLayer,  &lArmL);
	BoxDesc_BuildBox(&set64->rightArmLayer, &rArmL);

	lArmL.sizeX -= 1; lArmL.x1 += 1.0f/16.0f;
	rArmL.sizeX -= 1; rArmL.x2 -= 1.0f/16.0f;

	setSlim->leftLegLayer  = set64->leftLegLayer;
	setSlim->rightLegLayer = set64->rightLegLayer;
	BoxDesc_BuildBox(&setSlim->leftArmLayer,  &lArmL);
	BoxDesc_BuildBox(&setSlim->rightArmLayer, &rArmL);
}

static void HumanModel_Draw(struct Entity* entity) {
	HumanModel_DrawCore(entity, &human_set, true);
}

static void HumanModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArmCore(entity, &human_set);
}

static float HumanModel_GetNameY(struct Entity* e) { return 32.5f/16.0f; }
static float HumanModel_GetEyeY(struct Entity* e)  { return 26.0f/16.0f; }
static void HumanModel_GetSize(struct Entity* e)   { Model_RetSize(8.6f,28.1f,8.6f); }
static void HumanModel_GetBounds(struct Entity* e) { Model_RetAABB(-8,0,-4, 8,32,4); }

static struct ModelVertex human_vertices[MODEL_BOX_VERTICES * (7 + 7 + 4)];
static struct ModelTex human_tex = { "char.png" };
static struct Model  human_model = { 
	"humanoid", human_vertices, &human_tex,
	HumanModel_MakeParts, HumanModel_Draw,
	HumanModel_GetNameY,  HumanModel_GetEyeY,
	HumanModel_GetSize,   HumanModel_GetBounds,
};

static struct Model* HumanoidModel_GetInstance(void) {
	Model_Init(&human_model);
	human_model.DrawArm  = HumanModel_DrawArm;
	human_model.calcHumanAnims = true;
	human_model.usesHumanSkin  = true;
	return &human_model;
}


/*########################################################################################################################*
*---------------------------------------------------------ChibiModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelSet chibi_set;

CC_NOINLINE static void ChibiModel_ScalePart(struct ModelPart* dst, struct ModelPart* src) {
	struct Model* chibi = Models.Active;
	int i;

	*dst = *src;
	dst->rotX *= 0.5f; dst->rotY *= 0.5f; dst->rotZ *= 0.5f;
	
	for (i = src->offset; i < src->offset + src->count; i++) {
		chibi->vertices[i] = human_model.vertices[i];

		chibi->vertices[i].X *= 0.5f;
		chibi->vertices[i].Y *= 0.5f;
		chibi->vertices[i].Z *= 0.5f;
	}
}

CC_NOINLINE static void ChibiModel_ScaleLimbs(struct ModelLimbs* dst, struct ModelLimbs* src) {
	ChibiModel_ScalePart(&dst->leftLeg,  &src->leftLeg);
	ChibiModel_ScalePart(&dst->rightLeg, &src->rightLeg);
	ChibiModel_ScalePart(&dst->leftArm,  &src->leftArm);
	ChibiModel_ScalePart(&dst->rightArm, &src->rightArm);

	ChibiModel_ScalePart(&dst->leftLegLayer,  &src->leftLegLayer);
	ChibiModel_ScalePart(&dst->rightLegLayer, &src->rightLegLayer);
	ChibiModel_ScalePart(&dst->leftArmLayer,  &src->leftArmLayer);
	ChibiModel_ScalePart(&dst->rightArmLayer, &src->rightArmLayer);
}

static void ChibiModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,12,-4, 4,20,4),
		BoxDesc_Rot(0,13,0),
	};
	static const struct BoxDesc hat = {
		BoxDesc_Tex(32,0),
		BoxDesc_Dims(-4,12,-4, 4,20,4),
		BoxDesc_Bounds(-4.25f,11.75f,-4.25f, 4.25f,20.25f,4.25f),
		BoxDesc_Rot(0,13,0),
	}; 

	/* Chibi is mostly just half scale humanoid */
	ChibiModel_ScalePart(&chibi_set.torso,      &human_set.torso);
	ChibiModel_ScalePart(&chibi_set.torsoLayer, &human_set.torsoLayer);
	ChibiModel_ScaleLimbs(&chibi_set.limbs[0], &human_set.limbs[0]);
	ChibiModel_ScaleLimbs(&chibi_set.limbs[1], &human_set.limbs[1]);
	ChibiModel_ScaleLimbs(&chibi_set.limbs[2], &human_set.limbs[2]);

	/* But head is at normal size */
	Models.Active->index = human_set.head.offset;
	BoxDesc_BuildBox(&chibi_set.head, &head);
	Models.Active->index = human_set.hat.offset;
	BoxDesc_BuildBox(&chibi_set.hat,  &hat);
}

static void ChibiModel_Draw(struct Entity* entity) {
	HumanModel_DrawCore(entity, &chibi_set, true);
}

static void ChibiModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArmCore(entity, &chibi_set);
}

static float ChibiModel_GetNameY(struct Entity* e) { return 20.2f/16.0f; }
static float ChibiModel_GetEyeY(struct Entity* e)  { return 14.0f/16.0f; }
static void ChibiModel_GetSize(struct Entity* e)   { Model_RetSize(4.6f,20.1f,4.6f); }
static void ChibiModel_GetBounds(struct Entity* e) { Model_RetAABB(-4,0,-4, 4,16,4); }

static struct ModelVertex chibi_vertices[MODEL_BOX_VERTICES * (7 + 7 + 4)];
static struct Model chibi_model = { "chibi", chibi_vertices, &human_tex,
	ChibiModel_MakeParts, ChibiModel_Draw,
	ChibiModel_GetNameY,  ChibiModel_GetEyeY, 
	ChibiModel_GetSize,   ChibiModel_GetBounds
};

static struct Model* ChibiModel_GetInstance(void) {
	Model_Init(&chibi_model);
	chibi_model.DrawArm  = ChibiModel_DrawArm;
	chibi_model.armX = 3; chibi_model.armY = 6;
	chibi_model.calcHumanAnims = true;
	chibi_model.usesHumanSkin  = true;
	chibi_model.maxScale    = 3.0f;
	chibi_model.shadowScale = 0.5f;
	return &chibi_model;
}


/*########################################################################################################################*
*--------------------------------------------------------SittingModel-----------------------------------------------------*
*#########################################################################################################################*/
#define SIT_OFFSET 10.0f

static void SittingModel_GetTransform(struct Entity* entity, Vec3 pos, struct Matrix* m) {
	pos.Y -= (SIT_OFFSET / 16.0f) * entity->ModelScale.Y;
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}

static void SittingModel_Draw(struct Entity* entity) {
	entity->Anim.LeftLegX = 1.5f;  entity->Anim.RightLegX = 1.5f;
	entity->Anim.LeftLegZ = -0.1f; entity->Anim.RightLegZ = 0.1f;
	HumanModel_Draw(entity);
}

static float SittingModel_GetEyeY(struct Entity* e)  { return (26.0f - SIT_OFFSET) / 16.0f; }
static void SittingModel_GetSize(struct Entity* e)   { Model_RetSize(8.6f,28.1f - SIT_OFFSET,8.6f); }
static void SittingModel_GetBounds(struct Entity* e) { Model_RetAABB(-8,0,-4, 8,32 - SIT_OFFSET,4); }

static struct Model sitting_model = { "sit", human_vertices, &human_tex,
	Model_NoParts,        SittingModel_Draw,
	HumanModel_GetNameY,  SittingModel_GetEyeY,
	SittingModel_GetSize, SittingModel_GetBounds
};

static struct Model* SittingModel_GetInstance(void) {
	Model_Init(&sitting_model);
	sitting_model.DrawArm  = HumanModel_DrawArm;
	sitting_model.calcHumanAnims = true;
	sitting_model.usesHumanSkin  = true;
	sitting_model.shadowScale  = 0.5f;
	sitting_model.GetTransform = SittingModel_GetTransform;
	return &sitting_model;
}


/*########################################################################################################################*
*--------------------------------------------------------CorpseModel------------------------------------------------------*
*#########################################################################################################################*/
static void CorpseModel_Draw(struct Entity* entity) {
	entity->Anim.LeftLegX = 0.025f; entity->Anim.RightLegX = 0.025f;
	entity->Anim.LeftArmX = 0.025f; entity->Anim.RightArmX = 0.025f;
	entity->Anim.LeftLegZ = -0.15f; entity->Anim.RightLegZ =  0.15f;
	entity->Anim.LeftArmZ = -0.20f; entity->Anim.RightArmZ =  0.20f;
	HumanModel_Draw(entity);
}

static struct Model corpse_model;
static struct Model* CorpseModel_GetInstance(void) {
	corpse_model      = human_model;
	corpse_model.name = "corpse";
	corpse_model.MakeParts = Model_NoParts;
	corpse_model.Draw = CorpseModel_Draw;
	return &corpse_model;
}


/*########################################################################################################################*
*---------------------------------------------------------HeadModel-------------------------------------------------------*
*#########################################################################################################################*/
static void HeadModel_GetTransform(struct Entity* entity, Vec3 pos, struct Matrix* m) {
	pos.Y -= (24.0f/16.0f) * entity->ModelScale.Y;
	Entity_GetTransform(entity, pos, entity->ModelScale, m);
}

static void HeadModel_Draw(struct Entity* entity) {
	struct ModelPart part;
	Model_ApplyTexture(entity);

	part = human_set.head; part.rotY += 4.0f/16.0f;
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &part, true);
	part = human_set.hat;  part.rotY += 4.0f/16.0f;
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &part, true);

	Model_UpdateVB();
}

static float HeadModel_GetEyeY(struct Entity* e)  { return 6.0f/16.0f; }
static void HeadModel_GetSize(struct Entity* e)   { Model_RetSize(7.9f,7.9f,7.9f); }
static void HeadModel_GetBounds(struct Entity* e) { Model_RetAABB(-4,0,-4, 4,8,4); }

static struct Model head_model = { "head", human_vertices, &human_tex,
	Model_NoParts,       HeadModel_Draw,
	HumanModel_GetNameY, HeadModel_GetEyeY,
	HeadModel_GetSize,   HeadModel_GetBounds
};

static struct Model* HeadModel_GetInstance(void) {
	Model_Init(&head_model);
	head_model.usesHumanSkin = true;
	head_model.pushes        = false;
	head_model.GetTransform  = HeadModel_GetTransform;
	return &head_model;
}


/*########################################################################################################################*
*--------------------------------------------------------ChickenModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart chicken_head, chicken_wattle, chicken_beak, chicken_torso;
static struct ModelPart chicken_leftLeg, chicken_rightLeg, chicken_leftWing, Chicken_RightWing;

static void ChickenModel_MakeLeg(struct ModelPart* part, int x1, int x2, int legX1, int legX2) {
#define ch_y1 (1.0f  / 64.0f)
#define ch_y2 (5.0f  / 16.0f)
#define ch_z2 (1.0f  / 16.0f)
#define ch_z1 (-2.0f / 16.0f)

	struct Model* m = Models.Active;
	BoxDesc_YQuad(m, 32, 0, 3, 3,
		x2 / 16.0f, x1 / 16.0f, ch_z1, ch_z2, ch_y1, false); /* bottom feet */
	BoxDesc_ZQuad(m, 36, 3, 1, 5,
		legX1 / 16.0f, legX2 / 16.0f, ch_y1, ch_y2, ch_z2, false); /* vertical part of leg */

	ModelPart_Init(part, m->index - MODEL_QUAD_VERTICES * 2, MODEL_QUAD_VERTICES * 2,
		0.0f / 16.0f, 5.0f / 16.0f, 1.0f / 16.0f);
}

static void ChickenModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-2,9,-6, 2,15,-3),
		BoxDesc_Rot(0,9,-4),
	};
	static const struct BoxDesc wattle = {
		BoxDesc_Tex(14,4),
		BoxDesc_Box(-1,9,-7, 1,11,-5),
		BoxDesc_Rot(0,9,-4),
	};
	static const struct BoxDesc beak = {
		BoxDesc_Tex(14,0),
		BoxDesc_Box(-2,11,-8, 2,13,-6),
		BoxDesc_Rot(0,9,-4),
	};
	static const struct BoxDesc torso = {
		BoxDesc_Tex(0,9),
		BoxDesc_Box(-3,5,-4, 3,11,3),
		BoxDesc_Rot(0,5,0),
	};
	static const struct BoxDesc lWing = {
		BoxDesc_Tex(24,13),
		BoxDesc_Box(-4,7,-3, -3,11,3),
		BoxDesc_Rot(-3,11,0),
	}; 
	static const struct BoxDesc rWing = {
		BoxDesc_Tex(24,13),
		BoxDesc_Box(3,7,-3, 4,11,3),
		BoxDesc_Rot(3,11,0),
	}; 
	
	BoxDesc_BuildBox(&chicken_head,   &head);
	BoxDesc_BuildBox(&chicken_wattle, &wattle);
	BoxDesc_BuildBox(&chicken_beak,   &beak);
	BoxDesc_BuildRotatedBox(&chicken_torso, &torso);
	BoxDesc_BuildBox(&chicken_leftWing,     &lWing);
	BoxDesc_BuildBox(&Chicken_RightWing,    &rWing);

	ChickenModel_MakeLeg(&chicken_leftLeg, -3, 0, -2, -1);
	ChickenModel_MakeLeg(&chicken_rightLeg, 0, 3, 1, 2);
}

static void ChickenModel_Draw(struct Entity* entity) {
	PackedCol col = Models.Cols[0];
	int i;
	Model_ApplyTexture(entity);

	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &chicken_head,   true);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &chicken_wattle, true);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &chicken_beak,   true);

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

static float ChickenModel_GetNameY(struct Entity* e) { return 1.0125f; }
static float ChickenModel_GetEyeY(struct Entity* e)  { return 14.0f/16.0f; }
static void ChickenModel_GetSize(struct Entity* e)   { Model_RetSize(8.0f,12.0f,8.0f); }
static void ChickenModel_GetBounds(struct Entity* e) { Model_RetAABB(-4,0,-8, 4,15,4); }

static struct ModelVertex chicken_vertices[MODEL_BOX_VERTICES * 6 + (MODEL_QUAD_VERTICES * 2) * 2];
static struct ModelTex chicken_tex = { "chicken.png" };
static struct Model chicken_model = { "chicken", chicken_vertices, &chicken_tex,
	ChickenModel_MakeParts, ChickenModel_Draw,
	ChickenModel_GetNameY,  ChickenModel_GetEyeY,
	ChickenModel_GetSize,   ChickenModel_GetBounds
};

static struct Model* ChickenModel_GetInstance(void) {
	Model_Init(&chicken_model);
	return &chicken_model;
}


/*########################################################################################################################*
*--------------------------------------------------------CreeperModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart creeper_head, creeper_torso, creeper_leftLegFront;
static struct ModelPart creeper_rightLegFront, creeper_leftLegBack, creeper_rightLegBack;

static void CreeperModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,18,-4, 4,26,4),
		BoxDesc_Rot(0,18,0),
	};
	static const struct BoxDesc torso = {
		BoxDesc_Tex(16,16),
		BoxDesc_Box(-4,6,-2, 4,18,2),
		BoxDesc_Rot(0,6,0),
	}; 
	static const struct BoxDesc lFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-4,0,-6, 0,6,-2),
		BoxDesc_Rot(0,6,-2),
	}; 
	static const struct BoxDesc rFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(0,0,-6, 4,6,-2),
		BoxDesc_Rot(0,6,-2),
	}; 
	static const struct BoxDesc lBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-4,0,2, 0,6,6),
		BoxDesc_Rot(0,6,2),
	}; 
	static const struct BoxDesc rBack = {
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

static void CreeperModel_Draw(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &creeper_head, true);

	Model_DrawPart(&creeper_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &creeper_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &creeper_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &creeper_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &creeper_rightLegBack,  false);
	Model_UpdateVB();
}

static float CreeperModel_GetNameY(struct Entity* e) { return 1.7f; }
static float CreeperModel_GetEyeY(struct Entity* e)  { return 22.0f/16.0f; }
static void CreeperModel_GetSize(struct Entity* e)   { Model_RetSize(8.0f,26.0f,8.0f); }
static void CreeperModel_GetBounds(struct Entity* e) { Model_RetAABB(-4,0,-6, 4,26,6); }

static struct ModelVertex creeper_vertices[MODEL_BOX_VERTICES * 6];
static struct ModelTex creeper_tex = { "creeper.png" };
static struct Model creeper_model  = { 
	"creeper", creeper_vertices, &creeper_tex,
	CreeperModel_MakeParts, CreeperModel_Draw,
	CreeperModel_GetNameY,  CreeperModel_GetEyeY,
	CreeperModel_GetSize,   CreeperModel_GetBounds
};

static struct Model* CreeperModel_GetInstance(void) {
	Model_Init(&creeper_model);
	return &creeper_model;
}


/*########################################################################################################################*
*----------------------------------------------------------PigModel-------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart pig_head, pig_torso, pig_leftLegFront, pig_rightLegFront;
static struct ModelPart pig_leftLegBack, pig_rightLegBack;

static void PigModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,8,-14, 4,16,-6),
		BoxDesc_Rot(0,12,-6),
	}; 
	static const struct BoxDesc torso = {
		BoxDesc_Tex(28,8),
		BoxDesc_Box(-5,6,-8, 5,14,8),
		BoxDesc_Rot(0,6,0),
	}; 
	static const struct BoxDesc lFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,-7, -1,6,-3),
		BoxDesc_Rot(0,6,-5),
	}; 
	static const struct BoxDesc rFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,-7, 5,6,-3),
		BoxDesc_Rot(0,6,-5),
	}; 
	static const struct BoxDesc lBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,5, -1,6,9),
		BoxDesc_Rot(0,6,7),
	}; 
	static const struct BoxDesc rBack = {
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

static void PigModel_Draw(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &pig_head, true);

	Model_DrawPart(&pig_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &pig_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &pig_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &pig_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &pig_rightLegBack,  false);
	Model_UpdateVB();
}

static float PigModel_GetNameY(struct Entity* e) { return 1.075f; }
static float PigModel_GetEyeY(struct Entity* e)  { return 12.0f/16.0f; }
static void PigModel_GetSize(struct Entity* e)   { Model_RetSize(14.0f,14.0f,14.0f); }
static void PigModel_GetBounds(struct Entity* e) { Model_RetAABB(-5,0,-14, 5,16,9); }

static struct ModelVertex pig_vertices[MODEL_BOX_VERTICES * 6];
static struct ModelTex pig_tex = { "pig.png" };
static struct Model pig_model  = { "pig", pig_vertices, &pig_tex,
	PigModel_MakeParts, PigModel_Draw,
	PigModel_GetNameY,  PigModel_GetEyeY,
	PigModel_GetSize,   PigModel_GetBounds
};

static struct Model* PigModel_GetInstance(void) {
	Model_Init(&pig_model);
	return &pig_model;
}


/*########################################################################################################################*
*---------------------------------------------------------SheepModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart sheep_head, sheep_torso, sheep_leftLegFront;
static struct ModelPart sheep_rightLegFront, sheep_leftLegBack, sheep_rightLegBack;
static struct ModelPart fur_head, fur_torso, fur_leftLegFront, fur_rightLegFront;
static struct ModelPart fur_leftLegBack, fur_rightLegBack;
static struct ModelTex fur_tex = { "sheep_fur.png" };

static void SheepModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-3,16,-14, 3,22,-6),
		BoxDesc_Rot(0,18,-8),
	};
	static const struct BoxDesc torso = {
		BoxDesc_Tex(28,8),
		BoxDesc_Box(-4,12,-8, 4,18,8),
		BoxDesc_Rot(0,12,0),
	}; 	
	static const struct BoxDesc lFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,-7, -1,12,-3),
		BoxDesc_Rot(0,12,-5),
	}; 
	static const struct BoxDesc rFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,-7, 5,12,-3),
		BoxDesc_Rot(0,12,-5),
	}; 
	static const struct BoxDesc lBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-5,0,5, -1,12,9),
		BoxDesc_Rot(0,12,7),
	}; 
	static const struct BoxDesc rBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,5, 5,12,9),
		BoxDesc_Rot(0,12,7),
	};

	static const struct BoxDesc fHead = {
		BoxDesc_Tex(0,0),
		BoxDesc_Dims(-3,16,-12, 3,22,-6),
		BoxDesc_Bounds(-3.5f,15.5f,-12.5f, 3.5f,22.5f,-5.5f),
		BoxDesc_Rot(0,18,-8),
	}; 
	static const struct BoxDesc fTorso = {
		BoxDesc_Tex(28,8),
		BoxDesc_Dims(-4,12,-8, 4,18,8),
		BoxDesc_Bounds(-6.0f,10.5f,-10.0f, 6.0f,19.5f,10.0f),
		BoxDesc_Rot(0,12,0),
	};
	static const struct BoxDesc flFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(-5,6,-7, -1,12,-3),
		BoxDesc_Bounds(-5.5f,5.5f,-7.5f, -0.5f,12.5f,-2.5f),
		BoxDesc_Rot(0,12,-5),
	};
	static const struct BoxDesc frFront = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(1,6,-7, 5,12,-3),
		BoxDesc_Bounds(0.5f,5.5f,-7.5f, 5.5f,12.5f,-2.5f),
		BoxDesc_Rot(0,12,-5),
	};
	static const struct BoxDesc flBack = {
		BoxDesc_Tex(0,16),
		BoxDesc_Dims(-5,6,5, -1,12,9),
		BoxDesc_Bounds(-5.5f,5.5f,4.5f, -0.5f,12.5f,9.5f),
		BoxDesc_Rot(0,12,7),
	}; 
	static const struct BoxDesc frBack = {
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

static void FurlessModel_Draw(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &sheep_head, true);

	Model_DrawPart(&sheep_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &sheep_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &sheep_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &sheep_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &sheep_rightLegBack,  false);
	Model_UpdateVB();
}

static void SheepModel_Draw(struct Entity* entity) {
	FurlessModel_Draw(entity);
	Gfx_BindTexture(fur_tex.texID);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &fur_head, true);

	Model_DrawPart(&fur_torso);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &fur_leftLegFront,  false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &fur_rightLegFront, false);
	Model_DrawRotate(entity->Anim.RightLegX, 0, 0, &fur_leftLegBack,   false);
	Model_DrawRotate(entity->Anim.LeftLegX,  0, 0, &fur_rightLegBack,  false);
	Model_UpdateVB();
}

static float SheepModel_GetNameY(struct Entity* e) { return 1.48125f; }
static float SheepModel_GetEyeY(struct Entity* e)  { return 20.0f/16.0f; }
static void SheepModel_GetSize(struct Entity* e)   { Model_RetSize(10.0f,20.0f,10.0f); }
static void SheepModel_GetBounds(struct Entity* e) { Model_RetAABB(-6,0,-13, 6,23,10); }

static struct ModelVertex sheep_vertices[MODEL_BOX_VERTICES * 6 * 2];
static struct ModelTex sheep_tex = { "sheep.png" };
static struct Model sheep_model  = { "sheep", sheep_vertices, &sheep_tex,
	SheepModel_MakeParts, SheepModel_Draw,
	SheepModel_GetNameY,  SheepModel_GetEyeY,
	SheepModel_GetSize,   SheepModel_GetBounds
};
static struct Model nofur_model  = { "sheep_nofur", sheep_vertices, &sheep_tex,
	SheepModel_MakeParts, FurlessModel_Draw,
	SheepModel_GetNameY,  SheepModel_GetEyeY,
	SheepModel_GetSize,   SheepModel_GetBounds
};

static struct Model* SheepModel_GetInstance(void) {
	Model_Init(&sheep_model);
	return &sheep_model;
}

static struct Model* NoFurModel_GetInstance(void) {
	Model_Init(&nofur_model);
	return &nofur_model;
}


/*########################################################################################################################*
*-------------------------------------------------------SkeletonModel-----------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart skeleton_head, skeleton_torso, skeleton_leftLeg;
static struct ModelPart skeleton_rightLeg, skeleton_leftArm, skeleton_rightArm;

static void SkeletonModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-4,24,-4, 4,32,4),
		BoxDesc_Rot(0,24,0),
	};
	static const struct BoxDesc torso = {
		BoxDesc_Tex(16,16),
		BoxDesc_Box(-4,12,-2, 4,24,2),
		BoxDesc_Rot(0,12,0),
	}; 
	static const struct BoxDesc lLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(-1,0,-1, -3,12,1),
		BoxDesc_Rot(0,12,0),
	}; 
	static const struct BoxDesc rLeg = {
		BoxDesc_Tex(0,16),
		BoxDesc_Box(1,0,-1, 3,12,1),
		BoxDesc_Rot(0,12,0),
	}; 
	static const struct BoxDesc lArm = {
		BoxDesc_Tex(40,16),
		BoxDesc_Box(-4,12,-1, -6,24,1),
		BoxDesc_Rot(-5,23,0),
	}; 
	static const struct BoxDesc rArm = {
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

static void SkeletonModel_Draw(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &skeleton_head, true);

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

static void SkeletonModel_GetSize(struct Entity* e)   { Model_RetSize(8.0f,28.1f,8.0f); }
static void SkeletonModel_GetBounds(struct Entity* e) { Model_RetAABB(-4,0,-4, 4,32,4); }

static struct ModelVertex skeleton_vertices[MODEL_BOX_VERTICES * 6];
static struct ModelTex skeleton_tex = { "skeleton.png" };
static struct Model skeleton_model  = { "skeleton", skeleton_vertices, &skeleton_tex,
	SkeletonModel_MakeParts, SkeletonModel_Draw,
	HumanModel_GetNameY,     HumanModel_GetEyeY,
	SkeletonModel_GetSize,   SkeletonModel_GetBounds
};

static struct Model* SkeletonModel_GetInstance(void) {
	Model_Init(&skeleton_model);
	skeleton_model.DrawArm  = SkeletonModel_DrawArm;
	skeleton_model.armX = 5;
	return &skeleton_model;
}


/*########################################################################################################################*
*--------------------------------------------------------SpiderModel------------------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart spider_head, spider_link, spider_end;
static struct ModelPart spider_leftLeg, spider_rightLeg;

static void SpiderModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(32,4),
		BoxDesc_Box(-4,4,-11, 4,12,-3),
		BoxDesc_Rot(0,8,-3),
	};
	static const struct BoxDesc link = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-3,5,3, 3,11,-3),
		BoxDesc_Rot(0,5,0),
	}; 
	static const struct BoxDesc end = {
		BoxDesc_Tex(0,12),
		BoxDesc_Box(-5,4,3, 5,12,15),
		BoxDesc_Rot(0,4,9),
	}; 
	static const struct BoxDesc lLeg = {
		BoxDesc_Tex(18,0),
		BoxDesc_Box(-19,7,-1, -3,9,1),
		BoxDesc_Rot(-3,8,0),
	}; 
	static const struct BoxDesc rLeg = {
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

#define quarterPi (MATH_PI / 4.0f)
#define eighthPi  (MATH_PI / 8.0f)

static void SpiderModel_Draw(struct Entity* entity) {
	float rotX, rotY, rotZ;
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &spider_head, true);
	Model_DrawPart(&spider_link);
	Model_DrawPart(&spider_end);

	rotX = (float)Math_Sin(entity->Anim.WalkTime)     * entity->Anim.Swing * MATH_PI;
	rotZ = (float)Math_Cos(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 16.0f;
	rotY = (float)Math_Sin(entity->Anim.WalkTime * 2) * entity->Anim.Swing * MATH_PI / 32.0f;
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

static float SpiderModel_GetNameY(struct Entity* e) { return 1.0125f; }
static float SpiderModel_GetEyeY(struct Entity* e)  { return 8.0f/16.0f; }
static void SpiderModel_GetSize(struct Entity* e)   { Model_RetSize(15.0f,12.0f,15.0f); }
static void SpiderModel_GetBounds(struct Entity* e) { Model_RetAABB(-5,0,-11, 5,12,15); }

static struct ModelVertex spider_vertices[MODEL_BOX_VERTICES * 5];
static struct ModelTex spider_tex = { "spider.png" };
static struct Model spider_model  = { "spider", spider_vertices, &spider_tex,
	SpiderModel_MakeParts, SpiderModel_Draw,
	SpiderModel_GetNameY,  SpiderModel_GetEyeY,
	SpiderModel_GetSize,   SpiderModel_GetBounds
};

static struct Model* SpiderModel_GetInstance(void) {
	Model_Init(&spider_model);
	return &spider_model;
}


/*########################################################################################################################*
*--------------------------------------------------------ZombieModel------------------------------------------------------*
*#########################################################################################################################*/

static void ZombieModel_Draw(struct Entity* entity) {
	entity->Anim.LeftArmX  = 90.0f * MATH_DEG2RAD;
	entity->Anim.RightArmX = 90.0f * MATH_DEG2RAD;
	HumanModel_DrawCore(entity, &human_set, false);
}
static void ZombieModel_DrawArm(struct Entity* entity) {
	HumanModel_DrawArmCore(entity, &human_set);
}

static void ZombieModel_GetBounds(struct Entity* e) { Model_RetAABB(-4,0,-4, 4,32,4); }

static struct ModelTex zombie_tex = { "zombie.png" };
static struct Model zombie_model  = { "zombie", human_vertices, &zombie_tex,
	Model_NoParts,       ZombieModel_Draw,
	HumanModel_GetNameY, HumanModel_GetEyeY, 
	HumanModel_GetSize,  ZombieModel_GetBounds 
};

static struct Model* ZombieModel_GetInstance(void) {
	Model_Init(&zombie_model);
	zombie_model.DrawArm  = ZombieModel_DrawArm;
	return &zombie_model;
}


/*########################################################################################################################*
*---------------------------------------------------------BlockModel------------------------------------------------------*
*#########################################################################################################################*/
static BlockID bModel_block = BLOCK_AIR;
static int bModel_index, bModel_texIndices[8];

static float BlockModel_GetNameY(struct Entity* e) {
	BlockID block = e->ModelBlock;
	return Blocks.MaxBB[block].Y + 0.075f;
}

static float BlockModel_GetEyeY(struct Entity* e) {
	BlockID block = e->ModelBlock;
	float minY    = Blocks.MinBB[block].Y;
	float maxY    = Blocks.MaxBB[block].Y;
	return (minY + maxY) / 2.0f;
}

static void BlockModel_GetSize(struct Entity* e) {
	static Vec3 shrink = { 0.75f/16.0f, 0.75f/16.0f, 0.75f/16.0f };
	Vec3* size = &e->Size;
	BlockID block = e->ModelBlock;

	Vec3_Sub(size, &Blocks.MaxBB[block], &Blocks.MinBB[block]);
	/* to fit slightly inside */
	Vec3_SubBy(size, &shrink);

	/* fix for 0 size blocks */
	size->X = max(size->X, 0.125f/16.0f);
	size->Y = max(size->Y, 0.125f/16.0f);
	size->Z = max(size->Z, 0.125f/16.0f);
}

static void BlockModel_GetBounds(struct Entity* e) {
	static Vec3 offset = { -0.5f, 0.0f, -0.5f };
	BlockID block = e->ModelBlock;

	Vec3_Add(&e->ModelAABB.Min, &Blocks.MinBB[block], &offset);
	Vec3_Add(&e->ModelAABB.Max, &Blocks.MaxBB[block], &offset);
}

static TextureLoc BlockModel_GetTex(Face face) {
	TextureLoc loc = Block_Tex(bModel_block, face);
	bModel_texIndices[bModel_index++] = Atlas1D_Index(loc);
	return loc;
}

static void BlockModel_SpriteZQuad(cc_bool firstPart, cc_bool mirror) {
	VertexP3fT2fC4b* ptr, v;
	PackedCol col; int tmp;
	float xz1, xz2;
	TextureLoc loc = BlockModel_GetTex(FACE_ZMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &tmp);

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

	ptr   = &Models.Vertices[(bModel_index - 1) << 2];
	v.Col = col;

	v.X = xz1; v.Y = 0.0f; v.Z = xz1; v.U = rec.U2; v.V = rec.V2; *ptr++ = v;
	           v.Y = 1.0f;                          v.V = rec.V1; *ptr++ = v;
	v.X = xz2;             v.Z = xz2; v.U = rec.U1;               *ptr++ = v;
	           v.Y = 0.0f;                          v.V = rec.V2; *ptr++ = v;
}

static void BlockModel_SpriteXQuad(cc_bool firstPart, cc_bool mirror) {
	VertexP3fT2fC4b* ptr, v;
	PackedCol col; int tmp;
	float x1, x2, z1, z2;
	TextureLoc loc = BlockModel_GetTex(FACE_XMAX);
	TextureRec rec = Atlas1D_TexRec(loc, 1, &tmp);

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

	ptr   = &Models.Vertices[(bModel_index - 1) << 2];
	v.Col = col;

	v.X = x1; v.Y = 0.0f; v.Z = z1; v.U = rec.U2; v.V = rec.V2; *ptr++ = v;
	          v.Y = 1.0f;                         v.V = rec.V1; *ptr++ = v;
	v.X = x2;             v.Z = z2; v.U = rec.U1;               *ptr++ = v;
	          v.Y = 0.0f;                         v.V = rec.V2; *ptr++ = v;
}

static void BlockModel_BuildParts(cc_bool sprite) {
	Vec3 min, max;
	TextureLoc loc;
	VertexP3fT2fC4b* ptr;

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

		ptr = Models.Vertices;
		min = Blocks.RenderMinBB[bModel_block];
		max = Blocks.RenderMaxBB[bModel_block];

		Drawer.X1 = min.X - 0.5f; Drawer.Y1 = min.Y; Drawer.Z1 = min.Z - 0.5f;
		Drawer.X2 = max.X - 0.5f; Drawer.Y2 = max.Y; Drawer.Z2 = max.Z - 0.5f;		

		loc = BlockModel_GetTex(FACE_YMIN); Drawer_YMin(1, Models.Cols[1], loc, &ptr);
		loc = BlockModel_GetTex(FACE_ZMIN); Drawer_ZMin(1, Models.Cols[3], loc, &ptr);
		loc = BlockModel_GetTex(FACE_XMAX); Drawer_XMax(1, Models.Cols[5], loc, &ptr);
		loc = BlockModel_GetTex(FACE_ZMAX); Drawer_ZMax(1, Models.Cols[2], loc, &ptr);
		loc = BlockModel_GetTex(FACE_XMIN); Drawer_XMin(1, Models.Cols[4], loc, &ptr);
		loc = BlockModel_GetTex(FACE_YMAX); Drawer_YMax(1, Models.Cols[0], loc, &ptr);
	}
}

static void BlockModel_DrawParts(void) {
	int lastTexIndex, i, offset = 0, count = 0;
	Gfx_SetDynamicVbData(Models.Vb, Models.Vertices, bModel_index * 4);

	lastTexIndex = bModel_texIndices[0];
	for (i = 0; i < bModel_index; i++, count += 4) {
		if (bModel_texIndices[i] == lastTexIndex) continue;

		/* Different 1D flush texture, flush current vertices */
		Gfx_BindTexture(Atlas1D.TexIds[lastTexIndex]);
		Gfx_DrawVb_IndexedTris_Range(count, offset);
		lastTexIndex = bModel_texIndices[i];
			
		offset += count;
		count   = 0;
	}

	/* Leftover vertices */
	if (!count) return;
	Gfx_BindTexture(Atlas1D.TexIds[lastTexIndex]); 
	Gfx_DrawVb_IndexedTris_Range(count, offset);
}

static void BlockModel_Draw(struct Entity* p) {
	cc_bool sprite;
	int i;

	bModel_block = p->ModelBlock;
	bModel_index = 0;
	if (Blocks.Draw[bModel_block] == DRAW_GAS) return;

	if (Blocks.FullBright[bModel_block]) {
		for (i = 0; i < FACE_COUNT; i++) {
			Models.Cols[i] = PACKEDCOL_WHITE;
		}
	}

	sprite = Blocks.Draw[bModel_block] == DRAW_SPRITE;
	BlockModel_BuildParts(sprite);

	if (sprite) Gfx_SetFaceCulling(true);
	BlockModel_DrawParts();
	if (sprite) Gfx_SetFaceCulling(false);
}

static struct Model block_model = { "block", NULL, &human_tex,
	Model_NoParts,       BlockModel_Draw,
	BlockModel_GetNameY, BlockModel_GetEyeY,
	BlockModel_GetSize,  BlockModel_GetBounds,
};

static struct Model* BlockModel_GetInstance(void) {
	Model_Init(&block_model);
	block_model.bobbing  = false;
	block_model.usesSkin = false;
	block_model.pushes   = false;
	return &block_model;
}


/*########################################################################################################################*
*----------------------------------------------------------SkinnedCubeModel-----------------------------------------------*
*#########################################################################################################################*/
static struct ModelPart skinnedCube_head;

static void SkinnedCubeModel_MakeParts(void) {
	static const struct BoxDesc head = {
		BoxDesc_Tex(0,0),
		BoxDesc_Box(-8,0,-8, 8,16,8),
		BoxDesc_Rot(0,8,0),
	};

	BoxDesc_BuildBox(&skinnedCube_head, &head);
}

static void SkinnedCubeModel_Draw(struct Entity* entity) {
	Model_ApplyTexture(entity);
	Model_DrawRotate(-entity->Pitch * MATH_DEG2RAD, 0, 0, &skinnedCube_head, true);
	Model_UpdateVB();
}

static float SkinnedCubeModel_GetNameY(struct Entity* e) { return 1.075f; }
static float SkinnedCubeModel_GetEyeY(struct Entity* e) { return 8.0f / 16.0f; }
static void SkinnedCubeModel_GetSize(struct Entity* e) { Model_RetSize(15.0f, 15.0f, 15.0f); }
static void SkinnedCubeModel_GetBounds(struct Entity* e) { Model_RetAABB(-8, 0, -8, 8, 16, 8); }

static struct ModelVertex skinnedCube_vertices[MODEL_BOX_VERTICES];
static struct ModelTex skinnedCube_tex = { "skinnedcube.png" };
static struct Model skinnedCube_model = { "skinnedcube", skinnedCube_vertices, &skinnedCube_tex,
SkinnedCubeModel_MakeParts, SkinnedCubeModel_Draw,
SkinnedCubeModel_GetNameY,  SkinnedCubeModel_GetEyeY,
SkinnedCubeModel_GetSize,   SkinnedCubeModel_GetBounds
};

static struct Model* SkinnedCubeModel_GetInstance(void) {
	Model_Init(&skinnedCube_model);
	skinnedCube_model.usesHumanSkin = true;
	skinnedCube_model.pushes = false;
	return &skinnedCube_model;
}


/*########################################################################################################################*
*-------------------------------------------------------Model component---------------------------------------------------*
*#########################################################################################################################*/
/* NOTE: None of the built in models use more than 12 parts at once. */
static VertexP3fT2fC4b defaultVertices[MODEL_BOX_VERTICES * 12];

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
	Model_RegisterTexture(&skinnedCube_tex);

	Model_Register(HumanoidModel_GetInstance());
	MakeModel(&human_model);
	Models.Human = &human_model;

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
	Model_Register(SkinnedCubeModel_GetInstance());
}

static void Models_Init(void) {
	Models.Vertices    = defaultVertices;
	Models.MaxVertices = Array_Elems(defaultVertices);

	Model_RegisterDefaultModels();
	Models_ContextRecreated(NULL);
	Models.ClassicArms = Options_GetBool(OPT_CLASSIC_ARM_MODEL, Game_ClassicMode);

	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, Models_TextureChanged);
	Event_RegisterVoid(&GfxEvents.ContextLost,      NULL, Models_ContextLost);
	Event_RegisterVoid(&GfxEvents.ContextRecreated, NULL, Models_ContextRecreated);
}

static void Models_Free(void) {
	struct ModelTex* tex;

	for (tex = textures_head; tex; tex = tex->next) {
		Gfx_DeleteTexture(&tex->texID);
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
