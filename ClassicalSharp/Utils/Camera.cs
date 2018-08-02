// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Entities;
using OpenTK;

namespace ClassicalSharp {
	
	public abstract class Camera {
		protected Game game;
		internal static Matrix4 tiltM;
		internal static float bobbingVer, bobbingHor;
		
		public abstract void GetProjection(out Matrix4 m);
		public abstract void GetView(out Matrix4 m);
		
		public abstract Vector2 GetOrientation();
		public abstract Vector3 GetPosition(float t);
		
		public abstract bool IsThirdPerson { get; }
		public virtual bool Zoom(float amount) { return false; }
		
		public abstract void UpdateMouse();
		public abstract void RegrabMouse();
		
		public abstract void ResetRotOffset();
		
		public abstract void GetPickedBlock(PickedPos pos);
	}
	
	public abstract class PerspectiveCamera : Camera {
		
		protected static Vector2 rotOffset;
		protected LocalPlayer player;
		
		public PerspectiveCamera(Game game) {
			this.game = game;
			player = game.LocalPlayer;
			tiltM = Matrix4.Identity;
		}
		
		public override void GetProjection(out Matrix4 m) {
			float fov = game.Fov * Utils.Deg2Rad;
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			game.Graphics.CalcPerspectiveMatrix(fov, aspectRatio, zNear, game.ViewDistance, out m);
		}
		
		public override void GetView(out Matrix4 m) {
			Vector3 pos = game.CurrentCameraPos;
			Vector2 rot = GetOrientation();			
			Matrix4.LookRot(pos, rot, out m);
			Matrix4.Mult(out m, ref m, ref tiltM);
		}
		
		public override void GetPickedBlock(PickedPos pos) {
			Vector3 eyePos = player.EyePosition;
			Vector3 dir = Utils.GetDirVector(player.HeadYRadians, player.HeadXRadians);
			float reach = game.LocalPlayer.ReachDistance;
			
			Picking.CalculatePickedBlock(game, eyePos, dir, reach, pos);
		}
		
		protected static Point previous, delta;
		void CentreMousePosition() {
			Point topLeft = game.window.PointToScreen(Point.Empty);
			int cenX = topLeft.X + game.Width / 2;
			int cenY = topLeft.Y + game.Height / 2;
			
			game.DesktopCursorPos = new Point(cenX, cenY);
			// Fixes issues with large DPI displays on Windows >= 8.0.
			previous = game.DesktopCursorPos;
		}
		
		public override void RegrabMouse() {
			if (!game.window.Exists) return;
			delta = Point.Empty;
			CentreMousePosition();
		}
		
		public override void ResetRotOffset() {
            rotOffset.X = 0;
            rotOffset.Y = 0;
		}
		
		static readonly float sensiFactor = 0.0002f / 3 * Utils.Rad2Deg;
		const float slippery = 0.97f;
		const float adjust = 0.025f;
		
		static float speedX = 0, speedY = 0;
		protected Vector2 CalcMouseDelta() {
			float sensitivity = sensiFactor * game.MouseSensitivity;

			if (game.SmoothCamera) {
				speedX += delta.X * adjust;
				speedX *= slippery;
				speedY += delta.Y * adjust;
				speedY *= slippery;
			} else {
				speedX = delta.X;
				speedY = delta.Y;
			}
			
			float dx = speedX * sensitivity, dy = speedY * sensitivity;
			if (game.InvertMouse) dy = -dy;
			return new Vector2(dx, dy);
		}
		
		void UpdateMouseRotation() {
			Vector2 rot = CalcMouseDelta();
			if (game.Input.AltDown && IsThirdPerson) {
				rotOffset.X += rot.X; rotOffset.Y += rot.Y;
				return;
			}
			
			float headY = player.interp.next.HeadY + rot.X;
			float headX = player.interp.next.HeadX + rot.Y;
			LocationUpdate update = LocationUpdate.MakeOri(headY, headX);
			
			// Need to make sure we don't cross the vertical axes, because that gets weird.
			if (update.HeadX >= 90 && update.HeadX <= 270) {
				update.HeadX = player.interp.next.HeadX < 180 ? 90.0f : 270.0f;
			}
			game.LocalPlayer.SetLocation(update, false);
		}
		
