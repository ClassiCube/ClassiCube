using System;
using System.Drawing;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class Camera {
		protected Game game;
		
		public abstract Matrix4 GetProjection( out Matrix4 heldBlockProj );
		
		public abstract Matrix4 GetView( double delta );
		
		/// <summary> Calculates the location of the camera's position in the world
		/// based on the entity's eye position. </summary>
		public abstract Vector3 GetCameraPos( Vector3 eyePos );
		
		/// <summary> Whether this camera is using a third person perspective. </summary>
		/// <remarks> This causes the local player to be renderered if true. </remarks>
		public abstract bool IsThirdPerson { get; }
		
		public virtual void Tick( double elapsed ) {
		}
		
		public virtual bool MouseZoom( MouseWheelEventArgs e ) {
			return false;
		}
		
		public abstract void RegrabMouse();
		
		/// <summary> Calculates the picked block based on the camera's current position. </summary>
		public virtual void GetPickedBlock( PickedPos pos ) {
		}
		
		protected float AdjustPitch( float value ) {
			if( value >= 90.00f && value <= 90.25f ) return 90.25f * Utils.Deg2Rad;
			if( value >= 89.75f && value <= 90.00f ) return 89.75f * Utils.Deg2Rad;
			if( value >= 270.00f && value <= 270.25f ) return 270.25f * Utils.Deg2Rad;
			if( value >= 269.75f && value <= 270.00f ) return 269.75f * Utils.Deg2Rad;
			return value * Utils.Deg2Rad;
		}
	}
	
	public abstract class PerspectiveCamera : Camera {
		
		protected LocalPlayer player;
		protected Matrix4 tiltMatrix;
		public PerspectiveCamera( Game game ) {
			this.game = game;
			player = game.LocalPlayer;
			tiltMatrix = Matrix4.Identity;
		}
		
		public override Matrix4 GetProjection( out Matrix4 heldBlockProj ) {
			float fovy = game.FieldOfView * Utils.Deg2Rad;
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			heldBlockProj = Matrix4.CreatePerspectiveFieldOfView( 70 * Utils.Deg2Rad,
			                                                     aspectRatio, zNear, game.ViewDistance );
			return Matrix4.CreatePerspectiveFieldOfView( fovy, aspectRatio, zNear, game.ViewDistance );
		}
		
		public override void GetPickedBlock( PickedPos pos ) {
			Vector3 dir = Utils.GetDirVector( player.HeadYawRadians, 
			                                 AdjustPitch( player.PitchDegrees ) );
			Vector3 eyePos = player.EyePosition;
			float reach = game.LocalPlayer.ReachDistance;
			Picking.CalculatePickedBlock( game, eyePos, dir, reach, pos );
		}
		
		internal Point previous, delta;
		void CentreMousePosition() {
			if( !game.Focused ) return;
			Point current = game.DesktopCursorPos;
			delta = new Point( current.X - previous.X, current.Y - previous.Y );
			Point topLeft = game.PointToScreen( Point.Empty );
			int cenX = topLeft.X + game.Width / 2;
			int cenY = topLeft.Y + game.Height / 2;
			game.DesktopCursorPos = new Point( cenX, cenY );
			// Fixes issues with large DPI displays on Windows >= 8.0.
			previous = game.DesktopCursorPos;
		}
		
		public override void RegrabMouse() {
			if( !game.Exists ) return;
			Point topLeft = game.PointToScreen( Point.Empty );
			int cenX = topLeft.X + game.Width / 2;
			int cenY = topLeft.Y + game.Height / 2;
			game.DesktopCursorPos = new Point( cenX, cenY );
			previous = new Point( cenX, cenY );
			delta = Point.Empty;
		}
		
		static readonly float sensiFactor = 0.0002f / 3 * Utils.Rad2Deg;
		private void UpdateMouseRotation() {
			float sensitivity = sensiFactor * game.MouseSensitivity;
			float yaw = player.nextYaw + delta.X * sensitivity;
			float yAdj = game.InvertMouse ? -delta.Y * sensitivity : delta.Y * sensitivity;
			float pitch = player.nextPitch + yAdj;
			LocationUpdate update = LocationUpdate.MakeOri( yaw, pitch );
			
			// Need to make sure we don't cross the vertical axes, because that gets weird.
			if( update.Pitch >= 90 && update.Pitch <= 270 )
				update.Pitch = player.nextPitch < 180 ? 89.9f : 270.1f;
			game.LocalPlayer.SetLocation( update, true );
		}

		public override void Tick( double elapsed ) {
			if( game.ScreenLockedInput ) return;
			CentreMousePosition();
			UpdateMouseRotation();
		}
		
		protected float bobYOffset = 0;		
		protected void CalcViewBobbing( double delta ) {
			if( !game.ViewBobbing || !game.LocalPlayer.onGround ) {
				tiltMatrix = Matrix4.Identity;
				bobYOffset = 0;
			} else {
				tiltMatrix = Matrix4.RotateZ( game.LocalPlayer.anim.tilt );
				bobYOffset = game.LocalPlayer.anim.bobYOffset * (2.0f/2.5f);
			}
		}
	}
	
	public class ThirdPersonCamera : PerspectiveCamera {
		
		public ThirdPersonCamera( Game window ) : base( window ) {
		}
		
		float distance = 3;
		public override bool MouseZoom( MouseWheelEventArgs e ) {
			distance -= e.DeltaPrecise;
			if( distance < 2 ) distance = 2;
			return true;
		}
		
		public override Matrix4 GetView( double delta ) {
			CalcViewBobbing( delta );
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobYOffset;
			
			Vector3 dir = -Utils.GetDirVector( player.HeadYawRadians, 
			                                  AdjustPitch( player.PitchDegrees ) );
			Picking.ClipCameraPos( game, eyePos, dir, distance, game.CameraClipPos );
			Vector3 cameraPos = game.CameraClipPos.IntersectPoint;
			return Matrix4.LookAt( cameraPos, eyePos, Vector3.UnitY ) * tiltMatrix;
		}
		
		public override bool IsThirdPerson { get { return true; } }
		
		public override Vector3 GetCameraPos( Vector3 eyePos ) {
			Vector3 dir = -Utils.GetDirVector( player.HeadYawRadians, 
			                                  AdjustPitch( player.PitchDegrees ) );
			Picking.ClipCameraPos( game, eyePos, dir, distance, game.CameraClipPos );
			return game.CameraClipPos.IntersectPoint;
		}
	}
	
	public class ForwardThirdPersonCamera : PerspectiveCamera {
		
		public ForwardThirdPersonCamera( Game window ) : base( window ) {
		}
		
		float distance = 3;
		public override bool MouseZoom( MouseWheelEventArgs e ) {
			distance -= e.DeltaPrecise;
			if( distance < 2 ) distance = 2;
			return true;
		}
		
		public override Matrix4 GetView( double delta ) {
			CalcViewBobbing( delta );
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobYOffset;
			
			Vector3 dir = Utils.GetDirVector( player.HeadYawRadians, 
			                                 AdjustPitch( player.PitchDegrees ) );
			Picking.ClipCameraPos( game, eyePos, dir, distance, game.CameraClipPos );
			Vector3 cameraPos = game.CameraClipPos.IntersectPoint;
			return Matrix4.LookAt( cameraPos, eyePos, Vector3.UnitY ) * tiltMatrix;
		}
		
		public override bool IsThirdPerson { get { return true; } }
		
		public override Vector3 GetCameraPos( Vector3 eyePos ) {
			Vector3 dir = Utils.GetDirVector( player.HeadYawRadians, 
			                                 AdjustPitch( player.PitchDegrees ) );
			Picking.ClipCameraPos( game, eyePos, dir, distance, game.CameraClipPos );
			return game.CameraClipPos.IntersectPoint;
		}
	}
	
	public class FirstPersonCamera : PerspectiveCamera {
		
		public FirstPersonCamera( Game window ) : base( window ) {
		}	
		
		public override Matrix4 GetView( double delta ) {
			CalcViewBobbing( delta );
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobYOffset;
			Vector3 cameraDir = Utils.GetDirVector( player.HeadYawRadians, 
			                                       AdjustPitch( player.PitchDegrees ) );
			return Matrix4.LookAt( eyePos, eyePos + cameraDir, Vector3.UnitY ) * tiltMatrix;
		}
		
		public override bool IsThirdPerson { get { return false; } }
		
		public override Vector3 GetCameraPos( Vector3 eyePos ) {
			return eyePos;
		}
	}
}
