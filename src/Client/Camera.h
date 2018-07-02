#ifndef CC_CAMERA_H
#define CC_CAMERA_H
#include "Picking.h"

/* Represents a camera, may be first or third person.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

Matrix Camera_TiltM;
Real32 Camera_BobbingVer, Camera_BobbingHor;

typedef struct Camera_ {
	bool IsThirdPerson;
	void (*GetProjection)(Matrix* proj);
	void (*GetView)(Matrix* view);

	Vector2 (*GetOrientation)(void);
	Vector3 (*GetPosition)(Real32 t);
	Vector3 (*GetTarget)(void);

	void (*UpdateMouse)(void);
	void (*RegrabMouse)(void);

	void (*GetPickedBlock)(PickedPos* pos);
	bool (*Zoom)(Real32 amount);
} Camera;

Camera* Camera_Active;
void Camera_Init(void);
void Camera_CycleActive(void);
#endif