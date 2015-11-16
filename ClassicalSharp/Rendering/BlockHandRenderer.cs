using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class BlockHandRenderer : IDisposable {
		
		Game game;
		BlockModel block;
		FakePlayer fakePlayer;
		bool playAnimation, leftAnimation;
		
		public BlockHandRenderer( Game window ) {
			this.game = window;
		}
		
		public void Init() {
			block = new BlockModel( game );
			fakePlayer = new FakePlayer( game );
			SetupMatrices();
		}
		
		double animTime;
		public void Render( double delta ) {
			if( game.Camera.IsThirdPerson )
				return;
			game.Graphics.Texturing = true;
			game.Graphics.DepthTest = false;
			game.Graphics.AlphaTest = true;
			
			fakePlayer.Position = Vector3.Zero;
			if( playAnimation )
				DoAnimation( delta );
			
			byte type = (byte)game.Inventory.HeldBlock;
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
		
		const double animPeriod = 0.25;
		const double animSpeed = Math.PI / 0.25;
		void DoAnimation( double delta ) {
			if( !leftAnimation ) {
				fakePlayer.Position.Y = -(float)Math.Sin( animTime * animSpeed );
			} else {
				
			}
			
			animTime += delta;
			if( animTime > animPeriod ) {
				animTime = 0;
				playAnimation = false;
				fakePlayer.Position = Vector3.Zero;
			}
		}
		
		Matrix4 normalMat, spriteMat;
		void SetupMatrices() {
			Matrix4 m = Matrix4.Identity;
			m = m * Matrix4.Scale( 0.6f );
			m = m * Matrix4.RotateY( 45 * Utils.Deg2Rad );
			
			normalMat = m * Matrix4.Translate( 0.85f, -1.35f, -1.5f );
			spriteMat = m * Matrix4.Translate( 0.85f, -1.05f, -1.5f );
		}
		
		public void Dispose() {
		}
		
		/// <summary> Sets the current animation state of the held block.<br/>
		/// true = left mouse pressed, false = right mouse pressed. </summary>
		public void SetAnimation( bool left ) {
			playAnimation = true;
			animTime = 0;
			leftAnimation = left;
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
