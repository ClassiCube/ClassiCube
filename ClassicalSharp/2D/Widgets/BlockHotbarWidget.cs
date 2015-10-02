using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class BlockHotbarWidget : Widget {
		
		public BlockHotbarWidget( Game game ) : base( game ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.BottomOrRight;
			game.Events.HeldBlockChanged += HeldBlockChanged;
		}
		
		Texture[] barTextures = new Texture[9];
		Texture selectedBlock;
		const int blockSize = 32;
		
		public override bool HandlesKeyDown( Key key ) {
			if( key >= Key.Number1 && key <= Key.Number9 ) {
				game.Inventory.HeldBlockIndex = (int)key - (int)Key.Number1;
				return true;
			}
			return false;
		}
		
		public override void Init() {
			int y = game.Height - blockSize;
			
			Size size = new Size( 32, 32 );
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					drawer.DrawRectBounds( Color.White, blockSize / 8, 0, 0, blockSize, blockSize );
					selectedBlock = drawer.Make2DTexture( bmp, size, 0, y );
				}
			}
			
			int x = game.Width / 2 - ( blockSize * barTextures.Length ) / 2;
			X = x;
			Y = y;
			Width = blockSize * barTextures.Length;
			Height = blockSize;
			
			for( int i = 0; i < barTextures.Length; i++ ) {
				barTextures[i] = MakeTexture( x, y, game.Inventory.Hotbar[i] );
				x += blockSize;
			}
		}
		
		public override void Render( double delta ) {
			graphicsApi.Texturing = true;
			// TODO: Maybe redesign this so we don't have to bind the whole atlas. Not cheap.
			graphicsApi.BindTexture( game.TerrainAtlas.TexId );
			int selectedX = 0;
			for( int i = 0; i < barTextures.Length; i++ ) {
				barTextures[i].RenderNoBind( graphicsApi );
				if( i == game.Inventory.HeldBlockIndex ) {
					selectedX = barTextures[i].X1;
				}
			}
			selectedBlock.X1 = selectedX;
			selectedBlock.Render( graphicsApi );
			graphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			graphicsApi.DeleteTexture( ref selectedBlock );
			game.Events.HeldBlockChanged -= HeldBlockChanged;
		}
		
		void HeldBlockChanged( object sender, EventArgs e ) {
			int index = game.Inventory.HeldBlockIndex;
			Block block = game.Inventory.HeldBlock;
			int x = barTextures[index].X1;
			barTextures[index] = MakeTexture( x, Y, block );
		}
		
		Texture MakeTexture( int x, int y, Block block ) {
			int texId = game.BlockInfo.GetTextureLoc( (byte)block, TileSide.Left );
			TextureRectangle rec = game.TerrainAtlas.GetTexRec( texId );
			
			int verSize = blockSize;
			float height = game.BlockInfo.Height[(byte)block];
			int blockY = y;
			if( height != 1 ) {
				rec.V1 = rec.V1 + TerrainAtlas2D.invElementSize * height;
				verSize = (int)( blockSize * height );
				blockY = y + blockSize - verSize;
			}
			return new Texture( -1, x, blockY, blockSize, verSize, rec );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			X = newX;
			Y = newY;
			selectedBlock.X1 += deltaX;
			selectedBlock.Y1 += deltaY;
			
			for( int i = 0; i < barTextures.Length; i++ ) {
				Texture tex = barTextures[i];
				tex.X1 += deltaX;
				tex.Y1 += deltaY;
				barTextures[i] = tex;
			}
		}
	}
}