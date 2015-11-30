using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class BlockHotbarWidget : Widget {
		
		public BlockHotbarWidget( Game game ) : base( game ) {
			HorizontalAnchor = Anchor.Centre;
			VerticalAnchor = Anchor.BottomOrRight;
			hotbarCount = game.Inventory.Hotbar.Length;
		}
		
		int hotbarCount;
		Texture selectedBlock, background;
		int barHeight, selBlockSize, elemSize;
		int barXOffset, borderSize;
		
		public override bool HandlesKeyDown( Key key ) {
			if( key >= Key.Number1 && key <= Key.Number9 ) {
				game.Inventory.HeldBlockIndex = (int)key - (int)Key.Number1;
				return true;
			}
			return false;
		}
		
		public override void Init() {
			float scale = 2 * game.GuiScale;
			selBlockSize = (int)(24 * scale);
			barHeight = (int)(22 * scale);		
			Width = (int)(182 * scale);
			Height = barHeight;
			
			elemSize = (int)(16 * scale);
			barXOffset = (int)(3 * scale);
			borderSize = (int)(4 * scale);
			X = game.Width / 2 - Width / 2;
			Y = game.Height - barHeight;		
			
			MakeBackgroundTexture();
			MakeSelectionTexture();
		}
		
		public override void Render( double delta ) {
			graphicsApi.Texturing = true;
			background.Render( graphicsApi );
			graphicsApi.BindTexture( game.TerrainAtlas.TexId );
			graphicsApi.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			
			for( int i = 0; i < hotbarCount; i++ ) {
				byte block = (byte)game.Inventory.Hotbar[i];
				int x = X + barXOffset + (elemSize + borderSize) * i + elemSize / 2;
				int y = game.Height - barHeight / 2;
				float scale = (elemSize - 4) / 2f;
				IsometricBlockDrawer.Draw( game, block, scale, x, y );
				if( i == game.Inventory.HeldBlockIndex )
					selectedBlock.X1 = x - selBlockSize / 2;
			}
			
			selectedBlock.Render( graphicsApi );
			graphicsApi.Texturing = false;
		}
		
		public override void Dispose() { }
		
		public override void MoveTo( int newX, int newY ) {
			int diffX = newX - X, diffY = newY - Y;
			X = newX; Y = newY;
			Dispose();
			Init();
		}
		
		void MakeBackgroundTexture() {
			TextureRec rec = new TextureRec( 0, 0, 182/256f, 22/256f );
			background = new Texture( game.GuiTexId, X, Y, Width, Height, rec );
		}
		
		void MakeSelectionTexture() {
			int y = game.Height - selBlockSize;
			TextureRec rec = new TextureRec( 0, 22/256f, 24/256f, 24/256f );
			selectedBlock = new Texture( game.GuiTexId, 0, y, 
			                            selBlockSize, selBlockSize, rec );
		}
	}
}