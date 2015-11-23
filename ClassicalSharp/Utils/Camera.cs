using System;
using System.Drawing;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class Camera {
		protected Game game;
		
		public abstract Matrix4 GetProjection();
		
		public abstract Matrix4 GetView( double delta );
		
		/// <summary> Calculates the location of the camera's position in the world
		/// based on the entity's eye position. </summary>
		public abstract Vector3 GetCameraPos( Vector3 eyePos );
		
		/// <summary> Whether this camera is using a third person perspective. </summary>
		/// <remarks> This causes the local player to be renderered if true. </remarks>
		public abstract bool IsThirdPerson { get; }
		
		public virtual void Tick( double elapsed ) {
		}
		
		public virtual void PlayerTick( double elapsed, Vector3 prevVel,
		                               Vector3 nextVel, bool onGround ) {
		}
		
		public virtual bool MouseZoom( MouseWheelEventArgs e ) {
			return false;
		}
		
		public abstract void RegrabMouse();
		
		/// <summary> Calculates the picked block based on the camera's current position. </summary>
		public virtual void GetPickedBlock( PickedPos pos ) {
		}
	}
	
	public abstract class PerspectiveCamera : Camera {
		
		protected LocalPlayer player;
		protected Matrix4 BobMatrix;
		public PerspectiveCamera( Game game ) {
			this.game = game;
			player = game.LocalPlayer;
			BobMatrix = Matrix4.Identity;
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
			Picking.CalculatePickedBlock( game, eyePos, dir, reach, pos );
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
		
		static readonly float sensiFactor = 0.0002f / 3 * Utils.Rad2Deg;
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
		
		float speed = 0;
		public override void PlayerTick( double elapsed, Vector3 prevVel,
		                                Vector3 nextVel, bool onGround ) {
			if( !onGround || !game.ViewBobbing ) {
				speed = 0; return;
			}
			
			float dist = HorLength( nextVel ) - HorLength( prevVel );		
			if( zero( nextVel.X ) && zero( nextVel.Z ) &&
			   zero( prevVel.X ) && zero( prevVel.Z ) ) {
				speed = 0;
			} else {
				float sqrDist = (float)Math.Sqrt( Math.Abs( dist ) * 120 );
				speed += Math.Sign( dist ) * sqrDist;
			}
			Utils.Clamp( ref speed, 0, 15f );
		}
		
		bool zero( float value ) {
			return Math.Abs( value ) < 0.001f;
		}
		
		float HorLength( Vector3 v ) {
			return (float)Math.Sqrt( v.X * v.X + v.Z * v.Z );
		}
		
		double bobAccumulator;
		protected float bobYOffset = 0;
		const float angle = 0.25f * Utils.Deg2Rad;
		
		protected void CalcBobMatix( double delta ) {
			bobAccumulator += delta * speed;
			if( speed == 0 ) {
				BobMatrix = Matrix4.Identity;
				bobYOffset = 0;
			} else {
				float time = (float)Math.Sin( bobAccumulator );
				BobMatrix = Matrix4.RotateZ( time * angle );
				bobYOffset = (float)Math.Sin( bobAccumulator ) * 0.1f;
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
			CalcBobMatix( delta );
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobYOffset;
			Vector3 cameraPos = eyePos - Utils.GetDirVector( player.YawRadians, player.PitchRadians ) * distance;
			return Matrix4.LookAt( cameraPos, eyePos, Vector3.UnitY ) * BobMatrix;
		}
		
		public override bool IsThirdPerson {
			get { return true; }
		}
		
		public override Vector3 GetCameraPos( Vector3 eyePos ) {
			return eyePos - Utils.GetDirVector( player.YawRadians, player.PitchRadians ) * distance;
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
			CalcBobMatix( delta );
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobYOffset;
			Vector3 cameraPos = eyePos + Utils.GetDirVector( player.YawRadians, player.PitchRadians ) * distance;
			return Matrix4.LookAt( cameraPos, eyePos, Vector3.UnitY ) * BobMatrix;
		}
		
		public override bool IsThirdPerson {
			get { return true; }
		}
		
		public override Vector3 GetCameraPos( Vector3 eyePos ) {
			return eyePos + Utils.GetDirVector( player.YawRadians, player.PitchRadians ) * distance;
		}
	}
	
	public class FirstPersonCamera : PerspectiveCamera {
		
		public FirstPersonCamera( Game window ) : base( window ) {
		}
		
		
		public override Matrix4 GetView( double delta ) {
			CalcBobMatix( delta );
			Vector3 eyePos = player.EyePosition;
			eyePos.Y += bobYOffset;
			Vector3 cameraDir = Utils.GetDirVector( player.YawRadians, player.PitchRadians );
			return Matrix4.LookAt( eyePos, eyePos + cameraDir, Vector3.UnitY ) * BobMatrix;
		}
		
		public override bool IsThirdPerson {
			get { return false; }
		}
		
		public override Vector3 GetCameraPos( Vector3 eyePos ) {
			return eyePos;
		}
	}
}
