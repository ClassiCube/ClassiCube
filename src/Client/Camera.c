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
Vector3 cam_targetOffset;
Camera Camera_Cameras[3];
Int32 Camera_ActiveIndex;
#define Cam_IsForward_Third() (Camera_ActiveIndex == 2)

static Vector3 PerspectiveCamera_GetDirVector(void) {
	Entity* p = &LocalPlayer_Instance.Base;
	Vector2 rot = Camera_Active->GetOrientation();
	Vector3 dir = Vector3_GetDirVector(rot.X, rot.Y);

	/* Adjusts pitch of the player to avoid looking straight up or down, */
	/* as pitch parallel to camera up vector causes rendering issues */	
	if (dir.Y > +0.999998f) dir.Y = +0.999998f;
	if (dir.Y < -0.999998f) dir.Y = -0.999998f;

	return dir;
}

static void PerspectiveCamera_GetProjection(Matrix* proj) {
	Real32 fovy = Game_Fov * MATH_DEG2RAD;
	Real32 aspectRatio = (Real32)Game_Width / (Real32)Game_Height;
	Matrix_PerspectiveFieldOfView(proj, fovy, aspectRatio, Gfx_MinZNear, (Real32)Game_ViewDistance);
}

static void PerspectiveCamera_GetView(Matrix* mat) {
	Vector3 pos = Game_CurrentCameraPos, target, up = Vector3_UnitY;
	Vector3_Add(&target, &pos, &cam_targetOffset);

	Matrix_LookAt(mat, pos, target, up);
	Matrix_MulBy(mat, &Camera_TiltM);
}

static void PerspectiveCamera_GetPickedBlock(PickedPos* pos) {
	Entity* p = &LocalPlayer_Instance.Base;
	/* Vector3 dir = PerspectiveCamera_GetDirVector(); */
	Vector3 dir = Vector3_GetDirVector(p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD);
	Vector3 eyePos = Entity_GetEyePosition(p);
	Real32 reach = LocalPlayer_Instance.ReachDistance;
	Picking_CalculatePickedBlock(eyePos, dir, reach, pos);
}

Point2D cam_prev, cam_delta;
static void PerspectiveCamera_CentreMousePosition(void) {
	Point2D topLeft = Window_PointToScreen(Point2D_Empty);
	Int32 cenX = topLeft.X + Game_Width / 2;
	Int32 cenY = topLeft.Y + Game_Height / 2;

	Window_SetDesktopCursorPos(Point2D_Make(cenX, cenY));
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	cam_prev = Window_GetDesktopCursorPos();
}

static void PerspectiveCamera_RegrabMouse(void) {
	if (!Window_GetExists()) return;
	cam_delta = Point2D_Empty;
	PerspectiveCamera_CentreMousePosition();
}

#define CAMERA_SENSI_FACTOR (0.0002f / 3.0f * MATH_RAD2DEG)
#define CAMERA_SLIPPERY 0.97f
#define CAMERA_ADJUST 0.025f

