#include "Camera.h"
#include "ExtMath.h"
#include "Game.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Funcs.h"
#include "Gui.h"
#include "Entity.h"
#include "Input.h"

Vector2 cam_rotOffset;
struct Camera Camera_Cameras[3];
int Camera_ActiveIndex;
#define Cam_IsForward_Third() (Camera_ActiveIndex == 2)

static void PerspectiveCamera_GetProjection(struct Matrix* proj) {
	float fovy = Game_Fov * MATH_DEG2RAD;
	float aspectRatio = (float)Game_Width / (float)Game_Height;
	Matrix_PerspectiveFieldOfView(proj, fovy, aspectRatio, Gfx_MinZNear, (float)Game_ViewDistance);
}

static void PerspectiveCamera_GetView(struct Matrix* mat) {
	Vector3 pos = Game_CurrentCameraPos;
	Vector2 rot = Camera_Active->GetOrientation();
	Matrix_LookRot(mat, pos, rot);
	Matrix_MulBy(mat, &Camera_TiltM);
}

static void PerspectiveCamera_GetPickedBlock(struct PickedPos* pos) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector3 dir = Vector3_GetDirVector(p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD);
	Vector3 eyePos = Entity_GetEyePosition(p);
	float reach = LocalPlayer_Instance.ReachDistance;
	Picking_CalculatePickedBlock(eyePos, dir, reach, pos);
}

Point2D cam_prev, cam_delta;
static void PerspectiveCamera_CentreMousePosition(void) {
	Point2D topLeft = Window_PointToScreen(0, 0);
	int cenX = topLeft.X + Game_Width  / 2;
	int cenY = topLeft.Y + Game_Height / 2;

	Window_SetDesktopCursorPos(cenX, cenY);
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	cam_prev = Window_GetDesktopCursorPos();
}

static void PerspectiveCamera_RegrabMouse(void) {
	if (!Window_Exists) return;
	cam_delta.X = 0; cam_delta.Y = 0;
	PerspectiveCamera_CentreMousePosition();
}

#define CAMERA_SENSI_FACTOR (0.0002f / 3.0f * MATH_RAD2DEG)
#define CAMERA_SLIPPERY 0.97f
#define CAMERA_ADJUST 0.025f

float speedX = 0.0f, speedY = 0.0f;
static Vector2 PerspectiveCamera_GetMouseDelta(void) {
	float sensitivity = CAMERA_SENSI_FACTOR * Game_MouseSensitivity;

	if (Game_SmoothCamera) {
		speedX += cam_delta.X * CAMERA_ADJUST;
		speedX *= CAMERA_SLIPPERY;
		speedY += cam_delta.Y * CAMERA_ADJUST;
		speedY *= CAMERA_SLIPPERY;
	} else {
		speedX = (float)cam_delta.X;
		speedY = (float)cam_delta.Y;
	}

	Vector2 v = { speedX * sensitivity, speedY * sensitivity };
	if (Game_InvertMouse) v.Y = -v.Y;
	return v;
}

static void PerspectiveCamera_UpdateMouseRotation(void) {
	Vector2 rot = PerspectiveCamera_GetMouseDelta();
	if (Key_IsAltPressed() && Camera_Active->IsThirdPerson) {
		cam_rotOffset.X += rot.X; cam_rotOffset.Y += rot.Y;
		return;
	}
	struct LocalPlayer* player = &LocalPlayer_Instance;

	float headY = player->Interp.Next.HeadY + rot.X;
	float headX = player->Interp.Next.HeadX + rot.Y;
	struct LocationUpdate update;
	LocationUpdate_MakeOri(&update, headY, headX);

	/* Need to make sure we don't cross the vertical axes, because that gets weird. */
	if (update.HeadX >= 90.0f && update.HeadX <= 270.0f) {
		update.HeadX = player->Interp.Next.HeadX < 180.0f ? 90.0f : 270.0f;
	}

	struct Entity* e = &player->Base;
	e->VTABLE->SetLocation(e, &update, false);
}

static void PerspectiveCamera_UpdateMouse(void) {
	struct Screen* screen = Gui_GetActiveScreen();
	if (screen->HandlesAllInput) {
		cam_delta.X = 0; cam_delta.Y = 0;
	} else if (Window_Focused) {
		Point2D pos = Window_GetDesktopCursorPos();
		cam_delta.X = pos.X - cam_prev.X; cam_delta.Y = pos.Y - cam_prev.Y;
		PerspectiveCamera_CentreMousePosition();
	}
	PerspectiveCamera_UpdateMouseRotation();
}

