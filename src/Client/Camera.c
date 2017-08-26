#include "Camera.h"
#include "ExtMath.h"
#include "Game.h"
#include "Window.h"
#include "GraphicsAPI.h"
#include "Player.h"

Real32 Camera_AdjustHeadX(Real32 value) {
	if (value >= 90.00f && value <= 90.10f) return 90.10f * MATH_DEG2RAD;
	if (value >= 89.90f && value <= 90.00f) return 89.90f * MATH_DEG2RAD;
	if (value >= 270.0f && value <= 270.1f) return 270.1f * MATH_DEG2RAD;
	if (value >= 269.9f && value <= 270.0f) return 269.9f * MATH_DEG2RAD;
	return value * MATH_DEG2RAD;
}

Vector3 PerspectiveCamera_GetDirVector() {
	Entity* p = &LocalPlayer_Instance.Base.Base;
	Real32 yaw = p->HeadY * MATH_DEG2RAD;
	Real32 pitch = Camera_AdjustHeadX(p->HeadX);
	return Vector3_GetDirVector(yaw, pitch);
}

void PerspectiveCamera_GetProjection(Matrix* proj) {
	Real32 fovy = Game_Fov * MATH_DEG2RAD;
	Size2D size = Window_GetClientSize();
	Real32 aspectRatio = (Real32)size.Width / (Real32)size.Height;
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
	Size2D size = Window_GetClientSize();
	Int32 cenX = topLeft.X + size.Width  * 0.5f;
	Int32 cenY = topLeft.Y + size.Height * 0.5f;

	Window_SetDesktopCursorPos(Point2D_Make(cenX, cenY));
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	previous = Window_GetDesktopCursorPos();
}

void PerspectiveCamera_RegrabMouse(void) {
	if (!Window_GetExists()) return;

	Point2D topLeft = Window_PointToScreen(Point2D_Empty);
	Size2D size = Window_GetClientSize();
	Int32 cenX = topLeft.X + size.Width  * 0.5f;
	Int32 cenY = topLeft.Y + size.Height * 0.5f;

	Point2D point = Point2D_Make(cenX, cenY);
	Window_SetDesktopCursorPos(point);
	previous = point;
	delta = Point2D_Empty;
}

#define sensiFactor (0.0002f / 3.0f * MATH_RAD2DEG)
#define slippery 0.97f
#define adjust 0.025f

Real32 speedX = 0.0f, speedY = 0.0f;
void PersepctiveCamera_UpdateMouseRotation(void) {
	Real32 sensitivity = sensiFactor * Game_MouseSensitivity;

	if (Game_SmoothCamera) {
		speedX += delta.X * adjust;
		speedX *= slippery;
		speedY += delta.Y * adjust;
		speedY *= slippery;
	} else {
		speedX = delta.X;
		speedY = delta.Y;
	}

	Real32 rotY = player.interp.next.HeadY + speedX * sensitivity;
	Real32 yAdj = Game_InvertMouse ? -speedY * sensitivity : speedY * sensitivity;
	Real32 headX = player.interp.next.HeadX + yAdj;
	LocationUpdate* update;
	LocationUpdate_MakeOri(update, rotY, headX);

	/* Need to make sure we don't cross the vertical axes, because that gets weird. */
	if (update->HeadX >= 90.0f && update->HeadX <= 270.0f) {
		update->HeadX = player.interp.next.HeadX < 180.0f ? 89.9f : 270.1f;
	}
	game.LocalPlayer.SetLocation(update, false);
}

void PerspectiveCamera_UpdateMouse(void) {
	if (game.Gui.ActiveScreen.HandlesAllInput) return;
	CentreMousePosition();
	UpdateMouseRotation();
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


	public class ThirdPersonCamera : PerspectiveCamera {
		public ThirdPersonCamera(Game window, bool forward) : base(window) { this.forward = forward; }
		public override bool IsThirdPerson{ get{ return true; } }

		bool forward;
		float dist = 3;
		public override bool Zoom(float amount) {
			dist = Math.Max(dist - amount, 2);
			return true;
		}

		public override Matrix4 GetView() {
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobbingVer;

			Vector3 cameraPos = game.CurrentCameraPos;
			return Matrix4.LookAt(cameraPos, eyePos, Vector3.UnitY) * tiltM;
		}

		public override Vector2 GetCameraOrientation() {
			if (!forward)
				return new Vector2(player.HeadYRadians, player.HeadXRadians);
			return new Vector2(player.HeadYRadians + (float)Math.PI, -player.HeadXRadians);
		}

		public override Vector3 GetCameraPos(float t) {
			CalcViewBobbing(t, dist);
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobbingVer;

			Vector3 dir = GetDirVector();
			if (!forward) dir = -dir;
			Picking.ClipCameraPos(game, eyePos, dir, dist, game.CameraClipPos);
			return game.CameraClipPos.Intersect;
		}
	}


void FirstPersonCamera_GetView(Matrix* mat) {
	Vector3 camPos = Game_CurrentCameraPos;
	Vector3 dir = GetDirVector();
	return Matrix4.LookAt(camPos, camPos + dir, Vector3.UnitY) * tiltM;
}

Vector2 FirstPersonCamera_GetCameraOrientation(void) {
	Entity* p = &LocalPlayer_Instance->Base->Base;
	return Vector2_Create2(p->HeadY * MATH_DEG2RAD, p->HeadX * MATH_DEG2RAD);
}

Vector3 FirstPersonCamera_GetCameraPos(void) {
	CalcViewBobbing(t, 1);
	Vector3 camPos = player.EyePosition;
	camPos.Y += bobbingVer;

	double adjHeadY = player.HeadYRadians + Math.PI / 2;
	camPos.X += bobbingHor * (float)Math.Sin(adjHeadY);
	camPos.Z -= bobbingHor * (float)Math.Cos(adjHeadY);
	return camPos;
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