#include "Camera.h"
#include "ExtMath.h"
#include "Game.h"
#include "Window.h"
#include "Graphics.h"
#include "Funcs.h"
#include "Gui.h"
#include "Entity.h"
#include "Input.h"

struct _CameraData Camera;
static struct PickedPos cameraClipPos;
static Vector2 cam_rotOffset;
static bool cam_isForwardThird;
static Point2D cam_delta;

static void Camera_AcquireFocus(void) {
	Window_EnableRawMouse();
}
static void Camera_LoseFocus(void) {
	Window_DisableRawMouse();
}
static void Camera_OnRawMouseMoved(int deltaX, int deltaY) {
	cam_delta.X += deltaX; cam_delta.Y += deltaY;
}

/*########################################################################################################################*
*--------------------------------------------------Perspective camera-----------------------------------------------------*
*#########################################################################################################################*/
static void PerspectiveCamera_GetProjection(struct Matrix* proj) {
	float fovy = Game_Fov * MATH_DEG2RAD;
	float aspectRatio = (float)Game.Width / (float)Game.Height;
	Matrix_PerspectiveFieldOfView(proj, fovy, aspectRatio, Gfx.MinZNear, (float)Game_ViewDistance);
}

static void PerspectiveCamera_GetView(struct Matrix* mat) {
	Vector3 pos = Camera.CurrentPos;
	Vector2 rot = Camera.Active->GetOrientation();
	Matrix_LookRot(mat, pos, rot);
	Matrix_MulBy(mat, &Camera.TiltM);
}

static void PerspectiveCamera_GetPickedBlock(struct PickedPos* pos) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector3 dir    = Vector3_GetDirVector(p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD);
	Vector3 eyePos = Entity_GetEyePosition(p);
	float reach    = LocalPlayer_Instance.ReachDistance;
	Picking_CalculatePickedBlock(eyePos, dir, reach, pos);
}

#define CAMERA_SENSI_FACTOR (0.0002f / 3.0f * MATH_RAD2DEG)
#define CAMERA_SLIPPERY 0.97f
#define CAMERA_ADJUST 0.025f

static Vector2 PerspectiveCamera_GetMouseDelta(double delta) {
	float sensitivity = CAMERA_SENSI_FACTOR * Camera.Sensitivity;
	static float speedX, speedY;
	Vector2 v;

	if (Camera.Smooth) {
		speedX += cam_delta.X * CAMERA_ADJUST;
		speedX *= CAMERA_SLIPPERY;
		speedY += cam_delta.Y * CAMERA_ADJUST;
		speedY *= CAMERA_SLIPPERY;
	} else {
		speedX = (float)cam_delta.X;
		speedY = (float)cam_delta.Y;
	}

	v.X = speedX * sensitivity; v.Y = speedY * sensitivity;
	if (Camera.Invert) v.Y = -v.Y;
	return v;
}

static void PerspectiveCamera_UpdateMouseRotation(double delta) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Entity* e      = &p->Base;

	struct LocationUpdate update;
	float headY, headX;
	Vector2 rot = PerspectiveCamera_GetMouseDelta(delta);

	if (Key_IsAltPressed() && Camera.Active->IsThirdPerson) {
		cam_rotOffset.X += rot.X; cam_rotOffset.Y += rot.Y;
		return;
	}
	
	headY = p->Interp.Next.HeadY + rot.X;
	headX = p->Interp.Next.HeadX + rot.Y;
	LocationUpdate_MakeOri(&update, headY, headX);

	/* Need to make sure we don't cross the vertical axes, because that gets weird. */
	if (update.HeadX >= 90.0f && update.HeadX <= 270.0f) {
		update.HeadX = p->Interp.Next.HeadX < 180.0f ? 90.0f : 270.0f;
	}
	e->VTABLE->SetLocation(e, &update, false);
}

