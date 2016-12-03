// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Entities;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class Camera {
		protected Game game;
		protected internal Matrix4 tiltM;
		internal float bobbingVer, bobbingHor;
		
		public abstract Matrix4 GetProjection();
		
		public abstract Matrix4 GetView();
		
		/// <summary> Calculates the location of the camera's position in the world. </summary>
		public abstract Vector3 GetCameraPos(float t);
		
		/// <summary> Calculates the yaw and pitch of the camera in radians. </summary>
		public abstract Vector2 GetCameraOrientation();
		
		/// <summary> Whether this camera is using a third person perspective. </summary>
		/// <remarks> This causes the local player to be renderered if true. </remarks>
		public abstract bool IsThirdPerson { get; }
		
		public virtual void Tick(double elapsed) { }
		
		public virtual bool DoZoom(float deltaPrecise) { return false; }
		
		public abstract void RegrabMouse();
		
		/// <summary> Calculates the picked block based on the camera's current position. </summary>
		public virtual void GetPickedBlock(PickedPos pos) { }
		
		protected float AdjustPitch(float value) {
			if (value >= 90.0f && value <= 90.1f) return 90.1f * Utils.Deg2Rad;
			if (value >= 89.9f && value <= 90.0f) return 89.9f * Utils.Deg2Rad;
			if (value >= 270.0f && value <= 270.1f) return 270.1f * Utils.Deg2Rad;
			if (value >= 269.9f && value <= 270.0f) return 269.9f * Utils.Deg2Rad;
			return value * Utils.Deg2Rad;
		}
	}
	
	public abstract class PerspectiveCamera : Camera {
		
		protected LocalPlayer player;
		public PerspectiveCamera(Game game) {
			this.game = game;
			player = game.LocalPlayer;
			tiltM = Matrix4.Identity;
		}
		
		public override Matrix4 GetProjection() {
			float fovy = game.Fov * Utils.Deg2Rad;
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			return Matrix4.CreatePerspectiveFieldOfView(fovy, aspectRatio, zNear, game.ViewDistance);
		}
		
		public override void GetPickedBlock(PickedPos pos) {
			Vector3 dir = Utils.GetDirVector(player.HeadYawRadians,
			                                 AdjustPitch(player.PitchDegrees));
			Vector3 eyePos = player.EyePosition;
			float reach = game.LocalPlayer.ReachDistance;
			Picking.CalculatePickedBlock(game, eyePos, dir, reach, pos);
		}
		
		protected static Point previous, delta;
		void CentreMousePosition() {
			if (!game.Focused) return;
			Point current = game.DesktopCursorPos;
			delta = new Point(current.X - previous.X, current.Y - previous.Y);
			Point topLeft = game.PointToScreen(Point.Empty);
			int cenX = topLeft.X + game.Width / 2;
			int cenY = topLeft.Y + game.Height / 2;
			game.DesktopCursorPos = new Point(cenX, cenY);
			// Fixes issues with large DPI displays on Windows >= 8.0.
			previous = game.DesktopCursorPos;
		}
		
		public override void RegrabMouse() {
			if (!game.Exists) return;
			Point topLeft = game.PointToScreen(Point.Empty);
			int cenX = topLeft.X + game.Width / 2;
			int cenY = topLeft.Y + game.Height / 2;
			game.DesktopCursorPos = new Point(cenX, cenY);
			previous = new Point(cenX, cenY);
			delta = Point.Empty;
		}
		
		static readonly float sensiFactor = 0.0002f / 3 * Utils.Rad2Deg;
		private void UpdateMouseRotation() {
			float sensitivity = sensiFactor * game.MouseSensitivity;
			float yaw = player.nextYaw + delta.X * sensitivity;
			float yAdj = game.InvertMouse ? -delta.Y * sensitivity : delta.Y * sensitivity;
			float pitch = player.nextPitch + yAdj;
			LocationUpdate update = LocationUpdate.MakeOri(yaw, pitch);
			
			// Need to make sure we don't cross the vertical axes, because that gets weird.
			if (update.Pitch >= 90 && update.Pitch <= 270)
				update.Pitch = player.nextPitch < 180 ? 89.9f : 270.1f;
			game.LocalPlayer.SetLocation(update, true);
		}
		
		public override void Tick(double elapsed) {
			if (game.Gui.ActiveScreen.HandlesAllInput) return;
			CentreMousePosition();
			UpdateMouseRotation();
		}
		
		protected void CalcViewBobbing(float t, float velTiltScale) {
			if (!game.ViewBobbing) { tiltM = Matrix4.Identity; return; }
			
			LocalPlayer p = game.LocalPlayer;
			tiltM = Matrix4.RotateZ(-p.anim.tiltX * p.anim.bobStrength);
			tiltM = tiltM * Matrix4.RotateX(Math.Abs(p.anim.tiltY) * 3 * p.anim.bobStrength);
			
			bobbingHor = (p.anim.bobbingHor * 0.3f) * p.anim.bobStrength;
			bobbingVer = (p.anim.bobbingVer * 0.6f) * p.anim.bobStrength;
			
			float vel = Utils.Lerp(p.OldVelocity.Y + 0.08f, p.Velocity.Y + 0.08f, t);
			tiltM = tiltM * Matrix4.RotateX(-vel * 0.05f * p.anim.velTiltStrength / velTiltScale);
		}
		
		protected Vector3 GetDirVector() {
			return Utils.GetDirVector(player.HeadYawRadians,
			                          AdjustPitch(player.PitchDegrees));
		}
	}
	
	public class ThirdPersonCamera : PerspectiveCamera {
		public ThirdPersonCamera(Game window, bool forward) : base(window) { this.forward = forward; }
		public override bool IsThirdPerson { get { return true; } }
		
		bool forward;
		float dist = 3;
		public override bool DoZoom(float deltaPrecise) {
			dist = Math.Max(dist - deltaPrecise, 2);
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
				return new Vector2(player.HeadYawRadians, player.PitchRadians);
			return new Vector2(player.HeadYawRadians + (float)Math.PI, -player.PitchRadians);			
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
	
	public class FirstPersonCamera : PerspectiveCamera {
		public FirstPersonCamera(Game window) : base(window) { }
		public override bool IsThirdPerson { get { return false; } }
		
		public override Matrix4 GetView() {
			Vector3 camPos = game.CurrentCameraPos;
			Vector3 dir = GetDirVector();
			return Matrix4.LookAt(camPos, camPos + dir, Vector3.UnitY) * tiltM;
		}
		
		public override Vector2 GetCameraOrientation() {
			return new Vector2(player.HeadYawRadians, player.PitchRadians);
		}
		
		public override Vector3 GetCameraPos(float t) {
			CalcViewBobbing(t, 1);
			Vector3 camPos = player.EyePosition;
			camPos.Y += bobbingVer;
			
			double adjYaw = player.HeadYawRadians + Math.PI / 2;
			camPos.X += bobbingHor * (float)Math.Sin(adjYaw);
			camPos.Z -= bobbingHor * (float)Math.Cos(adjYaw);
			return camPos;
		}
	}
}