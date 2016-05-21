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
		
		public void Init( Game game ) {
			this.game = game;
			block = new BlockModel( game );
			fakeP = new FakePlayer( game );
			lastType = (byte)game.Inventory.HeldBlock;
			game.Events.HeldBlockChanged += HeldBlockChanged;
			game.UserEvents.BlockChanged += BlockChanged;
		}

		public void Ready( Game game ) { }		
		public void Reset( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		
		public void Render( double delta, float t ) {
			if( game.Camera.IsThirdPerson || !game.ShowBlockInHand ) return;

			Vector3 last = fakeP.Position;
			fakeP.Position = Vector3.Zero;
			type = (byte)game.Inventory.HeldBlock;
			if( playAnimation ) DoAnimation( delta, last );
			PerformViewBobbing( t );
			SetupMatrices();
			
			if( game.BlockInfo.IsSprite[type] )
				game.Graphics.LoadMatrix( ref spriteMat );
			else
				game.Graphics.LoadMatrix( ref normalMat );
			game.Graphics.SetMatrixMode( MatrixType.Projection );
			game.Graphics.LoadMatrix( ref game.HeldBlockProjection );
			bool translucent = game.BlockInfo.IsTranslucent[type];
			
			game.Graphics.Texturing = true;
			game.Graphics.DepthTest = false;
			if( translucent ) game.Graphics.AlphaBlending = true;
			else game.Graphics.AlphaTest = true;
			fakeP.Block = type;
			block.Render( fakeP );
			
			game.Graphics.LoadMatrix( ref game.Projection );
			game.Graphics.SetMatrixMode( MatrixType.Modelview );
			game.Graphics.LoadMatrix( ref game.View );
			
			game.Graphics.Texturing = false;
			game.Graphics.DepthTest = true;
			if( translucent ) game.Graphics.AlphaBlending = false;
			else game.Graphics.AlphaTest = false;
		}
		
		double animPeriod = 0.25, animSpeed = Math.PI / 0.25;
		void DoAnimation( double delta, Vector3 last ) {
			if( swingAnimation || !leftAnimation ) {
				fakeP.Position.Y = -1.2f * (float)Math.Sin( animTime * animSpeed );
				if( swingAnimation ) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if( fakeP.Position.Y > last.Y )
						lastType = type;
					type = lastType;
				}
			} else {
				//fakePlayer.Position.X = 0.2f * (float)Math.Sin( animTime * animSpeed );
				fakeP.Position.Y = 0.3f * (float)Math.Sin( animTime * animSpeed * 2 );
				fakeP.Position.Z = -0.7f * (float)Math.Sin( animTime * animSpeed );
				angleX = 20 * (float)Math.Sin( animTime * animSpeed * 2 );
			}
			animTime += delta;
			if( animTime > animPeriod )
				ResetAnimationState( true, 0.25 );
		}
		
		void SetupMatrices() {
			if( type == lastMatrixType && lastMatrixAngleX == angleX )
				return;
			Matrix4 m = Matrix4.Identity;
			m = m * Matrix4.Scale( 0.6f );
			m = m * Matrix4.RotateY( 45 * Utils.Deg2Rad );
			m = m * Matrix4.RotateX( angleX * Utils.Deg2Rad );
			
			Vector3 minBB = game.BlockInfo.MinBB[type];
			Vector3 maxBB = game.BlockInfo.MaxBB[type];
			float height = (maxBB.Y - minBB.Y);
			if( game.BlockInfo.IsSprite[type] )
				height = 1;
			
			float offset = (1 - height) * 0.4f;
			normalMat = m * Matrix4.Translate( 0.85f, -1.35f + offset, -1.5f );
			spriteMat = m * Matrix4.Translate( 0.85f, -1.05f + offset, -1.5f );
			lastMatrixType = type;
			lastMatrixAngleX = angleX;
		}
		
		void ResetAnimationState( bool updateLastType, double period ) {
			animTime = 0;
			playAnimation = false;
			fakeP.Position = Vector3.Zero;
			angleX = 0;
			animPeriod = period;
			animSpeed = Math.PI / period;
			
			if( updateLastType )
				lastType = (byte)game.Inventory.HeldBlock;
			fakeP.Position = Vector3.Zero;
		}
		
		/// <summary> Sets the current animation state of the held block.<br/>
		/// true = left mouse pressed, false = right mouse pressed. </summary>
		public void SetAnimationClick( bool left ) {
			ResetAnimationState( true, 0.25 );
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
				fakeP.Position.X += horAngle;
			else
				fakeP.Position.Z += horAngle;
			fakeP.Position.Y -= verAngle;
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
		
		public void Dispose() {
			game.Events.HeldBlockChanged -= HeldBlockChanged;
			game.UserEvents.BlockChanged -= BlockChanged;
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
