#ifndef CC_CAMERA_H
#define CC_CAMERA_H
#include "Vectors.h"

/* Represents a camera, may be first or third person.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct PickedPos;
struct Camera;

/* Shared data for cameras. */
extern struct _CameraData {
	/* How sensitive camera is to movements of mouse. */
	int Sensitivity;
	/* Whether smooth/cinematic camera mode is used. */
	cc_bool Smooth;
	/* Whether third person camera clip against blocks. */
	cc_bool Clipping;
	/* Whether to invert vertical mouse movement. */
	cc_bool Invert;
	/* The mass (i.e. smoothness) of the smooth camera. */
	float Mass;

	/* Tilt effect applied to the camera. */
	struct Matrix TiltM;
	/* Bobbing offset of camera from player's eye. */
	float BobbingVer, BobbingHor;

	/* Cached position the camera is at. */
	Vec3 CurrentPos;
	/* Camera user is currently using. */
	struct Camera* Active;
} Camera;

struct Camera {
	/* Whether this camera is third person. (i.e. not allowed when -thirdperson in MOTD) */
	cc_bool isThirdPerson;

	/* Calculates the current projection matrix of this camera. */
	void (*GetProjection)(struct Matrix* proj);
	/* Calculates the current modelview matrix of this camera. */
	void (*GetView)(struct Matrix* view);

	/* Returns the current orientation of the camera. */
	Vec2 (*GetOrientation)(void);
	/* Returns the current interpolated position of the camera. */
	Vec3 (*GetPosition)(float t);

	/* Called to update the camera's state. */
	/* Typically, this is used to adjust yaw/pitch based on accumulated mouse movement. */
	void (*UpdateMouse)(double delta);
	/* Called when mouse has moved. */
	void (*OnRawMouseMoved)(int deltaX, int deltaY);
	/* Called when user closes all menus, and is interacting with camera again. */
	void (*AcquireFocus)(void);
	/* Called when user is no longer interacting with camera. (e.g. opened menu) */
	void (*LoseFocus)(void);

	/* Calculates selected block in the world, based on camera's current state */
	void (*GetPickedBlock)(struct PickedPos* pos);
	/* Zooms the camera in or out when scrolling mouse wheel. */
	cc_bool (*Zoom)(float amount);

	/* Next camera in linked list of cameras. */
	struct Camera* next;
};

/* Initialises the default cameras. */
void Camera_Init(void);
/* Switches to next camera in the list of cameras. */
void Camera_CycleActive(void);
/* Registers a camera for use. */
CC_API void Camera_Register(struct Camera* camera);
/* Checks whether camera is still focused or not. */
/* If focus changes, calls AcquireFocus or LoseFocus */
void Camera_CheckFocus(void);
#endif
