#include "Camera.h"
#include "ExtMath.h"
#include "Game.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Player.h"
#include "Funcs.h"
#include "Gui.h"

Real32 Camera_AdjustHeadX(Real32 value) {
	if (value >= 90.00f && value <= 90.10f) return 90.10f * MATH_DEG2RAD;
	if (value >= 89.90f && value <= 90.00f) return 89.90f * MATH_DEG2RAD;
	if (value >= 270.0f && value <= 270.1f) return 270.1f * MATH_DEG2RAD;
	if (value >= 269.9f && value <= 270.0f) return 269.9f * MATH_DEG2RAD;
	return value * MATH_DEG2RAD;
}

Vector3 PerspectiveCamera_GetDirVector(void) {
	Entity* p = &LocalPlayer_Instance.Base.Base;
	Real32 yaw = p->HeadY * MATH_DEG2RAD;
	Real32 pitch = Camera_AdjustHeadX(p->HeadX);
	return Vector3_GetDirVector(yaw, pitch);
}

void PerspectiveCamera_GetProjection(Matrix* proj) {
	Real32 fovy = Game_Fov * MATH_DEG2RAD;
	Real32 aspectRatio = (Real32)Game_Width / (Real32)Game_Height;
	Matrix_PerspectiveFieldOfView(proj, fovy, aspectRatio, Gfx_MinZNear, Game_ViewDistance);
}

void PerspectiveCamera_GetPickedBlock(PickedPos* pos) {
	Entity* p = &LocalPlayer_Instance.Base.Base;
	Vector3 dir = PerspectiveCamera_GetDirVector();
	Vector3 eyePos = Entity_GetEyePosition(p);
	Real32 reach = LocalPlayer_Instance.ReachDistance;
	Picking_CalculatePickedBlock(eyePos, dir, reach, pos);
}

Point2D previous, delta;
void PerspectiveCamera_CentreMousePosition(void) {
	if (!Window_GetFocused()) return;
	Point2D current = Window_GetDesktopCursorPos();
	delta = Point2D_Make(current.X - previous.X, current.Y - previous.Y);

	Point2D topLeft = Window_PointToScreen(Point2D_Empty);
	Int32 cenX = topLeft.X + Game_Width  / 2;
	Int32 cenY = topLeft.Y + Game_Height / 2;

	Window_SetDesktopCursorPos(Point2D_Make(cenX, cenY));
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	previous = Window_GetDesktopCursorPos();
}

void PerspectiveCamera_RegrabMouse(void) {
	if (!Window_GetExists()) return;

	Point2D topLeft = Window_PointToScreen(Point2D_Empty);
	Int32 cenX = topLeft.X + Game_Width  / 2;
	Int32 cenY = topLeft.Y + Game_Height / 2;

	Point2D point = Point2D_Make(cenX, cenY);
	Window_SetDesktopCursorPos(point);
	previous = point;
	delta = Point2D_Empty;
}

#define CAMERA_SENSI_FACTOR (0.0002f / 3.0f * MATH_RAD2DEG)
#define CAMERA_SLIPPERY 0.97f
#define CAMERA_ADJUST 0.025f

Real32 speedX = 0.0f, speedY = 0.0f;
void PerspectiveCamera_UpdateMouseRotation(void) {
	Real32 sensitivity = CAMERA_SENSI_FACTOR * Game_MouseSensitivity;

	if (Game_SmoothCamera) {
		speedX += delta.X * CAMERA_ADJUST;
		speedX *= CAMERA_SLIPPERY;
		speedY += delta.Y * CAMERA_ADJUST;
		speedY *= CAMERA_SLIPPERY;
	} else {
		speedX = (Real32)delta.X;
		speedY = (Real32)delta.Y;
	}

	LocalPlayer* player = &LocalPlayer_Instance;
	Real32 rotY = player->Interp.Next.HeadY + speedX * sensitivity;
	Real32 yAdj = Game_InvertMouse ? -speedY * sensitivity : speedY * sensitivity;
	Real32 headX = player->Interp.Next.HeadX + yAdj;
	LocationUpdate update;
	LocationUpdate_MakeOri(&update, rotY, headX);

	/* Need to make sure we don't cross the vertical axes, because that gets weird. */
	if (update.HeadX >= 90.0f && update.HeadX <= 270.0f) {
		update.HeadX = player->Interp.Next.HeadX < 180.0f ? 89.9f : 270.1f;
	}

	Entity* e = &player->Base.Base;
	e->SetLocation(e, &update, false);
}

void PerspectiveCamera_UpdateMouse(void) {
	Screen* screen = Gui_GetActiveScreen();
	if (screen->HandlesAllInput) return;
	PerspectiveCamera_CentreMousePosition();
	PerspectiveCamera_UpdateMouseRotation();
}

