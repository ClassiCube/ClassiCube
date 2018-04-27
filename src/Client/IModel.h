#ifndef CC_MODEL_H
#define CC_MODEL_H
#include "Vectors.h"
#include "PackedCol.h"
#include "Constants.h"
/* Contains various structs and methods for an entity model.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct Entity_ Entity;
typedef struct AABB_ AABB;

#define IMODEL_QUAD_VERTICES 4
#define IMODEL_BOX_VERTICES (FACE_COUNT * IMODEL_QUAD_VERTICES)

#define ROTATE_ORDER_ZYX 0
#define ROTATE_ORDER_XZY 1
#define ROTATE_ORDER_YZX 2

/* Describes a vertex within a model. */
typedef struct ModelVertex_ { Real32 X, Y, Z; UInt16 U, V; } ModelVertex;
void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, Int32 u, Int32 v);

/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
typedef struct ModelPart_ { UInt16 Offset, Count; Real32 RotX, RotY, RotZ; } ModelPart;
void ModelPart_Init(ModelPart* part, Int32 offset, Int32 count, Real32 rotX, Real32 rotY, Real32 rotZ);

/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
typedef struct IModel_ {
	/* Pointer to the raw vertices of the model.*/
	ModelVertex* vertices;
	/* Count of assigned vertices within the raw vertices array. */
	Int32 index;
	/* Index within ModelCache's textures of the default texture for this model. */
	Int8 defaultTexIndex;

	/* Whether the vertices of this model have actually been filled. */
	bool initalised;
	/* Whether the entity should be slightly bobbed up and down when rendering.
	e.g. for players when their legs are at the peak of their swing, the whole model will be moved slightly down. */
	bool Bobbing;
	bool UsesSkin, CalcHumanAnims, UsesHumanSkin, Pushes;
	UInt8 SurvivalScore;

	Real32 Gravity; Vector3 Drag, GroundFriction;

	Real32 (*GetEyeY)(Entity* entity);
	Vector3 (*GetCollisionSize)(void);
	void (*GetPickingBounds)(AABB* bb);
	void (*CreateParts)(void);
	void (*DrawModel)(Entity* entity);
	void (*GetTransform)(Entity* entity, Vector3 pos);
	/* Recalculates properties such as name Y offset, collision size. 
	Not used by majority of models. (BlockModel is the exception).*/
	void (*RecalcProperties)(Entity* entity);

	Real32 NameYOffset, MaxScale, ShadowScale, NameScale;
} IModel;

PackedCol IModel_Cols[FACE_COUNT];
Real32 IModel_uScale, IModel_vScale;
/* Angle of offset from head to body rotation. */
Real32 IModel_cosHead, IModel_sinHead;
UInt8 IModel_Rotation;
IModel* IModel_ActiveModel;
void IModel_Init(IModel* model);

#define IModel_SetPointers(typeName)\
typeName.GetEyeY = typeName ## _GetEyeY;\
typeName.GetCollisionSize = typeName ## _GetCollisionSize; \
typeName.GetPickingBounds = typeName ## _GetPickingBounds;\
typeName.CreateParts = typeName ## _CreateParts;\
typeName.DrawModel = typeName ## _DrawModel;

bool IModel_ShouldRender(Entity* entity);
Real32 IModel_RenderDistance(Entity* entity);
void IModel_Render(IModel* model, Entity* entity);
void IModel_SetupState(IModel* model, Entity* entity);
void IModel_UpdateVB(void);
GfxResourceID IModel_GetTexture(Entity* entity);
void IModel_DrawPart(ModelPart part);
void IModel_DrawRotate(Real32 angleX, Real32 angleY, Real32 angleZ, ModelPart part, bool head);

/* Describes data for a box being built. */
typedef struct BoxDesc_ {
	/* Texture coordinates and dimensions. */
	Int32 TexX, TexY, SidesW, BodyW, BodyH;
	/* Box corner coordinates. */
	Real32 X1, X2, Y1, Y2, Z1, Z2;
	/* Coordinate around which this box is rotated. */
	Real32 RotX, RotY, RotZ;
} BoxDesc;

/* Sets the texture origin for this part within the texture atlas. */
void BoxDesc_TexOrigin(BoxDesc* desc, Int32 x, Int32 y);
/* Sets the the two corners of this box, in pixel coordinates. */
void BoxDesc_SetBounds(BoxDesc* desc, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2);
/* Expands the corners of this box outwards by the given amount in pixel coordinates. */
void BoxDesc_Expand(BoxDesc* desc, Real32 amount);
/* Scales the corners of this box outwards by the given amounts. */
void BoxDesc_Scale(BoxDesc* desc, Real32 scale);
/* Sets the coordinate that this box is rotated around, in pixel coordinates. */
void BoxDesc_RotOrigin(BoxDesc* desc, Int8 x, Int8 y, Int8 z);
/* Swaps the min and max X around, resulting in the part being drawn mirrored. */
void BoxDesc_MirrorX(BoxDesc* desc);

/* Constructs a description of the given box, from two corners.
See BoxDesc_BuildBox for details on texture layout. */
void BoxDesc_Box(BoxDesc* desc, Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2);
/* Constructs a description of the given rotated box, from two corners.
See BoxDesc_BuildRotatedBox for details on texture layout. */
void BoxDesc_RotatedBox(BoxDesc* desc, Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2);

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
void BoxDesc_BuildBox(ModelPart* part, IModel* m, BoxDesc* desc);

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
void BoxDesc_BuildRotatedBox(ModelPart* part, IModel* m, BoxDesc* desc);

void BoxDesc_XQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 z1, Real32 z2, Real32 y1, Real32 y2, Real32 x, bool swapU);
void BoxDesc_YQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 z1, Real32 z2, Real32 y, bool swapU);
void BoxDesc_ZQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 y1, Real32 y2, Real32 z, bool swapU);
#endif