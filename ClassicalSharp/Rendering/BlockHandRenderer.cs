// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class BlockHandRenderer : IGameComponent {
		
		Game game;
		BlockModel block;
		FakePlayer fakeP;
		bool playAnimation, leftAnimation, swingAnimation;
		float angleX = 0;
		
		double animTime;
		byte type, lastType;
		
		Matrix4 normalMat, spriteMat;
		byte lastMatrixType;
		float lastMatrixAngleX;
		Vector3 animPosition;
		Matrix4 heldBlockProj;
		
		public void Init( Game game ) {
			this.game = game;
			block = new BlockModel( game );
			fakeP = new FakePlayer( game );
			lastType = game.Inventory.HeldBlock;
			game.Events.HeldBlockChanged += HeldBlockChanged;
			game.UserEvents.BlockChanged += BlockChanged;
			game.Events.ProjectionChanged += ProjectionChanged;
		}

		public void Ready( Game game ) { }
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		public void Render( double delta, float t ) {
			//if( game.Camera.IsThirdPerson || !game.ShowBlockInHand ) return;

			Vector3 last = animPosition;
			animPosition = Vector3.Zero;
			type = game.Inventory.HeldBlock;
			if( playAnimation ) DoAnimation( delta, last );
			PerformViewBobbing( t );
			
			SetMatrix();
			game.Graphics.SetMatrixMode( MatrixType.Projection );
			game.Graphics.LoadMatrix( ref heldBlockProj );
			bool translucent = game.BlockInfo.IsTranslucent[type];
			
			game.Graphics.Texturing = true;
			game.Graphics.DepthTest = false;
			if( translucent ) game.Graphics.AlphaBlending = true;
			else game.Graphics.AlphaTest = true;
			
			SetPos();
			block.Render( fakeP );
			
			game.Graphics.LoadMatrix( ref game.Projection );
			game.Graphics.SetMatrixMode( MatrixType.Modelview );
			game.Graphics.LoadMatrix( ref game.View );
			
			game.Graphics.Texturing = false;
			game.Graphics.DepthTest = true;
			if( translucent ) game.Graphics.AlphaBlending = false;
			else game.Graphics.AlphaTest = false;
		}
		
		static Vector3 nOffset = new Vector3( 0.56f, -0.72f, -0.72f );
		internal static Vector3 sOffset = new Vector3( 0.46f, -0.52f, -0.72f );
		
		void SetMatrix() {
			Player p = game.LocalPlayer;
			Vector3 eyePos = p.EyePosition;
			Matrix4 m = Matrix4.LookAt( eyePos, eyePos + -Vector3.UnitZ, Vector3.UnitY );
			game.Graphics.LoadMatrix( ref m );
		}
		
		void SetPos() {
			// Based off details from http://pastebin.com/KFV0HkmD (Thanks goodlyay!)
			BlockInfo info = game.BlockInfo;
			Vector3 offset = info.IsSprite[type] ? sOffset : nOffset;
			Player p = game.LocalPlayer;
			fakeP.ModelScale = 0.4f;
			
			fakeP.Position = p.EyePosition + animPosition;
			fakeP.Position += offset;
			if( !info.IsSprite[type] ) {
				float height = info.MaxBB[type].Y - info.MinBB[type].Y;
				fakeP.Position.Y += 0.2f * (1 - height);
			}
			fakeP.Position.Y += p.anim.bobYOffset;
			
			fakeP.HeadYawDegrees = 45f;
			fakeP.YawDegrees = 45f;
			fakeP.PitchDegrees = 0;
			fakeP.Block = type;
		}
		
		double animPeriod = 0.25, animSpeed = Math.PI / 0.25;
		void DoAnimation( double delta, Vector3 last ) {
			if( swingAnimation || !leftAnimation ) {
				animPosition.Y = -0.4f * (float)Math.Sin( animTime * animSpeed );
				if( swingAnimation ) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if( animPosition.Y > last.Y )
						lastType = type;
					type = lastType;
				}
			} else {
				animPosition.X = -0.325f * (float)Math.Sin( animTime * animSpeed );
				animPosition.Y = 0.2f * (float)Math.Sin( animTime * animSpeed * 2 );
				animPosition.Z = -0.325f * (float)Math.Sin( animTime * animSpeed );
				angleX = 20 * (float)Math.Sin( animTime * animSpeed * 2 );
			}
			animTime += delta;
			if( animTime > animPeriod )
				ResetAnimationState( true, 0.25 );
		}
		
		void ResetAnimationState( bool updateLastType, double period ) {
			animTime = 0;
			playAnimation = false;
			animPosition = Vector3.Zero;
			angleX = 0;
			animPeriod = period;
			animSpeed = Math.PI / period;
			
			if( updateLastType )
				lastType = game.Inventory.HeldBlock;
			animPosition = Vector3.Zero;
		}
		
		/// <summary> Sets the current animation state of the held block.<br/>
		/// true = left mouse pressed, false = right mouse pressed. </summary>
		public void SetAnimationClick( bool left ) {
			ResetAnimationState( true, 2.25 );
			swingAnimation = false;
			leftAnimation = left;
			playAnimation = true;
		}
		
		public void SetAnimationSwitchBlock() {
			ResetAnimationState( false, 0.3 );
			swingAnimation = true;
			playAnimation = true;
		}
		
		float bobTimeO, bobTimeN;
		void PerformViewBobbing( float t ) {
			if( !game.ViewBobbing ) return;
			LocalPlayer p = game.LocalPlayer;
			float bobTime = Utils.Lerp( bobTimeO, bobTimeN, t );
			
			double horTime = Math.Sin( bobTime ) * p.curSwing;
			// (0.5 + 0.5cos(2x)) is smoother than abs(cos(x)) at endpoints
			double verTime = Math.Cos( bobTime * 2 );
			float horAngle = 0.2f * (float)horTime;
			float verAngle = 0.2f * (float)(0.5 + 0.5 * verTime) * p.curSwing;
			
			if( horAngle > 0 )
				animPosition.X += horAngle;
			else
				animPosition.Z += horAngle;
			animPosition.Y -= verAngle;
		}
		
		void HeldBlockChanged( object sender, EventArgs e ) {
			SetAnimationSwitchBlock();
		}
		
		bool stop = false;
		public void Tick( double delta ) {
			Player p = game.LocalPlayer;
			float bobTimeDelta = p.anim.walkTimeN - p.anim.walkTimeO;
			bobTimeO = bobTimeN;

			if( game.LocalPlayer.onGround ) stop = false;
			if( stop ) return;
			bobTimeN += bobTimeDelta;
			if( game.LocalPlayer.onGround ) return;

			// Keep returning the held block back to centre position.
			if( Math.Sign( Math.Sin( bobTimeO ) ) == Math.Sign( Math.Sin( bobTimeN ) ) )
				return;
			// Stop bob time at next periodic '0' angle.
			double left = Math.PI - (bobTimeO % Math.PI);
			bobTimeN = (float)(bobTimeO + left);
			stop = true;
		}
		
		void ProjectionChanged( object sender, EventArgs e ) {
			float aspectRatio = (float)game.Width / game.Height;
			float zNear = game.Graphics.MinZNear;
			heldBlockProj = Matrix4.CreatePerspectiveFieldOfView( 70 * Utils.Deg2Rad,
			                                                     aspectRatio, zNear, game.ViewDistance );
		}
		
		public void Dispose() {
			game.Events.HeldBlockChanged -= HeldBlockChanged;
			game.UserEvents.BlockChanged -= BlockChanged;
			game.Events.ProjectionChanged -= ProjectionChanged;
		}
		
		void BlockChanged( object sender, BlockChangedEventArgs e ) {
			if( e.Block == 0 ) return;
			SetAnimationClick( false );
		}
	}
	
	/// <summary> Skeleton implementation of a player entity so we can reuse
	/// block model rendering code. </summary>
	internal class FakePlayer : Player {
		
		public FakePlayer( Game game ) : base( game ) {
		}
		public byte Block;
		
		public override void SetLocation( LocationUpdate update, bool interpolate ) { }
		
		public override void Tick( double delta ) { }

		public override void RenderModel( double deltaTime, float t ) { }
		
		public override void RenderName() { }
	}
}