void PerspectiveCamera_CalcViewBobbing(Real32 t, Real32 velTiltScale) {
	if (!Game_ViewBobbing) { Camera_TiltM = Matrix_Identity; return; }
	LocalPlayer* p = &LocalPlayer_Instance;
	Entity* e = &p->Base.Base;
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

void PerspectiveCamera_Init(Camera* cam) {
	cam->GetProjection = PerspectiveCamera_GetProjection;
	cam->UpdateMouse = PerspectiveCamera_UpdateMouse;
	cam->RegrabMouse = PerspectiveCamera_RegrabMouse;
	cam->GetPickedBlock = PerspectiveCamera_GetPickedBlock;
}


void FirstPersonCamera_GetView(Matrix* mat) {
	Vector3 camPos = Game_CurrentCameraPos;
	Vector3 dir = PerspectiveCamera_GetDirVector();
	Vector3 targetPos;
	Vector3_Add(&targetPos, &camPos, &dir);

	Matrix_LookAt(mat, camPos, targetPos, Vector3_UnitY);
	Matrix_MulBy(mat, &Camera_TiltM);
}

Vector3 FirstPersonCamera_GetCameraPos(Real32 t) {
	PerspectiveCamera_CalcViewBobbing(t, 1);
	Entity* p = &LocalPlayer_Instance.Base.Base;
	Vector3 camPos = Entity_GetEyePosition(p);
	camPos.Y += Camera_BobbingVer;

	Real32 adjHeadY = (p->HeadY * MATH_DEG2RAD) + MATH_PI / 2.0f;
	camPos.X += Camera_BobbingHor * Math_Sin(adjHeadY);
	camPos.Z -= Camera_BobbingHor * Math_Cos(adjHeadY);
	return camPos;
}

Vector2 FirstPersonCamera_GetCameraOrientation(void) {
	Entity* p = &LocalPlayer_Instance.Base.Base;
	return Vector2_Create2(p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD);
}

bool FirstPersonCamera_Zoom(Real32 amount) { return false; }

void FirstPersonCamera_Init(Camera* cam) {
	PerspectiveCamera_Init(cam);
	cam->IsThirdPerson = false;
	cam->GetView = FirstPersonCamera_GetView;
	cam->GetCameraPos = FirstPersonCamera_GetCameraPos;
	cam->GetCameraOrientation = FirstPersonCamera_GetCameraOrientation;
	cam->Zoom = FirstPersonCamera_Zoom;
}


Real32 dist_third = 3.0f, dist_forward = 3.0f;
void ThirdPersonCamera_GetView(Matrix* mat) {
	Vector3 cameraPos = Game_CurrentCameraPos;
	Entity* p = &LocalPlayer_Instance.Base.Base;
	Vector3 targetPos = Entity_GetEyePosition(p);
	targetPos.Y += Camera_BobbingVer;

	Matrix_LookAt(mat, cameraPos, targetPos, Vector3_UnitY);
	Matrix_MulBy(mat, &Camera_TiltM);
}

Vector3 ThirdPersonCamera_GetCameraPosShared(Real32 t, Real32 dist, bool forward) {
	PerspectiveCamera_CalcViewBobbing(t, dist);
	Entity* p = &LocalPlayer_Instance.Base.Base;
	Vector3 eyePos = Entity_GetEyePosition(p);
	eyePos.Y += Camera_BobbingVer;

	Vector3 dir = PerspectiveCamera_GetDirVector();
	if (!forward) Vector3_Negate(&dir, &dir);

	Picking_ClipCameraPos(eyePos, dir, dist, &Game_CameraClipPos);
	return Game_CameraClipPos.Intersect;
}

Vector3 ThirdPersonCamera_GetCameraPos(Real32 t) {
	return ThirdPersonCamera_GetCameraPosShared(t, dist_third, false);
}

Vector3 ForwardThirdPersonCamera_GetCameraPos(Real32 t) {
	return ThirdPersonCamera_GetCameraPosShared(t, dist_forward, true);
}


Vector2 ThirdPersonCamera_GetCameraOrientation(void) {
	Entity* p = &LocalPlayer_Instance.Base.Base;
	return Vector2_Create2(p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD);
}

Vector2 ForwardThirdPersonCamera_GetCameraOrientation(void) {
	Entity* p = &LocalPlayer_Instance.Base.Base;
	return Vector2_Create2(p->HeadY * MATH_DEG2RAD + MATH_PI, -p->HeadX * MATH_DEG2RAD);
}

bool ThirdPersonCamera_Zoom(Real32 amount) {
	dist_third -= amount;
	if (dist_third < 2.0f) dist_third = 2.0f;
	return true;
}


bool ForwardThirdPersonCamera_Zoom(Real32 amount) {
	dist_forward -= amount;
	if (dist_forward < 2.0f) dist_forward = 2.0f;
	return true;
}

void ThirdPersonCamera_Init(Camera* cam) {
	PerspectiveCamera_Init(cam);
	cam->IsThirdPerson = true;
	cam->GetView = ThirdPersonCamera_GetView;
	cam->GetCameraPos = ThirdPersonCamera_GetCameraPos;
	cam->GetCameraOrientation = ThirdPersonCamera_GetCameraOrientation;
	cam->Zoom = ThirdPersonCamera_Zoom;
}

void ForwardThirdPersonCamera_Init(Camera* cam) {
	ThirdPersonCamera_Init(cam);
	cam->GetCameraPos = ForwardThirdPersonCamera_GetCameraPos;
	cam->GetCameraOrientation = ForwardThirdPersonCamera_GetCameraOrientation;
	cam->Zoom = ForwardThirdPersonCamera_Zoom;
}

Camera Camera_Cameras[3];
Int32 Camera_ActiveIndex;
void Camera_Init(void) {
	FirstPersonCamera_Init(&Camera_Cameras[0]);
	ThirdPersonCamera_Init(&Camera_Cameras[1]);
	ForwardThirdPersonCamera_Init(&Camera_Cameras[2]);

	Camera_ActiveCamera = &Camera_Cameras[0];
	Camera_ActiveIndex = 0;
}

void Camera_CycleActive(void) {
	if (Game_ClassicMode) return;

	Int32 i = Camera_ActiveIndex;
	i = (i + 1) % Array_NumElements(Camera_Cameras);

	LocalPlayer* player = &LocalPlayer_Instance;
	if (!player->Hacks.CanUseThirdPersonCamera || !player->Hacks.Enabled) { i = 0; }

	Camera_ActiveCamera = &Camera_Cameras[i];
	Game_UpdateProjection();
}