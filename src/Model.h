#ifndef CC_MODEL_H
#define CC_MODEL_H
#include "Vectors.h"
#include "PackedCol.h"
#include "Constants.h"
#include "Physics.h"
CC_BEGIN_HEADER

/* Contains various structs and methods for an entity model.
   Also contains a list of models and default textures for those models.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct Entity;
struct AABB;
struct IGameComponent;
struct VertexTextured;
extern struct IGameComponent Models_Component;

#define MODEL_QUAD_VERTICES 4
#define MODEL_BOX_VERTICES (FACE_COUNT * MODEL_QUAD_VERTICES)
enum RotateOrder { ROTATE_ORDER_ZYX, ROTATE_ORDER_XZY, ROTATE_ORDER_YZX, ROTATE_ORDER_XYZ };

/* Describes a vertex within a model. */
struct ModelVertex { float x, y, z; cc_uint16 u, v; };
static CC_INLINE void ModelVertex_Init(struct ModelVertex* vertex, float x, float y, float z, int u, int v) {
	vertex->x = x; vertex->y = y; vertex->z = z;
	vertex->u = u; vertex->v = v;
}

/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
struct ModelPart { cc_uint16 offset, count; float rotX, rotY, rotZ; };
static CC_INLINE void ModelPart_Init(struct ModelPart* part, int offset, int count, float rotX, float rotY, float rotZ) {
	part->offset = offset; part->count = count;
	part->rotX = rotX; part->rotY = rotY; part->rotZ = rotZ;
}

struct ModelTex;
/* Contains information about a texture used for models. */
struct ModelTex { const char* name; cc_uint8 skinType; GfxResourceID texID; struct ModelTex* next; };

#define MODEL_FLAG_INITED    0x01
#define MODEL_FLAG_CLEAR_HAT 0x02

struct Model;
/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
struct Model {
	/* Name of this model */
	const char* name;
	/* Pointer to the raw vertices of the model */
	struct ModelVertex* vertices;
	/* Pointer to default texture for this model */
	struct ModelTex* defaultTex;

	/* Creates the ModelParts of this model and fills out vertices. */
	void (*MakeParts)(void);
	/* Draws/Renders this model for the given entity. */
	void (*Draw)(struct Entity* entity);
	/* Returns height the 'nametag' gets drawn at above the entity's feet. */
	float (*GetNameY)(struct Entity* entity);
	/* Returns height the 'eye' is located at above the entity's feet. */
	float (*GetEyeY)(struct Entity* entity);
	/* Sets entity->Size to the collision size of this model. */
	void (*GetCollisionSize)(struct Entity* entity);
	/* Sets entity->ModelAABB to the 'picking' bounds of this model. */
	/* This is the AABB around the entity in which mouse clicks trigger 'interaction'. */
	/* NOTE: These bounds are not transformed. (i.e. no rotation, centered around 0,0,0) */
	void (*GetPickingBounds)(struct Entity* entity);

	/* The rest of the fields are set in Model_Init() */
	int index;
	cc_uint8 armX, armY; /* these translate arm model part back to (0, 0) */

	cc_uint8 flags;
	/* Whether the model should be slightly bobbed up and down when rendering. */
	/* e.g. for HumanoidModel, when legs are at the peak of their swing, whole model is moved slightly down */
	cc_bool bobbing;
	cc_bool usesSkin, calcHumanAnims, usesHumanSkin, pushes;

	float gravity; Vec3 drag, groundFriction;

	/* Returns the transformation matrix applied to the model when rendering. */
	/* NOTE: Most models just use Entity_GetTransform (except SittingModel) */
	void (*GetTransform)(struct Entity* entity, Vec3 pos, struct Matrix* m);
	void (*DrawArm)(struct Entity* entity);

	float maxScale, shadowScale;
	int maxVertices;
	struct Model* next;
};

/* Shared data for models. */
CC_VAR extern struct _ModelsData {
	/* Tint colour applied to the faces of model parts. */
	PackedCol Cols[FACE_COUNT];
	/* U/V scale applied to skin texture when rendering models. */
	/* Default uScale is 1/32, vScale is 1/32 or 1/64 depending on skin. */
	float uScale, vScale;
	/* Angle of offset of head from body rotation */
	float cosHead, sinHead;
	/* Order of axes rotation when rendering parts. */
	cc_uint8 Rotation;
	/* Skin type of current skin texture. */
	cc_uint8 skinType;
	/* Whether to render arms like vanilla Minecraft Classic. */
	cc_bool ClassicArms;
	/* Model currently being built or rendered. */
	struct Model* Active;
	/* Dynamic vertex buffer for uploading model vertices. */
	GfxResourceID Vb;
	/* Temporary storage for vertices. */
	struct VertexTextured* Vertices;
	/* Maximum number of vertices that can be stored in Vertices. */
	/* NOTE: If you change this, you MUST also destroy and recreate the dynamic VB. */
	int MaxVertices;
	/* Pointer to humanoid/human model */
	struct Model* Human;
	/* Pointer to block model */
	struct Model* Block;
} Models;

