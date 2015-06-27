using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class BlockHotbarWidget : Widget {
		
		public BlockHotbarWidget( Game window ) : base( window ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.BottomOrRight;
			window.HeldBlockChanged += HeldBlockChanged;
		}
		
		Texture2D[] barTextures = new Texture2D[9];
		Texture2D selectedBlock;
		Inventory inventory;
		const int blockSize = 32;
		
		public override bool HandlesKeyDown( Key key ) {
			if( key >= Key.Number1 && key <= Key.Number9 ) {
				inventory.HeldBlockIndex = (int)key - (int)Key.Number1;
				return true;
			}
			return false;
		}
		
		public override void Init() {
			int y = Window.Height - blockSize;
			inventory = Window.Inventory;
			
			Size size = new Size( 32, 32 );
			using( Bitmap bmp = Utils2D.CreatePow2Bitmap( size ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRectBounds( g, Color.White, blockSize / 8, 0, 0, blockSize, blockSize );
				}
				selectedBlock = Utils2D.Make2DTexture( GraphicsApi, bmp, size, 0, y );
			}
			
			int x = Window.Width / 2 - ( blockSize * barTextures.Length ) / 2;
			X = x;
			Y = y;
			Width = blockSize * barTextures.Length;
			Height = blockSize;
			
			for( int i = 0; i < barTextures.Length; i++ ) {
				barTextures[i] = MakeTexture( x, y, inventory.BlocksHotbar[i] );
				x += blockSize;
			}
		}
		
		public override void Render( double delta ) {
			// TODO: Maybe redesign this so we don't have to bind the whole atlas. Not cheap.
			Window.TerrainAtlas.TexId.Bind();
			int selectedX = 0;
			for( int i = 0; i < barTextures.Length; i++ ) {
				barTextures[i].RenderNoBind( GraphicsApi );
				if( i == inventory.HeldBlockIndex ) {
					selectedX = barTextures[i].X1;
				}
			}
			selectedBlock.X1 = selectedX;
			selectedBlock.Render( GraphicsApi );
		}
		
		public override void Dispose() {
			selectedBlock.Delete();
			Window.HeldBlockChanged -= HeldBlockChanged;
		}
		
		void HeldBlockChanged( object sender, EventArgs e ) {
			int index = inventory.HeldBlockIndex;
			Block block = inventory.HeldBlock;
			int x = barTextures[index].X1;
			barTextures[index] = MakeTexture( x, Y, block );
		}
		
		Texture2D MakeTexture( int x, int y, Block block ) {
			int texId = Window.BlockInfo.GetOptimTextureLoc( (byte)block, TileSide.Left );
			TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texId );
			
			int verSize = blockSize;
			float height = Window.BlockInfo.BlockHeight( (byte)block );
			int blockY = y;
			if( height != 1 ) {
				rec.V1 = rec.V1 + TerrainAtlas2D.usedInvVerElemSize * height;
				verSize = (int)( blockSize * height );
				blockY = y + blockSize - verSize;
			}
			return new Texture2D( TextureObj.Empty, x, blockY, blockSize, verSize, rec );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			X = newX;
			Y = newY;
			selectedBlock.X1 += deltaX;
			selectedBlock.Y1 += deltaY;
			
			for( int i = 0; i < barTextures.Length; i++ ) {
				Texture2D tex = barTextures[i];
				tex.X1 += deltaX;
				tex.Y1 += deltaY;
				barTextures[i] = tex;
			}
		}
	}
}