static void PerspectiveCamera_UpdateMouse(double delta) {
	struct Screen* s = Gui_GetActiveScreen();
	if (!s->HandlesAllInput && Window_Focused) Window_UpdateRawMouse();

	PerspectiveCamera_UpdateMouseRotation(delta);
	cam_delta.X = 0; cam_delta.Y = 0;
}

static void PerspectiveCamera_CalcViewBobbing(float t, float velTiltScale) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Entity* e = &p->Base;

	struct Matrix tiltY, velX;
	float vel;
	if (!Game_ViewBobbing) { Camera.TiltM = Matrix_Identity; return; }

	Matrix_RotateZ(&Camera.TiltM, -p->Tilt.TiltX                  * e->Anim.BobStrength);
	Matrix_RotateX(&tiltY,        Math_AbsF(p->Tilt.TiltY) * 3.0f * e->Anim.BobStrength);
	Matrix_MulBy(&Camera.TiltM, &tiltY);

	Camera.BobbingHor = (e->Anim.BobbingHor * 0.3f) * e->Anim.BobStrength;
	Camera.BobbingVer = (e->Anim.BobbingVer * 0.6f) * e->Anim.BobStrength;

	vel = Math_Lerp(p->OldVelocity.Y + 0.08f, e->Velocity.Y + 0.08f, t);
	Matrix_RotateX(&velX, -vel * 0.05f * p->Tilt.VelTiltStrength / velTiltScale);
	Matrix_MulBy(&Camera.TiltM, &velX);
}


/*########################################################################################################################*
*---------------------------------------------------First person camera---------------------------------------------------*
*#########################################################################################################################*/
static Vector2 FirstPersonCamera_GetOrientation(void) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector2 v;	
	v.X = p->HeadY * MATH_DEG2RAD; v.Y = p->HeadX * MATH_DEG2RAD; 
	return v;
}

static Vector3 FirstPersonCamera_GetPosition(float t) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector3 camPos   = Entity_GetEyePosition(p);
	float headY      = p->HeadY * MATH_DEG2RAD;
	PerspectiveCamera_CalcViewBobbing(t, 1);
	
	camPos.Y += Camera.BobbingVer;
	camPos.X += Camera.BobbingHor * (float)Math_Cos(headY);
	camPos.Z += Camera.BobbingHor * (float)Math_Sin(headY);
	return camPos;
}

static bool FirstPersonCamera_Zoom(float amount) { return false; }
static struct Camera cam_FirstPerson = {
	false,
	PerspectiveCamera_GetProjection,  PerspectiveCamera_GetView,
	FirstPersonCamera_GetOrientation, FirstPersonCamera_GetPosition,
	PerspectiveCamera_UpdateMouse,    Camera_OnRawMouseMoved,
	Camera_AcquireFocus,              Camera_LoseFocus,
	PerspectiveCamera_GetPickedBlock, FirstPersonCamera_Zoom,
};


/*########################################################################################################################*
*---------------------------------------------------Third person camera---------------------------------------------------*
*#########################################################################################################################*/
float dist_third = 3.0f, dist_forward = 3.0f;

static Vector2 ThirdPersonCamera_GetOrientation(void) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector2 v;	
	v.X = p->HeadY * MATH_DEG2RAD; v.Y = p->HeadX * MATH_DEG2RAD;
	if (cam_isForwardThird) { v.X += MATH_PI; v.Y = -v.Y; }

	v.X += cam_rotOffset.X * MATH_DEG2RAD; 
	v.Y += cam_rotOffset.Y * MATH_DEG2RAD;
	return v;
}

static Vector3 ThirdPersonCamera_GetPosition(float t) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	float dist = cam_isForwardThird ? dist_forward : dist_third;
	Vector3 target, dir;
	Vector2 rot;

	PerspectiveCamera_CalcViewBobbing(t, dist);
	target = Entity_GetEyePosition(p);
	target.Y += Camera.BobbingVer;

	rot = Camera.Active->GetOrientation();
	dir = Vector3_GetDirVector(rot.X, rot.Y);
	Vector3_Negate(&dir, &dir);

	Picking_ClipCameraPos(target, dir, dist, &cameraClipPos);
	return cameraClipPos.Intersect;
}