/* Initialises fields of a model to default. */
CC_API void Model_Init(struct Model* model);

void Model_GetEntityTransform(struct Model* model, struct Entity* e, struct Matrix* transform);
/* Whether the bounding sphere of the model is currently visible. */
cc_bool Model_ShouldRender(struct Entity* entity);
/* Approximately how far the given entity is away from the player. */
float Model_RenderDistance(struct Entity* entity);
/* Draws the given entity as the given model. */
CC_API void Model_Render(struct Model* model, struct Entity* entity);
/* Sets up state to be suitable for rendering the given model. */
/* NOTE: Model_Render already calls this, you don't normally need to call this. */
CC_API void Model_SetupState(struct Model* model, struct Entity* entity);
/* Applies the skin texture of the given entity to the model. */
/* Uses model's default texture if the entity doesn't have a custom skin. */
CC_API void Model_ApplyTexture(struct Entity* entity);

/* Flushes buffered vertices to the GPU. */
CC_API void Model_UpdateVB(void);
void Model_LockVB(struct Entity* entity, int verticesCount);
void Model_UnlockVB(void);

/* Draws the given part with no part-specific rotation (e.g. torso). */
CC_API void Model_DrawPart(struct ModelPart* part);
/* Draws the given part with rotation around part's rotation origin. (e.g. arms, head) */
CC_API void Model_DrawRotate(float angleX, float angleY, float angleZ, struct ModelPart* part, cc_bool head);
/* Renders the 'arm' of a model. */
void Model_RenderArm(struct Model* model, struct Entity* entity);
/* Draws the given part with appropriate rotation to produce an arm look. */
CC_API void Model_DrawArmPart(struct ModelPart* part);

/* Returns a pointer to the model whose name caselessly matches given name. */
CC_API struct Model* Model_Get(const cc_string* name);
/* Adds a model to the list of models. (e.g. "skeleton") */
/* Models can be applied to entities to change their appearance. Use Entity_SetModel for that. */
CC_API void Model_Register(struct Model* model);
/* Unregister a model from the list of models, and set all entities using this model to the default humanoid model. */
void Model_Unregister(struct Model* model);
/* Adds a texture to the list of automatically managed model textures. */
/* These textures are automatically loaded from texture packs. (e.g. "skeleton.png") */
CC_API void Model_RegisterTexture(struct ModelTex* tex);

/* Describes data for a box being built. */
struct BoxDesc {
	cc_uint16 texX, texY;         /* Texture origin */
	cc_uint8 sizeX, sizeY, sizeZ; /* Texture dimensions */
	float x1,y1,z1, x2,y2,z2;    /* Box corners coordinates */
	float rotX,rotY,rotZ;        /* Rotation origin point */
};

#define BoxDesc_Dim(p1, p2) p1 < p2 ? p2 - p1 : p1 - p2
/* Macros for making initialising a BoxDesc easier to understand. See Model.c for how these get used. */
#define BoxDesc_Tex(x, y)                 x,y
#define BoxDesc_Dims(x1,y1,z1,x2,y2,z2)   BoxDesc_Dim(x1,x2), BoxDesc_Dim(y1,y2), BoxDesc_Dim(z1,z2)
#define BoxDesc_Bounds(x1,y1,z1,x2,y2,z2) (x1)/16.0f,(y1)/16.0f,(z1)/16.0f, (x2)/16.0f,(y2)/16.0f,(z2)/16.0f
#define BoxDesc_Rot(x, y, z)              (x)/16.0f,(y)/16.0f,(z)/16.0f
#define BoxDesc_Box(x1,y1,z1,x2,y2,z2)    BoxDesc_Dims(x1,y1,z1,x2,y2,z2), BoxDesc_Bounds(x1,y1,z1,x2,y2,z2)

/* Builds a box model assuming the follow texture layout:
let SW = sides width, BW = body width, BH = body height
*********************************************************************************************
|----------SW----------|----------BW----------|----------BW----------|----------------------|
|S--------------------S|S--------top---------S|S-------bottom-------S|----------------------|
|W--------------------W|W--------tex---------W|W--------tex---------W|----------------------|
|----------SW----------|----------BW----------|----------BW----------|----------------------|
*********************************************************************************************
|----------SW----------|----------BW----------|----------SW----------|----------BW----------|
|B--------left--------B|B-------front--------B|B-------right--------B|B--------back--------B|
|H--------tex---------H|H--------tex---------H|H--------tex---------H|H--------tex---------H|
|----------SW----------|----------BW----------|----------SW----------|----------BW----------|
********************************************************************************************* */
CC_API void BoxDesc_BuildBox(struct ModelPart* part, const struct BoxDesc* desc);

