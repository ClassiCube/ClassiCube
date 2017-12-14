// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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
		
		/// <summary> Calculates the projection matrix for this camera. </summary>
		public abstract void GetProjection(out Matrix4 m);
		
		/// <summary> Calculates the world/view matrix for this camera. </summary>
		public abstract void GetView(out Matrix4 m);
		
		/// <summary> Calculates the location of the camera's position in the world. </summary>
		public abstract Vector3 GetCameraPos(float t);
		
		/// <summary> Calculates the yaw and pitch of the camera in radians. </summary>
		public abstract Vector2 GetCameraOrientation();
		
		/// <summary> Whether this camera is using a third person perspective. </summary>
		/// <remarks> Causes the player's own entity model to be renderered if true. </remarks>
		public abstract bool IsThirdPerson { get; }
		
		/// <summary> Attempts to zoom the camera's position closer to or further from its origin point. </summary>
		/// <returns> true if this camera handled zooming </returns>
		/// <example> Third person cameras override this method to zoom in or out, and hence return true.
		/// The first person camera does not perform zomming, so returns false. </example>
		public virtual bool Zoom(float amount) { return false; }
		
		/// <summary> Called every frame for the camera to update its state. </summary>
		/// <example> The perspective cameras gets delta between mouse cursor's current position and the centre of the window,
		/// then uses this to adjust the player's horizontal and vertical rotation.	</example>
		public abstract void UpdateMouse();
		
		/// <summary> Called after the camera has regrabbed the mouse from 2D input handling. </summary>
		/// <example> The perspective cameras set the mouse cursor to the centre of the window. </example>
		public abstract void RegrabMouse();
		
		/// <summary> Calculates the picked block based on the camera's current state. </summary>
		public virtual void GetPickedBlock(PickedPos pos) { }
		
		/// <summary> Adjusts the head X rotation of the player to avoid looking straight up or down. </summary>
		/// <remarks> Looking straight up or down (parallel to camera up vector) can otherwise cause rendering issues. </remarks>
		protected float AdjustHeadX(float value) {
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
		
		protected Vector3 GetDirVector() {
			return Utils.GetDirVector(player.HeadYRadians, AdjustHeadX(player.HeadX));
		}
		
		public override void GetProjection(out Matrix4 m) {
			float fov = game.Fov * Utils.Deg2Rad;
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			game.Graphics.CalcPerspectiveMatrix(fov, aspectRatio, zNear, game.ViewDistance, out m);
		}
		
		public override void GetPickedBlock(PickedPos pos) {
			Vector3 dir = GetDirVector();
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
		const float slippery = 0.97f;
		const float adjust = 0.025f;
		
		static float speedX = 0, speedY = 0;
		void UpdateMouseRotation() {
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
			
			float rotY =  player.interp.next.HeadY + speedX * sensitivity;
			float yAdj =  game.InvertMouse ? -speedY * sensitivity : speedY * sensitivity;
			float headX = player.interp.next.HeadX + yAdj;
			LocationUpdate update = LocationUpdate.MakeOri(rotY, headX);
			
			// Need to make sure we don't cross the vertical axes, because that gets weird.
			if (update.HeadX >= 90 && update.HeadX <= 270)
				update.HeadX = player.interp.next.HeadX < 180 ? 89.9f : 270.1f;
			game.LocalPlayer.SetLocation(update, false);
		}
		
		public override void UpdateMouse() {
			if (game.Gui.ActiveScreen.HandlesAllInput) return;
			CentreMousePosition();
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
		public ThirdPersonCamera(Game window, bool forward) : base(window) { this.forward = forward; }
		public override bool IsThirdPerson { get { return true; } }
		
		bool forward;
		float dist = 3;
		public override bool Zoom(float amount) {
			dist = Math.Max(dist - amount, 2);
			return true;
		}
		
		public override void GetView(out Matrix4 m) {
			Vector3 camPos = game.CurrentCameraPos;
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobbingVer;
			
			Matrix4 lookAt;
			Matrix4.LookAt(camPos, eyePos, Vector3.UnitY, out lookAt);
			Matrix4.Mult(out m, ref lookAt, ref tiltM);
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
	
	public class FirstPersonCamera : PerspectiveCamera {
		public FirstPersonCamera(Game window) : base(window) { }
		public override bool IsThirdPerson { get { return false; } }
		
		public override void GetView(out Matrix4 m) {
			Vector3 camPos = game.CurrentCameraPos;
			Vector3 dir = GetDirVector();
			
			Matrix4 lookAt;
			Matrix4.LookAt(camPos, camPos + dir, Vector3.UnitY, out lookAt);
			Matrix4.Mult(out m, ref lookAt, ref tiltM);
		}
		
		public override Vector2 GetCameraOrientation() {
			return new Vector2(player.HeadYRadians, player.HeadXRadians);
		}
		
		public override Vector3 GetCameraPos(float t) {
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