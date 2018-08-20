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

#define IMODEL_QUAD_VERTICES 4
#define IMODEL_BOX_VERTICES (FACE_COUNT * IMODEL_QUAD_VERTICES)
enum ROTATE_ORDER { ROTATE_ORDER_ZYX, ROTATE_ORDER_XZY, ROTATE_ORDER_YZX };

/* Describes a vertex within a model. */
struct ModelVertex { Real32 X, Y, Z; UInt16 U, V; };
void ModelVertex_Init(struct ModelVertex* vertex, Real32 x, Real32 y, Real32 z, Int32 u, Int32 v);

/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
struct ModelPart { UInt16 Offset, Count; Real32 RotX, RotY, RotZ; };
void ModelPart_Init(struct ModelPart* part, Int32 offset, Int32 count, Real32 rotX, Real32 rotY, Real32 rotZ);

/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
struct IModel {
	/* Pointer to the raw vertices of the model.*/
	struct ModelVertex* vertices;
	/* Count of assigned vertices within the raw vertices array. */
	Int32 index;
	/* Index within ModelCache's textures of the default texture for this model. */
	Int8 defaultTexIndex;
	UInt8 armX, armY; /* these translate arm model part back to (0, 0) */

	bool initalised;
	/* Whether the entity should be slightly bobbed up and down when rendering.
	e.g. for players when their legs are at the peak of their swing, the whole model will be moved slightly down. */
	bool Bobbing;
	bool UsesSkin, CalcHumanAnims, UsesHumanSkin, Pushes;

	Real32 Gravity; Vector3 Drag, GroundFriction;

	Real32 (*GetEyeY)(struct Entity* entity);
	void (*GetCollisionSize)(Vector3* size);
	void (*GetPickingBounds)(struct AABB* bb);
	void (*CreateParts)(void);
	void (*DrawModel)(struct Entity* entity);
	void (*GetTransform)(struct Entity* entity, Vector3 pos, struct Matrix* m);
	/* Recalculates properties such as name Y offset, collision size. 
	Not used by majority of models. (BlockModel is the exception).*/
	void (*RecalcProperties)(struct Entity* entity);
	void (*DrawArm)(struct Entity* entity);

	Real32 NameYOffset, MaxScale, ShadowScale, NameScale;
};

PackedCol IModel_Cols[FACE_COUNT];
Real32 IModel_uScale, IModel_vScale;
/* Angle of offset from head to body rotation. */
Real32 IModel_cosHead, IModel_sinHead;
UInt8 IModel_Rotation, IModel_skinType;
struct IModel* IModel_ActiveModel;
void IModel_Init(struct IModel* model);

#define IModel_SetPointers(typeName)\
typeName.GetEyeY = typeName ## _GetEyeY;\
typeName.GetCollisionSize = typeName ## _GetCollisionSize; \
typeName.GetPickingBounds = typeName ## _GetPickingBounds;\
typeName.CreateParts = typeName ## _CreateParts;\
typeName.DrawModel = typeName ## _DrawModel;

bool IModel_ShouldRender(struct Entity* entity);
Real32 IModel_RenderDistance(struct Entity* entity);
void IModel_Render(struct IModel* model, struct Entity* entity);
void IModel_SetupState(struct IModel* model, struct Entity* entity);
void IModel_UpdateVB(void);
void IModel_ApplyTexture(struct Entity* entity);
void IModel_DrawPart(struct ModelPart* part);
void IModel_DrawRotate(Real32 angleX, Real32 angleY, Real32 angleZ, struct ModelPart* part, bool head);
void IModel_RenderArm(struct IModel* model, struct Entity* entity);
void IModel_DrawArmPart(struct ModelPart* part);

/* Describes data for a box being built. */
struct BoxDesc {
	/* Texture coordinates and dimensions. */
	Int32 TexX, TexY, SizeZ, SizeX, SizeY;
	/* Box corner coordinates. */
	Real32 X1, X2, Y1, Y2, Z1, Z2;
	/* Coordinate around which this box is rotated. */
	Real32 RotX, RotY, RotZ;
};

/* Sets the texture origin for this part within the texture atlas. */
void BoxDesc_TexOrigin(struct BoxDesc* desc, Int32 x, Int32 y);
/* Sets the the two corners of this box, in pixel coordinates. */
void BoxDesc_SetBounds(struct BoxDesc* desc, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2);
/* Expands the corners of this box outwards by the given amount in pixel coordinates. */
void BoxDesc_Expand(struct BoxDesc* desc, Real32 amount);
/* Scales the corners of this box outwards by the given amounts. */
void BoxDesc_Scale(struct BoxDesc* desc, Real32 scale);
/* Sets the coordinate that this box is rotated around, in pixel coordinates. */
void BoxDesc_RotOrigin(struct BoxDesc* desc, Int8 x, Int8 y, Int8 z);
/* Swaps the min and max X around, resulting in the part being drawn mirrored. */
void BoxDesc_MirrorX(struct BoxDesc* desc);

/* Constructs a description of the given box, from two corners. */
void BoxDesc_Box(struct BoxDesc* desc, Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2);

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
void BoxDesc_BuildBox(struct ModelPart* part, struct BoxDesc* desc);

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
void BoxDesc_BuildRotatedBox(struct ModelPart* part, struct BoxDesc* desc);

void BoxDesc_XQuad(struct IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 z1, Real32 z2, Real32 y1, Real32 y2, Real32 x, bool swapU);
void BoxDesc_YQuad(struct IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 z1, Real32 z2, Real32 y, bool swapU);
void BoxDesc_ZQuad(struct IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 y1, Real32 y2, Real32 z, bool swapU);
#endif