/* Builds a box model assuming the follow texture layout:
let SW = sides width, BW = body width, BH = body height
*********************************************************************************************
|----------SW----------|----------BW----------|----------BW----------|----------------------|
|S--------------------S|S-------front--------S|S--------back--------S|----------------------|
|W--------------------W|W--------tex---------W|W--------tex---------W|----------------------|
|----------SW----------|----------BW----------|----------BW----------|----------------------|
*********************************************************************************************
|----------SW----------|----------BW----------|----------BW----------|----------SW----------|
|B--------left--------B|B-------bottom-------B|B-------right--------B|B--------top---------B|
|H--------tex---------H|H--------tex---------H|H--------tex---------H|H--------tex---------H|
|----------SW----------|----------BW----------|----------BW----------|----------------------|
********************************************************************************************* */
CC_API void BoxDesc_BuildRotatedBox(struct ModelPart* part, const struct BoxDesc* desc);

/* DEPRECATED */
CC_API void BoxDesc_XQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float z1, float z2, float y1, float y2, float x, cc_bool swapU);
CC_API void BoxDesc_YQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float z1, float z2, float y, cc_bool swapU);
CC_API void BoxDesc_ZQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float y1, float y2, float z, cc_bool swapU);

CC_API void BoxDesc_XQuad2(struct Model* m, float z1, float z2, float y1, float y2, float x, int u1, int v1, int u2, int v2);
CC_API void BoxDesc_YQuad2(struct Model* m, float x1, float x2, float z1, float z2, float y, int u1, int v1, int u2, int v2);
CC_API void BoxDesc_ZQuad2(struct Model* m, float x1, float x2, float y1, float y2, float z, int u1, int v1, int u2, int v2);

/* CustomModels */

#define MAX_CUSTOM_MODELS 64
#define MAX_CUSTOM_MODEL_PARTS 64
#define MAX_CUSTOM_MODEL_ANIMS 4

enum CustomModelAnimType {
	CustomModelAnimType_None = 0,
	CustomModelAnimType_Head = 1,
	CustomModelAnimType_LeftLegX = 2,
	CustomModelAnimType_RightLegX = 3,
	CustomModelAnimType_LeftArmX = 4,
	CustomModelAnimType_LeftArmZ = 5,
	CustomModelAnimType_RightArmX = 6,
	CustomModelAnimType_RightArmZ = 7,
	CustomModelAnimType_Spin = 8,
	CustomModelAnimType_SpinVelocity = 9,
	CustomModelAnimType_SinRotate = 10,
	CustomModelAnimType_SinRotateVelocity = 11,
	CustomModelAnimType_SinTranslate = 12,
	CustomModelAnimType_SinTranslateVelocity = 13,
	CustomModelAnimType_SinSize = 14,
	CustomModelAnimType_SinSizeVelocity = 15,
	CustomModelAnimType_FlipRotate = 16,
	CustomModelAnimType_FlipRotateVelocity = 17,
	CustomModelAnimType_FlipTranslate = 18,
	CustomModelAnimType_FlipTranslateVelocity = 19,
	CustomModelAnimType_FlipSize = 20,
	CustomModelAnimType_FlipSizeVelocity = 21
};

enum CustomModelAnimAxis {
	CustomModelAnimAxis_X = 0,
	CustomModelAnimAxis_Y = 1,
	CustomModelAnimAxis_Z = 2,
};

struct CustomModelAnim {
	float a, b, c, d;
};

struct CustomModelPartDef {
	Vec3 min, max;
	/* uv coords in order: top, bottom, front, back, left, right */
	cc_uint16 u1[6], v1[6], u2[6], v2[6];
	Vec3 rotationOrigin;
	cc_uint8 flags;
};

struct CustomModelPart {
	struct ModelPart modelPart;
	Vec3 rotation; /* rotation angles */
	struct CustomModelAnim anims[MAX_CUSTOM_MODEL_ANIMS];
	cc_uint8 animType[MAX_CUSTOM_MODEL_ANIMS];
	cc_uint8 animAxis[MAX_CUSTOM_MODEL_ANIMS];
	cc_bool fullbright;
	cc_bool firstPersonArm;
};

struct CustomModel {
	struct Model model;
	char name[STRING_SIZE + 1];
	cc_bool registered, defined;
	cc_uint8 curPartIndex;

	float nameY;
	float eyeY;
	Vec3 collisionBounds;
	struct AABB pickingBoundsAABB;

	cc_uint16 uScale;
	cc_uint16 vScale;

	cc_uint8 numParts;
	cc_uint8 numArmParts;
	struct CustomModelPart parts[MAX_CUSTOM_MODEL_PARTS];
};

struct CustomModel* CustomModel_Get(int id);
void CustomModel_BuildPart(struct CustomModel* cm, struct CustomModelPartDef* part);
void CustomModel_Register(struct CustomModel* cm);
void CustomModel_Undefine(struct CustomModel* cm);

CC_END_HEADER
#endif