Real32 speedX = 0.0f, speedY = 0.0f;
static Vector2 PerspectiveCamera_GetMouseDelta(void) {
	Real32 sensitivity = CAMERA_SENSI_FACTOR * Game_MouseSensitivity;

	if (Game_SmoothCamera) {
		speedX += cam_delta.X * CAMERA_ADJUST;
		speedX *= CAMERA_SLIPPERY;
		speedY += cam_delta.Y * CAMERA_ADJUST;
		speedY *= CAMERA_SLIPPERY;
	} else {
		speedX = (Real32)cam_delta.X;
		speedY = (Real32)cam_delta.Y;
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
	LocalPlayer* player = &LocalPlayer_Instance;

	Real32 headY = player->Interp.Next.HeadY + rot.X;
	Real32 headX = player->Interp.Next.HeadX + rot.Y;
	LocationUpdate update;
	LocationUpdate_MakeOri(&update, headY, headX);

	/* Need to make sure we don't cross the vertical axes, because that gets weird. */
	if (update.HeadX >= 90.0f && update.HeadX <= 270.0f) {
		update.HeadX = player->Interp.Next.HeadX < 180.0f ? 89.9f : 270.1f;
	}

	Entity* e = &player->Base;
	e->VTABLE->SetLocation(e, &update, false);
}

static void PerspectiveCamera_UpdateMouse(void) {
	Screen* screen = Gui_GetActiveScreen();
	if (screen->HandlesAllInput) {
		cam_delta = Point2D_Empty;
	} else if (Window_GetFocused()) {
		Point2D pos = Window_GetDesktopCursorPos();
		cam_delta = Point2D_Make(pos.X - cam_prev.X, pos.Y - cam_prev.Y);
		PerspectiveCamera_CentreMousePosition();
	}
	PerspectiveCamera_UpdateMouseRotation();
}

static void PerspectiveCamera_CalcViewBobbing(Real32 t, Real32 velTiltScale) {
	if (!Game_ViewBobbing) { Camera_TiltM = Matrix_Identity; return; }
	LocalPlayer* p = &LocalPlayer_Instance;
	Entity* e = &p->Base;
	Matrix Camera_tiltY, Camera_velX;

	Matrix_RotateZ(&Camera_TiltM, -p->Tilt.TiltX * e->Anim.BobStrength);
	Matrix_RotateX(&Camera_tiltY, Math_AbsF(p->Tilt.TiltY) * 3.0f * e->Anim.BobStrength);
	Matrix_MulBy(&Camera_TiltM, &Camera_tiltY);

	Camera_BobbingHor = (e->Anim.BobbingHor * 0.3f) * e->Anim.BobStrength;
	Camera_BobbingVer = (e->Anim.BobbingVer * 0.6f) * e->Anim.BobStrength;

	Real32 vel = Math_Lerp(e->OldVelocity.Y + 0.08f, e->Velocity.Y + 0.08f, t);
	Matrix_RotateX(&Camera_velX, -vel * 0.05f * p->Tilt.VelTiltStrength / velTiltScale);
	Matrix_MulBy(&Camera_TiltM, &Camera_velX);
}

static void PerspectiveCamera_Init(Camera* cam) {
	cam->GetProjection = PerspectiveCamera_GetProjection;
	cam->GetView = PerspectiveCamera_GetView;
	cam->UpdateMouse = PerspectiveCamera_UpdateMouse;
	cam->RegrabMouse = PerspectiveCamera_RegrabMouse;
	cam->GetPickedBlock = PerspectiveCamera_GetPickedBlock;
}


static Vector2 FirstPersonCamera_GetOrientation(void) {
	Entity* p = &LocalPlayer_Instance.Base;
	Vector2 ori = { p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD };
	return ori;
}

static Vector3 FirstPersonCamera_GetPosition(Real32 t) {
	PerspectiveCamera_CalcViewBobbing(t, 1);
	Entity* p = &LocalPlayer_Instance.Base;
	Vector3 camPos = Entity_GetEyePosition(p);
	camPos.Y += Camera_BobbingVer;

	Real32 headY = (p->HeadY * MATH_DEG2RAD);
	camPos.X += Camera_BobbingHor * Math_CosF(headY);
	camPos.Z += Camera_BobbingHor * Math_SinF(headY);

	cam_targetOffset = PerspectiveCamera_GetDirVector();
	return camPos;
}

static bool FirstPersonCamera_Zoom(Real32 amount) { return false; }

static void FirstPersonCamera_Init(Camera* cam) {
	PerspectiveCamera_Init(cam);
	cam->IsThirdPerson = false;
	cam->GetOrientation = FirstPersonCamera_GetOrientation;
	cam->GetPosition = FirstPersonCamera_GetPosition;
	cam->Zoom = FirstPersonCamera_Zoom;
}


Real32 dist_third = 3.0f, dist_forward = 3.0f;
static Vector2 ThirdPersonCamera_GetOrientation(void) {
	Entity* p = &LocalPlayer_Instance.Base;
	Vector2 v = { p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD };
	if (Cam_IsForward_Third()) { v.X += MATH_PI; v.Y = -v.Y; }

	v.X += cam_rotOffset.X * MATH_DEG2RAD; 
	v.Y += cam_rotOffset.Y * MATH_DEG2RAD;
	return v;
}

static Vector3 ThirdPersonCamera_GetPosition(Real32 t) {
	Real32 dist = Cam_IsForward_Third() ? dist_forward : dist_third;
	PerspectiveCamera_CalcViewBobbing(t, dist);

	Entity* p = &LocalPlayer_Instance.Base;
	Vector3 target = Entity_GetEyePosition(p);
	target.Y += Camera_BobbingVer;

	Vector3 dir = PerspectiveCamera_GetDirVector(); Vector3_Negate(&dir, &dir);
	Picking_ClipCameraPos(target, dir, dist, &Game_CameraClipPos);
	Vector3 camPos = Game_CameraClipPos.Intersect;

	Vector3_Sub(&cam_targetOffset, &target, &camPos);
	return camPos;
}

static Vector3 ThirdPersonCamera_GetTarget(void) {
	Entity* p = &LocalPlayer_Instance.Base;
	Vector3 eyePos = Entity_GetEyePosition(p);
	eyePos.Y += Camera_BobbingVer;
	return eyePos;
}

static bool ThirdPersonCamera_Zoom(Real32 amount) {
	Real32* dist = Cam_IsForward_Third() ? &dist_forward : &dist_third;
	Real32 newDist = *dist - amount;

	newDist = min(newDist, 2.0f); *dist = newDist;
	return true;
}

static void ThirdPersonCamera_Init(Camera* cam) {
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

	Int32 i = Camera_ActiveIndex;
	i = (i + 1) % Array_Elems(Camera_Cameras);

	LocalPlayer* player = &LocalPlayer_Instance;
	if (!player->Hacks.CanUseThirdPersonCamera || !player->Hacks.Enabled) { i = 0; }

	Camera_Active = &Camera_Cameras[i];
	Camera_ActiveIndex = i;
	Game_UpdateProjection();
}