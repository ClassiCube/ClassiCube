#ifndef CS_MODEL_H
#define CS_MODEL_H
#include "Typedefs.h"
#include "Vectors.h"
#include "PackedCol.h"
#include "BlockEnums.h"
#include "Entity.h"
#include "AABB.h"
#include "GraphicsEnums.h"
/* Contains various structs and methods for an entity model.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define IModel_QuadVertices 4
#define IModel_BoxVertices (Face_Count * IModel_QuadVertices)

/* Order in which axis rotations are applied to a part. */
typedef Int32 RotateOrder;
#define RotateOrder_ZYX 0
#define RotateOrder_XZY 1

/* Skin layout a humanoid skin texture can have. */
typedef Int32 SkinType;
#define SkinType_64x32 0
#define SkinType_64x64 1
#define SkinType_64x64Slim 2
#define SkinType_Invalid 3


/* Describes a vertex within a model. */
typedef struct ModelVertex {
	Real32 X, Y, Z;
	UInt16 U, V;
} ModelVertex;

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, UInt16 u, UInt16 v);


/* Describes the starting index of this part within a model's array of vertices,
and the number of vertices following the starting index that this part uses. */
typedef struct ModelPart {
	Int32 Offset, Count;
	Real32 RotX, RotY, RotZ;
} ModelPart;

void ModelPart_Init(ModelPart* part, Int32 offset, Int32 count, Real32 rotX, Real32 rotY, Real32 rotZ);


/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
typedef struct IModel {

	/* Pointer to the raw vertices of the model.*/
	ModelVertex* vertices;

	/* Count of assigned vertices within the raw vertices array. */
	Int32 index;

	/* Index within ModelCache's textures of the default texture for this model. */
	Int32 defaultTexIndex;

	/* Whether the vertices of this model have actually been filled. */
	bool initalised;

	/* Specifies order in which axis roations are applied to a part. */
	RotateOrder Rotation;


	/* Whether the entity should be slightly bobbed up and down when rendering.
	e.g. for players when their legs are at the peak of their swing, the whole model will be moved slightly down. */
	bool Bobbing;

	/* Whether this entity requires downloading of a skin texture.*/
	bool UsesSkin;

	/* Whether humanoid animations should be calculated, instead of normal animations.*/
	bool CalcHumanAnims;


	/* Gravity applied to this entity.*/
	Real32 Gravity;

	/* Drag applied to the entity.*/
	Vector3 Drag;

	/* Friction applied to the entity when is on the ground.*/
	Vector3 GroundFriction;


	/* Vertical offset from the model's feet/base that the name texture should be drawn at. */
	Real32 (*GetNameYOffset)(void);

	/* Vertical offset from the model's feet/base that the model's eye is located. */
	Real32 (*GetEyeY)(Entity* entity);

	/* The size of the bounding box that is used when performing collision detection for this model. */
	Vector3 (*GetCollisionSize)(void);

	/* Bounding box that contains this model, assuming that the model is not rotated at all. */
	AABB (*GetPickingBounds)(void);

	/* Fills out the vertices of this model. */
	void (*CreateParts)(void);

	/* Performs the actual rendering of an entity model. */
	void (*DrawModel)(Entity* entity);


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

/* Pointer to model that is currently being rendered/drawn. */
IModel* IModel_ActiveModel;


/* Sets default values for fields of a model. */
void IModel_Init(IModel* model);

/* Sets the function pointers for a model instance assuming typeName_XYZ naming. */
#define IModel_SetFuncPointers(typeName)\
typeName.CreateParts = typeName ## _CreateParts;\
typeName.vertices = typeName ## _Vertices;\
typeName.GetNameYOffset = typeName ## _GetNameYOffset;\
typeName.GetCollisionSize = typeName ## _GetCollisionSize;\
typeName.GetPickingBounds = typeName ## _GetPickingBounds;\
typeName.DrawModel = typeName ## _DrawModel;


/* Returns whether the model should be rendered based on the given entity's position. */
bool IModel_ShouldRender(Entity* entity);

/* Returns the closest distance of the given entity to the camera. */
Real32 IModel_RenderDistance(Entity* entity);

static Real32 IModel_MinDist(Real32 dist, Real32 extent);

/*Sets up the state for, then renders an entity model, based on the entity's position and orientation. */
void IModel_Render(IModel* model, Entity entity);

void IModel_SetupState(Entity* p);

/* Sends the updated vertex data to the GPU. */
void IModel_UpdateVB();

/* Gets the appropriate native texture ID for the given model.
if pTex is > 0 then it is returned. Otherwise ID of the model's default texture is returned. */
GfxResourceID IModel_GetTexture(IModel* model, GfxResourceID pTex);

void IModel_DrawPart(ModelPart part);

void IModel_DrawRotate(Real32 angleX, Real32 angleY, Real32 angleZ, ModelPart part, bool head);
#endif