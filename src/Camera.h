#ifndef CC_CAMERA_H
#define CC_CAMERA_H
#include "Vectors.h"

/* Represents a camera, may be first or third person.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct PickedPos;
struct Camera;

/* Shared data for cameras. */
struct _CameraData {
	/* How sensitive camera is to movements of mouse. */
	int Sensitivity;
	/* Whether smooth/cinematic camera mode is used. */
	bool Smooth;
	/* Whether third person camera clip against blocks. */
	bool Clipping;
	/* Whether to invert vertical mouse movement. */
	bool Invert;

	/* Tilt effect applied to the camera. */
	struct Matrix TiltM;
	/* Bobbing offset of camera from player's eye. */
	float BobbingVer, BobbingHor;

	/* Cached position the camera is at. */
	Vector3 CurrentPos;
	/* Camera user is currently using. */
	struct Camera* Active;
} Camera;

struct Camera {
	/* Whether this camera is third person. (i.e. not allowed when -thirdperson in MOTD) */
	bool IsThirdPerson;

	/* Calculates the current projection matrix of this camera. */
	void (*GetProjection)(struct Matrix* proj);
	/* Calculates the current modelview matrix of this camera. */
	void (*GetView)(struct Matrix* view);

	/* Returns the current orientation of the camera. */
	Vector2 (*GetOrientation)(void);
	/* Returns the current interpolated position of the camera. */
	Vector3 (*GetPosition)(float t);

	/* Called to update the camera's state. */
	/* Typically, this is used to adjust yaw/pitch based on mouse movement. */
	void (*UpdateMouse)(float delta);
	/* Called when user closes all menus, and is interacting with camera again. */
	/* Typically, this is used to move mouse cursor to centre of the window. */
	void (*RegrabMouse)(void);

	/* Calculates selected block in the world, based on camera's current state */
	void (*GetPickedBlock)(struct PickedPos* pos);
	/* Zooms the camera in or out when scrolling mouse wheel. */
	bool (*Zoom)(float amount);

	/* Next camera in linked list of cameras. */
	struct Camera* Next;
};

/* Initialises the default cameras. */
void Camera_Init(void);
/* Switches to next camera in the list of cameras. */
void Camera_CycleActive(void);
/* Registers a camera for use. */
CC_API void Camera_Register(struct Camera* camera);
#endif
