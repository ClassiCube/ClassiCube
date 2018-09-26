#ifndef CC_CAMERA_H
#define CC_CAMERA_H
#include "Picking.h"

/* Represents a camera, may be first or third person.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct Matrix Camera_TiltM;
float Camera_BobbingVer, Camera_BobbingHor;

struct Camera {
	bool IsThirdPerson;
	void (*GetProjection)(struct Matrix* proj);
	void (*GetView)(struct Matrix* view);

	Vector2 (*GetOrientation)(void);
	Vector3 (*GetPosition)(float t);

	void (*UpdateMouse)(void);
	void (*RegrabMouse)(void);

	void (*GetPickedBlock)(struct PickedPos* pos);
	bool (*Zoom)(float amount);
};

struct Camera* Camera_Active;
void Camera_Init(void);
void Camera_CycleActive(void);
#endif
