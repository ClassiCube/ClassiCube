#ifndef CS_MODEL_H
#define CS_MODEL_H
#include "Typedefs.h"
#include "Vectors.h"
#include "PackedCol.h"
#include "BlockEnums.h"
#include "Entity.h"
#include "AABB.h"
/* Containts various structs and methods for an entity model.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Describes a vertex within a model. */
typedef struct ModelVertex {
	Real32 X, Y, Z;
	UInt16 U, V;
} ModelVertex;

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, UInt16 u, UInt16 v);


/* Contains a set of quads and/or boxes that describe a 3D object as well as
the bounding boxes that contain the entire set of quads and/or boxes. */
typedef struct IModel {

	/* Pointer to the raw vertices of the model.*/
	ModelVertex* vertices;
	/* Count of assigned vertices within the raw vertices array. */
	Int32 index;
	/* Index within ModelCache's textures of the default texture for this model. */
	Int32 defaultTexIndex;


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
	Real32 (*GetNameYOffset);

	/* Vertical offset from the model's feet/base that the model's eye is located. */
	Real32 (*GetEyeY)(Entity* entity);

	/*The size of the bounding box that is used when performing collision detection for this model. */
	Vector3 (*GetCollisionSize);

	/* Bounding box that contains this model, assuming that the model is not rotated at all. */
	AABB (*GetPickingBounds);

	/* Fills out the vertices of this model. */
	void (*CreateParts)(void);

	/* Performs the actual rendering of an entity model. */
	void (*DrawModel)(Entity* entity);


	/* The maximum scale the entity can have (for collisions and rendering).*/
	Real32 MaxScale;

	/* Scaling factor applied, multiplied by the entity's current model scale.*/
	Real32 ShadowScale;

	/*  Scaling factor applied, multiplied by the entity's current model scale.*/
	Real32 NameScale;

} IModel;

void IModel_Init(IModel* model);
#endif