static bool ThirdPersonCamera_Zoom(float amount) {
	float* dist   = cam_isForwardThird ? &dist_forward : &dist_third;
	float newDist = *dist - amount;

	*dist = max(newDist, 2.0f); 
	return true;
}

static struct Camera cam_ThirdPerson = {
	true,
	PerspectiveCamera_GetProjection,  PerspectiveCamera_GetView,
	ThirdPersonCamera_GetOrientation, ThirdPersonCamera_GetPosition,
	PerspectiveCamera_UpdateMouse,    Camera_OnRawMouseMoved,
	Camera_AcquireFocus,              Camera_LoseFocus,
	PerspectiveCamera_GetPickedBlock, ThirdPersonCamera_Zoom,
};
static struct Camera cam_ForwardThird = {
	true,
	PerspectiveCamera_GetProjection,  PerspectiveCamera_GetView,
	ThirdPersonCamera_GetOrientation, ThirdPersonCamera_GetPosition,
	PerspectiveCamera_UpdateMouse,    Camera_OnRawMouseMoved,
	Camera_AcquireFocus,              Camera_LoseFocus,
	PerspectiveCamera_GetPickedBlock, ThirdPersonCamera_Zoom,
};


/*########################################################################################################################*
*-----------------------------------------------------General camera------------------------------------------------------*
*#########################################################################################################################*/
static void Camera_RawMouseMovedHandler(void* obj, int deltaX, int deltaY) {
	Camera.Active->OnRawMouseMoved(deltaX, deltaY);
}

static void Camera_CheckThirdPerson(void* obj) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->CanUseThirdPersonCamera || !h->Enabled) Camera_CycleActive();
}

void Camera_Init(void) {
	Camera_Register(&cam_FirstPerson);
	Camera_Register(&cam_ThirdPerson);
	Camera_Register(&cam_ForwardThird);

	Camera.Active = &cam_FirstPerson;
	Event_RegisterMouseMove(&MouseEvents.RawMoved,         NULL, Camera_RawMouseMovedHandler);
	Event_RegisterVoid(&UserEvents.HackPermissionsChanged, NULL, Camera_CheckThirdPerson);

#ifdef CC_BUILD_WIN
	Camera.Sensitivity = Options_GetInt(OPT_SENSITIVITY, 1, 100, 50);
#else
	Camera.Sensitivity = Options_GetInt(OPT_SENSITIVITY, 1, 100, 30);
#endif
	Camera.Clipping    = Options_GetBool(OPT_CAMERA_CLIPPING, true);
	Camera.Invert      = Options_GetBool(OPT_INVERT_MOUSE, false);
}

void Camera_CycleActive(void) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (Game_ClassicMode) return;
	Camera.Active = Camera.Active->Next;

	if (!p->Hacks.CanUseThirdPersonCamera || !p->Hacks.Enabled) {
		Camera.Active = &cam_FirstPerson;
	}
	cam_isForwardThird = Camera.Active == &cam_ForwardThird;

	/* reset rotation offset when changing cameras */
	cam_rotOffset.X = 0.0f; cam_rotOffset.Y = 0.0f;
	Game_UpdateProjection();
}

static struct Camera* cams_head;
static struct Camera* cams_tail;
void Camera_Register(struct Camera* cam) {
	LinkedList_Add(cam, cams_head, cams_tail);
	/* want a circular linked list */
	cam->Next = cams_head;
}

static bool cam_focussed;
void Camera_CheckFocus(void) {
	bool focus = !Gui_GetActiveScreen()->HandlesAllInput;
	if (focus == cam_focussed) return;
	cam_focussed = focus;

	if (focus) {
		Camera.Active->AcquireFocus();
	} else {
		Camera.Active->LoseFocus();
	}
}
