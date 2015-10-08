using System;
using System.Drawing;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class Camera {
		protected Game game;
		
		public abstract Matrix4 GetProjection();
		
		public abstract Matrix4 GetView();
		
		public abstract bool IsThirdPerson { get; }
		
		public virtual void Tick( double elapsed ) {
		}
		
		public virtual bool MouseZoom( MouseWheelEventArgs e ) {
			return false;
		}
		
		public abstract void RegrabMouse();
		
		public virtual void GetPickedBlock( PickedPos pos ) {
		}
	}
	
	public abstract class PerspectiveCamera : Camera {
		
		protected LocalPlayer player;
		public PerspectiveCamera( Game game ) {
			this.game = game;
			player = game.LocalPlayer;
		}
		
		public override Matrix4 GetProjection() {
			float fovy = 70 * Utils.Deg2Rad;
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			return Matrix4.CreatePerspectiveFieldOfView( fovy, aspectRatio, zNear, game.ViewDistance );
		}
		
		public override void GetPickedBlock( PickedPos pos ) {
			Vector3 dir = Utils.GetDirVector( player.YawRadians, player.PitchRadians );
			Vector3 eyePos = player.EyePosition;
			float reach = game.LocalPlayer.ReachDistance;
			Picking.GetPickedBlockPos( game, eyePos, dir, reach, pos );
		}
		
		internal Point previous, delta;
		void CentreMousePosition() {
			if( !game.Focused ) return;
			Point current = game.DesktopCursorPos;
			delta = new Point( current.X - previous.X, current.Y - previous.Y );
			Rectangle bounds = game.Bounds;
			int cenX = bounds.Left + bounds.Width / 2;
			int cenY = bounds.Top + bounds.Height / 2;
			game.DesktopCursorPos = new Point( cenX, cenY );
			// Fixes issues with large DPI displays on Windows >= 8.0.
			previous = game.DesktopCursorPos;
		}
		
		public override void RegrabMouse() {
			Rectangle bounds = game.Bounds;
			int cenX = bounds.Left + bounds.Width / 2;
			int cenY = bounds.Top + bounds.Height / 2;
			game.DesktopCursorPos = new Point( cenX, cenY );
			previous = new Point( cenX, cenY );
			delta = Point.Empty;
		}
		
		static readonly float sensiFactor = 0.0002f * Utils.Rad2Deg;
		private void UpdateMouseRotation() {
			float sensitivity = sensiFactor * game.MouseSensitivity;
			float yaw = player.nextYaw + delta.X * sensitivity;
			float pitch = player.nextPitch + delta.Y * sensitivity;
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
		
		public override Matrix4 GetView() {
			Vector3 eyePos = player.EyePosition;
			Vector3 cameraPos = eyePos - Utils.GetDirVector( player.YawRadians, player.PitchRadians ) * distance;
			return Matrix4.LookAt( cameraPos, eyePos, Vector3.UnitY );
		}
		
		public override bool IsThirdPerson {
			get { return true; }
		}
	}
	
	public class FirstPersonCamera : PerspectiveCamera {
		
		public FirstPersonCamera( Game window ) : base( window ) {
		}
		
		public override Matrix4 GetView() {
			Vector3 eyePos = player.EyePosition;
			Vector3 cameraDir = Utils.GetDirVector( player.YawRadians, player.PitchRadians );
			return Matrix4.LookAt( eyePos, eyePos + cameraDir, Vector3.UnitY );
		}
		
		public override bool IsThirdPerson {
			get { return false; }
		}
	}
}
