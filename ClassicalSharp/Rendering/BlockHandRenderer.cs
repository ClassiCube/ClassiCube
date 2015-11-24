using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class BlockHandRenderer : IDisposable {
		
		Game game;
		BlockModel block;
		FakePlayer fakeP;
		bool playAnimation, leftAnimation, swingAnimation;
		float angleX = 0;
		
		public BlockHandRenderer( Game window ) {
			this.game = window;
		}
		
		public void Init() {
			block = new BlockModel( game );
			fakeP = new FakePlayer( game );
			SetupMatrices();
			lastType = (byte)game.Inventory.HeldBlock;
			game.Events.HeldBlockChanged += HeldBlockChanged;
		}
		
		double animTime;
		byte type, lastType;
		public void Render( double delta ) {
			if( game.Camera.IsThirdPerson )
				return;
			game.Graphics.Texturing = true;
			game.Graphics.DepthTest = false;
			game.Graphics.AlphaTest = true;
			
			fakeP.Position = Vector3.Zero;
			type = (byte)game.Inventory.HeldBlock;
			if( playAnimation )
				DoAnimation( delta );
			PerformViewBobbing();
			
			if( game.BlockInfo.IsSprite[type] )
				game.Graphics.LoadMatrix( ref spriteMat );
			else
				game.Graphics.LoadMatrix( ref normalMat );
			fakeP.Block = type;
			block.RenderModel( fakeP );
			
			game.Graphics.LoadMatrix( ref game.View );
			game.Graphics.Texturing = false;
			game.Graphics.DepthTest = true;
			game.Graphics.AlphaTest = false;
		}
		
		double animPeriod = 0.25, animSpeed = Math.PI / 0.25;
		void DoAnimation( double delta ) {
			if( swingAnimation || !leftAnimation ) {
				float oldY = fakeP.Position.Y;
				fakeP.Position.Y = -1.2f * (float)Math.Sin( animTime * animSpeed );
				if( swingAnimation ) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if( fakeP.Position.Y > oldY )
						lastType = type;
					type = lastType;
				}
			} else {
				//fakePlayer.Position.X = 0.2f * (float)Math.Sin( animTime * animSpeed );
				fakeP.Position.Y = 0.3f * (float)Math.Sin( animTime * animSpeed * 2 );
				fakeP.Position.Z = -0.7f * (float)Math.Sin( animTime * animSpeed );
				angleX = 20 * (float)Math.Sin( animTime * animSpeed * 2 );
				SetupMatrices();
			}
			animTime += delta;
			if( animTime > animPeriod )
				ResetAnimationState( true, 0.25 );
		}
		
		Matrix4 normalMat, spriteMat;
		void SetupMatrices() {
			Matrix4 m = Matrix4.Identity;
			m = m * Matrix4.Scale( 0.6f );
			m = m * Matrix4.RotateY( 45 * Utils.Deg2Rad );
			m = m * Matrix4.RotateX( angleX * Utils.Deg2Rad );
			
			normalMat = m * Matrix4.Translate( 0.85f, -1.35f, -1.5f );
			spriteMat = m * Matrix4.Translate( 0.85f, -1.05f, -1.5f );
		}
		
		void ResetAnimationState( bool updateLastType, double period ) {
			animTime = 0;
			playAnimation = false;
			fakeP.Position = Vector3.Zero;
			angleX = 0;
			animPeriod = period;
			animSpeed = Math.PI / period;
			 
			SetupMatrices();
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
		
		void PerformViewBobbing() {
			if( !game.ViewBobbing ) return;
			float horAngle = 0.25f * (float)Math.Sin( game.accumulator * 5 );
			// (0.5 + 0.5cos(2x)) is smoother than abs(cos(x)) at endpoints
			float verAngle = (float)(0.5 + 0.5 * Math.Cos( game.accumulator * 10 ) );
			verAngle = 0.25f * (float)Math.Abs( verAngle );
			
			if( horAngle > 0 ) 
				fakeP.Position.X += horAngle;
			else 
				fakeP.Position.Z += horAngle;
			fakeP.Position.Y -= verAngle;
		}
		
		void HeldBlockChanged( object sender, EventArgs e ) {
			SetAnimationSwitchBlock();
		}
		
		public void Dispose() {
			game.Events.HeldBlockChanged -= HeldBlockChanged;
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
