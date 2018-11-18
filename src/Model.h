#ifndef CC_MODEL_H
#define CC_MODEL_H
#include "Vectors.h"
#include "PackedCol.h"
#include "Constants.h"
/* Contains various structs and methods for an entity model.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Entity;
struct AABB;

#define MODEL_QUAD_VERTICES 4
#define MODEL_BOX_VERTICES (FACE_COUNT * MODEL_QUAD_VERTICES)
enum ROTATE_ORDER { ROTATE_ORDER_ZYX, ROTATE_ORDER_XZY, ROTATE_ORDER_YZX };

/* Describes a vertex within a model. */
struct ModelVertex { float X, Y, Z; uint16_t U, V; };
void ModelVertex_Init(struct ModelVertex* vertex, float x, float y, float z, int u, int v);

/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
struct ModelPart { uint16_t Offset, Count; float RotX, RotY, RotZ; };
void ModelPart_Init(struct ModelPart* part, int offset, int count, float rotX, float rotY, float rotZ);

/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
struct Model {
	/* Pointer to the raw vertices of the model */
	struct ModelVertex* vertices;
	/* Count of assigned vertices within the raw vertices array */
	int index;
	/* Index within ModelCache's textures of the default texture for this model */
	int8_t defaultTexIndex;
	uint8_t armX, armY; /* these translate arm model part back to (0, 0) */

	bool initalised;
	/* Whether the model should be slightly bobbed up and down when rendering */
	/* e.g. for HumanoidModel, when legs are at the peak of their swing, whole model is moved slightly down */
	bool Bobbing;
	bool UsesSkin, CalcHumanAnims, UsesHumanSkin, Pushes;

	float Gravity; Vector3 Drag, GroundFriction;

	float (*GetEyeY)(struct Entity* entity);
	void (*GetCollisionSize)(Vector3* size);
	void (*GetPickingBounds)(struct AABB* bb);
	void (*CreateParts)(void);
	void (*DrawModel)(struct Entity* entity);
	/* Returns the transformation matrix applied to the model when rendering */
	/* NOTE: Most models just use Entity_GetTransform (except SittingModel) */
	void (*GetTransform)(struct Entity* entity, Vector3 pos, struct Matrix* m);
	/* Recalculates properties such as name Y offset, collision size */
	/* NOTE: Not needed for most models (except BlockModel) */
	void (*RecalcProperties)(struct Entity* entity);
	void (*DrawArm)(struct Entity* entity);

	float NameYOffset, MaxScale, ShadowScale, NameScale;
};

PackedCol Model_Cols[FACE_COUNT];
/* U/V scale applied to the skin when rendering the model. */
/* Default uScale is 1/32, vScale is 1/32 or 1/64 depending on skin. */
float Model_uScale, Model_vScale;
/* Angle of offset of head to body rotation */
float Model_cosHead, Model_sinHead;
uint8_t Model_Rotation, Model_skinType;
struct Model* Model_ActiveModel;
void Model_Init(struct Model* model);

#define Model_SetPointers(typeName)\
typeName.GetEyeY = typeName ## _GetEyeY;\
typeName.GetCollisionSize = typeName ## _GetCollisionSize; \
typeName.GetPickingBounds = typeName ## _GetPickingBounds;\
typeName.CreateParts = typeName ## _CreateParts;\
typeName.DrawModel = typeName ## _DrawModel;

bool Model_ShouldRender(struct Entity* entity);
float Model_RenderDistance(struct Entity* entity);
void Model_Render(struct Model* model, struct Entity* entity);
void Model_SetupState(struct Model* model, struct Entity* entity);
void Model_UpdateVB(void);
void Model_ApplyTexture(struct Entity* entity);
void Model_DrawPart(struct ModelPart* part);
void Model_DrawRotate(float angleX, float angleY, float angleZ, struct ModelPart* part, bool head);
void Model_RenderArm(struct Model* model, struct Entity* entity);
void Model_DrawArmPart(struct ModelPart* part);

/* Describes data for a box being built. */
struct BoxDesc {
	uint16_t TexX, TexY;         /* Texture origin */
	uint8_t SizeX, SizeY, SizeZ; /* Texture dimensions */
	float X1,Y1,Z1, X2,Y2,Z2;    /* Box corners coordinates */
	float RotX,RotY,RotZ;        /* Rotation origin point */
};

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
void BoxDesc_BuildBox(struct ModelPart* part, const struct BoxDesc* desc);

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
void BoxDesc_BuildRotatedBox(struct ModelPart* part, const struct BoxDesc* desc);

void BoxDesc_XQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float z1, float z2, float y1, float y2, float x, bool swapU);
void BoxDesc_YQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float z1, float z2, float y, bool swapU);
void BoxDesc_ZQuad(struct Model* m, int texX, int texY, int texWidth, int texHeight, float x1, float x2, float y1, float y2, float z, bool swapU);
#endif