static void PerspectiveCamera_CalcViewBobbing(float t, float velTiltScale) {
	if (!Game_ViewBobbing) { Camera_TiltM = Matrix_Identity; return; }
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Entity* e = &p->Base;
	struct Matrix Camera_tiltY, Camera_velX;

	Matrix_RotateZ(&Camera_TiltM, -p->Tilt.TiltX * e->Anim.BobStrength);
	Matrix_RotateX(&Camera_tiltY, Math_AbsF(p->Tilt.TiltY) * 3.0f * e->Anim.BobStrength);
	Matrix_MulBy(&Camera_TiltM, &Camera_tiltY);

	Camera_BobbingHor = (e->Anim.BobbingHor * 0.3f) * e->Anim.BobStrength;
	Camera_BobbingVer = (e->Anim.BobbingVer * 0.6f) * e->Anim.BobStrength;

	float vel = Math_Lerp(p->OldVelocity.Y + 0.08f, e->Velocity.Y + 0.08f, t);
	Matrix_RotateX(&Camera_velX, -vel * 0.05f * p->Tilt.VelTiltStrength / velTiltScale);
	Matrix_MulBy(&Camera_TiltM, &Camera_velX);
}

static void PerspectiveCamera_Init(struct Camera* cam) {
	cam->GetProjection = PerspectiveCamera_GetProjection;
	cam->GetView = PerspectiveCamera_GetView;
	cam->UpdateMouse = PerspectiveCamera_UpdateMouse;
	cam->RegrabMouse = PerspectiveCamera_RegrabMouse;
	cam->GetPickedBlock = PerspectiveCamera_GetPickedBlock;
}


static Vector2 FirstPersonCamera_GetOrientation(void) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector2 ori = { p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD };
	return ori;
}

static Vector3 FirstPersonCamera_GetPosition(float t) {
	PerspectiveCamera_CalcViewBobbing(t, 1);
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector3 camPos = Entity_GetEyePosition(p);
	camPos.Y += Camera_BobbingVer;

	float headY = (p->HeadY * MATH_DEG2RAD);
	camPos.X += Camera_BobbingHor * (float)Math_Cos(headY);
	camPos.Z += Camera_BobbingHor * (float)Math_Sin(headY);
	return camPos;
}

static bool FirstPersonCamera_Zoom(float amount) { return false; }

static void FirstPersonCamera_Init(struct Camera* cam) {
	PerspectiveCamera_Init(cam);
	cam->IsThirdPerson = false;
	cam->GetOrientation = FirstPersonCamera_GetOrientation;
	cam->GetPosition = FirstPersonCamera_GetPosition;
	cam->Zoom = FirstPersonCamera_Zoom;
}


float dist_third = 3.0f, dist_forward = 3.0f;
static Vector2 ThirdPersonCamera_GetOrientation(void) {
	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector2 v = { p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD };
	if (Cam_IsForward_Third()) { v.X += MATH_PI; v.Y = -v.Y; }

	v.X += cam_rotOffset.X * MATH_DEG2RAD; 
	v.Y += cam_rotOffset.Y * MATH_DEG2RAD;
	return v;
}

static Vector3 ThirdPersonCamera_GetPosition(float t) {
	float dist = Cam_IsForward_Third() ? dist_forward : dist_third;
	PerspectiveCamera_CalcViewBobbing(t, dist);

	struct Entity* p = &LocalPlayer_Instance.Base;
	Vector3 target = Entity_GetEyePosition(p);
	target.Y += Camera_BobbingVer;

	Vector2 rot = Camera_Active->GetOrientation();
	Vector3 dir = Vector3_GetDirVector(rot.X, rot.Y);
	Vector3_Negate(&dir, &dir);

	Picking_ClipCameraPos(target, dir, dist, &Game_CameraClipPos);
	return Game_CameraClipPos.Intersect;
}

static bool ThirdPersonCamera_Zoom(float amount) {
	float* dist = Cam_IsForward_Third() ? &dist_forward : &dist_third;
	float newDist = *dist - amount;

	*dist = max(newDist, 2.0f); 
	return true;
}

static void ThirdPersonCamera_Init(struct Camera* cam) {
	PerspectiveCamera_Init(cam);
	cam->IsThirdPerson = true;
	cam->GetOrientation = ThirdPersonCamera_GetOrientation;
	cam->GetPosition = ThirdPersonCamera_GetPosition;
	cam->Zoom = ThirdPersonCamera_Zoom;
}


void Camera_Init(void) {
	FirstPersonCamera_Init(&Camera_Cameras[0]);
	ThirdPersonCamera_Init(&Camera_Cameras[1]);
	ThirdPersonCamera_Init(&Camera_Cameras[2]);

	Camera_Active = &Camera_Cameras[0];
	Camera_ActiveIndex = 0;
}

void Camera_CycleActive(void) {
	if (Game_ClassicMode) return;

	int i = Camera_ActiveIndex;
	i = (i + 1) % Array_Elems(Camera_Cameras);

	struct LocalPlayer* player = &LocalPlayer_Instance;
	if (!player->Hacks.CanUseThirdPersonCamera || !player->Hacks.Enabled) { i = 0; }

	Camera_Active = &Camera_Cameras[i];
	Camera_ActiveIndex = i;
	/* reset rotation offset when changing cameras */
	cam_rotOffset.X = 0.0f; cam_rotOffset.Y = 0.0f;

	Game_UpdateProjection();
}
