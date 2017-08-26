#ifndef CS_CAMERA_H
#define CS_CAMERA_H
#include "Typedefs.h"
#include "Vectors.h"
#include "Picking.h"

/* Represents a camera, may be first or third person.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Tilt matrix applied to the active camera. */
Matrix Camera_TiltM;
/* Bobbing applied to the active camera. */
Real32 Camera_BobbingVer, Camera_BobbingHor;

typedef struct Camera_ {
	/* Whether this camera is a third person camera.
	NOTE: Causes the player's own entity model to be renderered if true.*/
	bool IsThirdPerson;

	/* Calculates the projection matrix for this camera. */
	void (*GetProjection)(Matrix* proj);
	/* Calculates the world/view matrix for this camera. */
	void (*GetView)(Matrix* view);
	/* Calculates the location of the camera's position in the world. */
	Vector3 (*GetCameraPos)(Real32 t);
	/* Calculates the yaw and pitch of the camera in radians. */
	Vector2 (*GetCameraOrientation)(void);

	/* Called every frame for the camera to update its state.
	e.g. Perspective cameras adjusts player's rotatation using delta between mouse position and window centre. */
	void (*UpdateMouse)(void);
	/* Called after the camera has regrabbed the mouse from 2D input handling.
	e.g. Perspective cameras set the mouse position to window centre. */
	void (*RegrabMouse)(void);

	/* Calculates the picked block based on the camera's current state. */
	void (*GetPickedBlock)(PickedPos* pos);
	/* Attempts to zoom the camera's position closer to or further from its origin point.
	returns true if this camera handled zooming (Such as third person cameras) */
	bool (*Zoom)(Real32 amount);
} Camera;

/* Adjusts head X rotation of the player to avoid looking straight up or down.
Returns the resulting angle in radians.
NOTE: looking straight up or down (parallel to camera up vector) can otherwise cause rendering issues. */
Real32 Camera_AdjustHeadX(Real32 degrees);

/* Returns the active camera instance. */
Camera* Camera_ActiveCamera;

/* Initalises and adds the default cameras. */
void Camera_Init(void);

/* Cycles the active camera to the next allowed camera type. */
void Camera_CycleActive(void);
#endif