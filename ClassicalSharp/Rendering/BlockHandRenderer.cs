using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class BlockHandRenderer : IDisposable {
		
		Game game;
		BlockModel block;
		FakePlayer fakePlayer;
		bool playAnimation, leftAnimation, swingAnimation;
		float angleX = 0;
		
		public BlockHandRenderer( Game window ) {
			this.game = window;
		}
		
		public void Init() {
			block = new BlockModel( game );
			fakePlayer = new FakePlayer( game );
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
			
			type = (byte)game.Inventory.HeldBlock;
			if( playAnimation )
				DoAnimation( delta );
			
			if( game.BlockInfo.IsSprite[type] )
				game.Graphics.LoadMatrix( ref spriteMat );
			else
				game.Graphics.LoadMatrix( ref normalMat );
			fakePlayer.Block = type;
			block.RenderModel( fakePlayer );
			
			game.Graphics.LoadMatrix( ref game.View );
			game.Graphics.Texturing = false;
			game.Graphics.DepthTest = true;
			game.Graphics.AlphaTest = false;
		}
		
		double animPeriod = 0.25, animSpeed = Math.PI / 0.25;
		void DoAnimation( double delta ) {
			if( swingAnimation || !leftAnimation ) {
				float oldY = fakePlayer.Position.Y;
				fakePlayer.Position.Y = -1.2f * (float)Math.Sin( animTime * animSpeed );
				if( swingAnimation ) {
					// i.e. the block has gone to bottom of screen and is now returning back up
					// at this point we switch over to the new held block.
					if( fakePlayer.Position.Y > oldY )
						lastType = type;
					type = lastType;
				}
			} else {
				//fakePlayer.Position.X = 0.2f * (float)Math.Sin( animTime * animSpeed );
				fakePlayer.Position.Y = 0.3f * (float)Math.Sin( animTime * animSpeed * 2 );
				fakePlayer.Position.Z = -0.7f * (float)Math.Sin( animTime * animSpeed );
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
			fakePlayer.Position = Vector3.Zero;
			angleX = 0;
			animPeriod = period;
			animSpeed = Math.PI / period;
			 
			SetupMatrices();
			if( updateLastType )
				lastType = (byte)game.Inventory.HeldBlock;
			fakePlayer.Position = Vector3.Zero;
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
