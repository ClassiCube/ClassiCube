using System;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class BlockHandRenderer : IDisposable {
		
		Game game;
		BlockModel block;
		FakePlayer fakePlayer;
		
		public BlockHandRenderer( Game window ) {
			this.game = window;
		}
		
		public void Init() {
			block = new BlockModel( game );
			fakePlayer = new FakePlayer( game );
			SetupMatrices();
		}
		
		public void Render( double delta ) {
			game.Graphics.Texturing = true;
			game.Graphics.DepthTest = false;
			game.Graphics.AlphaTest = true;
			
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
	}
	
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
