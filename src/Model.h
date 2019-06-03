#ifndef CC_MODEL_H
#define CC_MODEL_H
#include "Vectors.h"
#include "PackedCol.h"
#include "Constants.h"
#include "VertexStructs.h"
/* Contains various structs and methods for an entity model.
   Also contains a list of models and default textures for those models.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct Entity;
struct AABB;
struct IGameComponent;
extern struct IGameComponent Models_Component;

#define MODEL_QUAD_VERTICES 4
#define MODEL_BOX_VERTICES (FACE_COUNT * MODEL_QUAD_VERTICES)
enum RotateOrder { ROTATE_ORDER_ZYX, ROTATE_ORDER_XZY, ROTATE_ORDER_YZX };

/* Describes a vertex within a model. */
struct ModelVertex { float X, Y, Z; uint16_t U, V; };
static CC_INLINE void ModelVertex_Init(struct ModelVertex* vertex, float x, float y, float z, int u, int v) {
	vertex->X = x; vertex->Y = y; vertex->Z = z;
	vertex->U = u; vertex->V = v;
}

/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
struct ModelPart { uint16_t Offset, Count; float RotX, RotY, RotZ; };
static CC_INLINE void ModelPart_Init(struct ModelPart* part, int offset, int count, float rotX, float rotY, float rotZ) {
	part->Offset = offset; part->Count = count;
	part->RotX = rotX; part->RotY = rotY; part->RotZ = rotZ;
}

struct ModelTex;
/* Contains information about a texture used for models. */
struct ModelTex { const char* Name; uint8_t SkinType; GfxResourceID TexID; struct ModelTex* Next; };

struct Model;
/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
struct Model {
	/* Name of this model */
	const char* Name;
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
	uint8_t armX, armY; /* these translate arm model part back to (0, 0) */

	bool initalised;
	/* Whether the model should be slightly bobbed up and down when rendering. */
	/* e.g. for HumanoidModel, when legs are at the peak of their swing, whole model is moved slightly down */
	bool Bobbing;
	bool UsesSkin, CalcHumanAnims, UsesHumanSkin, Pushes;

	float Gravity; Vector3 Drag, GroundFriction;

	/* Returns the transformation matrix applied to the model when rendering. */
	/* NOTE: Most models just use Entity_GetTransform (except SittingModel) */
	void (*GetTransform)(struct Entity* entity, Vector3 pos, struct Matrix* m);
	void (*DrawArm)(struct Entity* entity);

	float MaxScale, ShadowScale, NameScale;
	struct Model* Next;
};
#if 0
public CustomModel[] CustomModels = new CustomModel[256];
#endif

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
	uint8_t Rotation;
	/* Skin type of current skin texture. */
	uint8_t skinType;
	/* Whether to render arms like vanilla Minecraft Classic. */
	bool ClassicArms;
	/* Model currently being built or rendered. */
	struct Model* Active;
	/* Dynamic vertex buffer for uploading model vertices. */
	GfxResourceID Vb;
	/* Temporary storage for vertices. */
	VertexP3fT2fC4b* Vertices;
	/* Maximum number of vertices that can be stored in Vertices. */
	/* NOTE: If you change this, you MUST also destroy and recreate the dynamic VB. */
	int MaxVertices;
	/* Pointer to humanoid/human model.*/
	struct Model* Human;
} Models;

/* Initialises fields of a model to default. */
CC_API void Model_Init(struct Model* model);

/* Whether the bounding sphere of the model is currently visible. */
bool Model_ShouldRender(struct Entity* entity);
/* Approximately how far the given entity is away from the player. */
float Model_RenderDistance(struct Entity* entity);
/* Draws the given entity as the given model. */
CC_API void Model_Render(struct Model* model, struct Entity* entity);
/* Sets up state to be suitable for rendering the given model. */
/* NOTE: Model_Render already calls this, you don't normally need to call this. */
CC_API void Model_SetupState(struct Model* model, struct Entity* entity);
/* Flushes buffered vertices to the GPU. */
CC_API void Model_UpdateVB(void);
/* Applies the skin texture of the given entity to the model. */
/* Uses model's default texture if the entity doesn't have a custom skin. */
CC_API void Model_ApplyTexture(struct Entity* entity);
/* Draws the given part with no part-specific rotation (e.g. torso). */
CC_API void Model_DrawPart(struct ModelPart* part);
/* Draws the given part with rotation around part's rotation origin. (e.g. arms, head) */
CC_API void Model_DrawRotate(float angleX, float angleY, float angleZ, struct ModelPart* part, bool head);
/* Renders the 'arm' of a model. */
void Model_RenderArm(struct Model* model, struct Entity* entity);
/* Draws the given part with appropriate rotation to produce an arm look. */
CC_API void Model_DrawArmPart(struct ModelPart* part);

/* Returns a pointer to the model whose name caselessly matches given name. */
CC_API struct Model* Model_Get(const String* name);
/* Returns index of the model texture whose name caselessly matches given name. */
CC_API struct ModelTex* Model_GetTexture(const String* name);
/* Adds a model to the list of models. (e.g. "skeleton") */
/* Models can be applied to entities to change their appearance. Use Entity_SetModel for that. */
CC_API void Model_Register(struct Model* model);
/* Adds a texture to the list of model textures. (e.g. "skeleton.png") */
/* Model textures are automatically loaded from texture packs. Used as a 'default skin' for models. */
CC_API void Model_RegisterTexture(struct ModelTex* tex);

/* Describes data for a box being built. */
struct BoxDesc {
	uint16_t TexX, TexY;         /* Texture origin */
	uint8_t SizeX, SizeY, SizeZ; /* Texture dimensions */
	float X1,Y1,Z1, X2,Y2,Z2;    /* Box corners coordinates */
	float RotX,RotY,RotZ;        /* Rotation origin point */
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

CC_API void BoxDesc_XQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float z1, float z2, float y1, float y2, float x, bool swapU);
CC_API void BoxDesc_YQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float z1, float z2, float y, bool swapU);
CC_API void BoxDesc_ZQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float y1, float y2, float z, bool swapU);
#endif