		public override void UpdateMouse() {
			if (game.Gui.ActiveScreen.HandlesAllInput) {
				delta = Point.Empty;
			} else if (game.Focused) {
				Point pos = game.DesktopCursorPos;
				delta = new Point(pos.X - previous.X, pos.Y - previous.Y);
				CentreMousePosition();
			}
			UpdateMouseRotation();
		}
		
		protected void CalcViewBobbing(float t, float velTiltScale) {
			if (!game.ViewBobbing) { tiltM = Matrix4.Identity; return; }
			LocalPlayer p = game.LocalPlayer;
			Matrix4 tiltY, velX;
			
			Matrix4.RotateZ(out tiltM, -p.tilt.tiltX * p.anim.bobStrength);
			Matrix4.RotateX(out tiltY, Math.Abs(p.tilt.tiltY) * 3 * p.anim.bobStrength);
			Matrix4.Mult(out tiltM, ref tiltM, ref tiltY);
			
			bobbingHor = (p.anim.bobbingHor * 0.3f) * p.anim.bobStrength;
			bobbingVer = (p.anim.bobbingVer * 0.6f) * p.anim.bobStrength;
			
			float vel = Utils.Lerp(p.OldVelocity.Y + 0.08f, p.Velocity.Y + 0.08f, t);
			Matrix4.RotateX(out velX, -vel * 0.05f * p.tilt.velTiltStrength / velTiltScale);
			Matrix4.Mult(out tiltM, ref tiltM, ref velX);
		}
	}
	
	public class ThirdPersonCamera : PerspectiveCamera {
		public ThirdPersonCamera(Game game, bool forward) : base(game) { this.forward = forward; }
		public override bool IsThirdPerson { get { return true; } }
		
		bool forward;
		float dist = 3;
		public override bool Zoom(float amount) {
			dist = Math.Max(dist - amount, 2);
			return true;
		}
		
		public override Vector2 GetOrientation() {
			Vector2 v = new Vector2(player.HeadYRadians, player.HeadXRadians);
			if (forward) { v.X += (float)Math.PI; v.Y = -v.Y; }
			
			v.X += rotOffset.X * Utils.Deg2Rad; 
			v.Y += rotOffset.Y * Utils.Deg2Rad;
			return v;
		}
		
		public override Vector3 GetPosition(float t) {
			CalcViewBobbing(t, dist);
			Vector3 target = player.EyePosition;
			target.Y += bobbingVer;
			
			// cast ray from player position to camera position
			// this way we can stop if we hit a block in the way
			Vector2 rot = GetOrientation();
			Vector3 dir = -Utils.GetDirVector(rot.X, rot.Y);
			
			Picking.ClipCameraPos(game, target, dir, dist, game.CameraClipPos);
			return game.CameraClipPos.Intersect;
		}
	}
	
	public class FirstPersonCamera : PerspectiveCamera {
		public FirstPersonCamera(Game game) : base(game) { }
		public override bool IsThirdPerson { get { return false; } }
		
		public override Vector2 GetOrientation() {
			return new Vector2(player.HeadYRadians, player.HeadXRadians);
		}
		
		public override Vector3 GetPosition(float t) {
			CalcViewBobbing(t, 1);
			Vector3 camPos = player.EyePosition;
			camPos.Y += bobbingVer;
			
			double headY = player.HeadYRadians;
			camPos.X += bobbingHor * (float)Math.Cos(headY);
			camPos.Z += bobbingHor * (float)Math.Sin(headY);
			return camPos;
		}
	}
}