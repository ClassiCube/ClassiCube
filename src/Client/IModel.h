#ifndef CS_MODEL_H
#define CS_MODEL_H
#include "Typedefs.h"
#include "Vectors.h"
#include "PackedCol.h"
#include "Block.h"
#include "Entity.h"
#include "AABB.h"
#include "GraphicsEnums.h"
/* Contains various structs and methods for an entity model.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct Entity_ Entity; /* Forward declaration */

#define IModel_QuadVertices 4
#define IModel_BoxVertices (Face_Count * IModel_QuadVertices)

/* Order in which axis rotations are applied to a part. */
typedef UInt8 RotateOrder;
#define RotateOrder_ZYX 0
#define RotateOrder_XZY 1
#define RotateOrder_YZX 2

/* Skin layout a humanoid skin texture can have. */
typedef UInt8 SkinType;
#define SkinType_64x32 0
#define SkinType_64x64 1
#define SkinType_64x64Slim 2
#define SkinType_Invalid 3

/* Describes a vertex within a model. */
typedef struct ModelVertex_ {
	Real32 X, Y, Z;
	UInt16 U, V;
} ModelVertex;
void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, Int32 u, Int32 v);

/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
typedef struct ModelPart_ {
	UInt16 Offset, Count;
	Real32 RotX, RotY, RotZ;
} ModelPart;
void ModelPart_Init(ModelPart* part, Int32 offset, Int32 count, Real32 rotX, Real32 rotY, Real32 rotZ);

/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
typedef struct IModel_ {
	/* Pointer to the raw vertices of the model.*/
	ModelVertex* vertices;
	/* Count of assigned vertices within the raw vertices array. */
	Int32 index;
	/* Index within ModelCache's textures of the default texture for this model. */
	Int32 defaultTexIndex;

	/* Whether the vertices of this model have actually been filled. */
	bool initalised;
	/* Whether the entity should be slightly bobbed up and down when rendering.
	e.g. for players when their legs are at the peak of their swing, the whole model will be moved slightly down. */
	bool Bobbing;
	/* Whether this model uses a skin texture at all.
	If false, no attempt is made to download the skin of an entity which has this model. */
	bool UsesSkin;
	/* Whether humanoid animations should be calculated, instead of normal animations.*/
	bool CalcHumanAnims;
	/* Whether the model uses humanoid skin texture, instead of mob skin texture. */
	bool UsesHumanSkin;
	/* Score earned by killing an entity with this model in survival mode. */
	UInt8 SurvivalScore;
	/* Whether this model pushes other models when collided with. */
	bool Pushes;

	/* Gravity applied to this entity.*/
	Real32 Gravity;
	/* Drag applied to the entity.*/
	Vector3 Drag;
	/* Friction applied to the entity when is on the ground.*/
	Vector3 GroundFriction;

	/* Vertical offset from the model's feet/base that the model's eye is located. */
	Real32 (*GetEyeY)(Entity* entity);
	/* The size of the bounding box that is used when performing collision detection for this model. */
	Vector3 (*GetCollisionSize)(void);
	/* Bounding box that contains this model, assuming that the model is not rotated at all. */
	void (*GetPickingBounds)(AABB* bb);
	/* Fills out the vertices of this model. */
	void (*CreateParts)(void);
	/* Performs the actual rendering of an entity model. */
	void (*DrawModel)(Entity* entity);
	/* Gets the transformation matrix of this entity. */
	void (*GetTransform)(Entity* entity, Vector3 pos);
	/* Recalculates properties such as name Y offset, collision size. 
	Not used by majority of models. (BlockModel is the exception).*/
	void (*RecalcProperties)(Entity* entity);

	/* Vertical offset from the model's feet/base that the name texture should be drawn at. */
	Real32 NameYOffset;
	/* The maximum scale the entity can have (for collisions and rendering).*/
	Real32 MaxScale;
	/* Scaling factor applied, multiplied by the entity's current model scale.*/
	Real32 ShadowScale;
	/* Scaling factor applied, multiplied by the entity's current model scale.*/
	Real32 NameScale;
} IModel;

/* Colour tint applied to each face, when rendering a model. */
PackedCol IModel_Cols[Face_Count];
/* Scaling applied to UV coordinates when rendering a model. */
Real32 IModel_uScale, IModel_vScale;
/* Angle of offset from head to body rotation. */
Real32 IModel_cosHead, IModel_sinHead;
/* Specifies order in which axis roations are applied to a part. */
RotateOrder IModel_Rotation;
/* Pointer to model that is currently being rendered/drawn. */
IModel* IModel_ActiveModel;
/* Sets default values for fields of a model. */
void IModel_Init(IModel* model);

/* Sets the data and function pointers for a model instance assuming typeName_XYZ naming. */
#define IModel_SetPointers(typeName)\
typeName.GetEyeY = typeName ## _GetEyeY;\
typeName.GetCollisionSize = typeName ## _GetCollisionSize; \
typeName.GetPickingBounds = typeName ## _GetPickingBounds;\
typeName.CreateParts = typeName ## _CreateParts;\
typeName.DrawModel = typeName ## _DrawModel;

/* Returns whether the model should be rendered based on the given entity's position. */
bool IModel_ShouldRender(Entity* entity);
/* Returns the closest distance of the given entity to the camera. */
Real32 IModel_RenderDistance(Entity* entity);
/*Sets up the state for, then renders an entity model, based on the entity's position and orientation. */
void IModel_Render(IModel* model, Entity* entity);
void IModel_SetupState(IModel* model, Entity* entity);
/* Sends the updated vertex data to the GPU. */
void IModel_UpdateVB(void);
/* Gets the appropriate native texture ID for the given entity and current model. */
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
ModelPart BoxDesc_BuildBox(IModel* m, BoxDesc* desc);

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
ModelPart BoxDesc_BuildRotatedBox(IModel* m, BoxDesc* desc);

void BoxDesc_XQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 z1, Real32 z2, Real32 y1, Real32 y2, Real32 x, bool swapU);
void BoxDesc_YQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 z1, Real32 z2, Real32 y, bool swapU);
void BoxDesc_ZQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 y1, Real32 y2, Real32 z, bool swapU);
#endif