using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Window;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class HotbarWidget : Widget {
		
		public HotbarWidget( Game window ) : base( window ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.BottomOrRight;
		}
		
		Texture selectedBlock, slotTexture;
		const int hotbarCount = 9;
		const int blockSize = 40;
		int iconsTexId;
		
		public override bool HandlesKeyDown( Key key ) {
			if( key >= Key.Number1 && key <= Key.Number9 ) {
				int index = (int)key - (int)Key.Number1;
				Window.Inventory.SetAndSendHeldSlotIndex( (short)index );
				return true;
			}
			return false;
		}
		
		public override void Init() {
			int y = Window.Height - blockSize;
			
			using( Bitmap bmp = new Bitmap( blockSize, blockSize ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					using( Pen pen = new Pen( Color.White, blockSize / 10 ) ) {
						pen.Alignment = PenAlignment.Inset;
						g.DrawRectangle( pen, 0, 0, blockSize, blockSize );
					}
				}
				selectedBlock = Utils2D.Make2DTexture( GraphicsApi, bmp, 0, y );
			}
			slotTexture = Utils2D.CreateTransparentSlot( GraphicsApi, 0, 0, blockSize );
			iconsTexId = GraphicsApi.LoadTexture( "icons.png" );
			
			int x = Window.Width / 2 - ( blockSize * hotbarCount ) / 2;
			X = x;
			Y = y;
			Width = blockSize * hotbarCount;
			Height = blockSize;
		}
		
		public override void Render( double delta ) {
			GraphicsApi.Texturing = true;
			RenderHotbarBackground();
			GraphicsApi.Texturing = true;
			GraphicsApi.Bind2DTexture( Window.TerrainAtlasTexId );
			RenderHotbarBlocks();
			GraphicsApi.Bind2DTexture( Window.ItemsAtlasTexId );
			RenderHotbarItems();
			RenderHearts();
			GraphicsApi.Texturing = false;
		}
		
		void RenderHearts() {
			GraphicsApi.Bind2DTexture( iconsTexId );
			int health = Window.LocalPlayer.Health;
			
			for( int heart = 0; heart < 10; heart++ ) {
				TextureRectangle rec = new TextureRectangle( 16 / 256f, 0 / 256f, 9 / 256f, 9 / 256f );
				Texture tex = new Texture( iconsTexId, X + 16 * heart, Y - 18, 18, 18, rec );
				tex.RenderNoBind( GraphicsApi );
				
				if( health >= 2 ) {
					rec = new TextureRectangle( 53 / 256f, 1 / 256f, 7 / 256f, 7 / 256f );
				} else if( health == 1 ) {
					rec = new TextureRectangle( 62 / 256f, 1 / 256f, 7 / 256f, 7 / 256f );
				} else {
					continue;
				}
				tex = new Texture( iconsTexId, X + 16 * heart + 2, Y - 18 + 2, 14, 14, rec );
				tex.RenderNoBind( GraphicsApi );
				health -= 2;
			}
		}
		
		void RenderHotbarBackground() {
			GraphicsApi.Bind2DTexture( slotTexture.ID );
			
			for( int x = 0; x < hotbarCount; x++ ) {
				slotTexture.X1 = X + x * blockSize;
				slotTexture.Y1 = Y;
				slotTexture.RenderNoBind( GraphicsApi );
				if( x == Window.Inventory.HeldSlotIndex ) {
					selectedBlock.X1 = slotTexture.X1;
				}
			}
			selectedBlock.Render( GraphicsApi );
		}
		
		void RenderHotbarBlocks() {
			for( int i = 0; i < hotbarCount; i++ ) {
				Slot slot = Window.Inventory.GetHotbarSlot( i );
				if( slot.IsEmpty || slot.Id > 255 ) continue;
				
				int texId = Window.BlockInfo.GetOptimTextureLoc( (byte)slot.Id, TileSide.Front );
				if( texId == 0 ) continue;
				
				int x = X + i * blockSize;
				TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texId );
				float x1 = x + 4, y1 = Y + 4, x2 = x1 + 32, y2 = y1 + 32;
				float height = Window.BlockInfo.BlockHeight( (byte)slot.Id );
				if( height != 1 ) {
					rec.V1 = rec.V1 + Window.TerrainAtlas.invVerElementSize * height;
					y2 = y1 + (int)( 32 * height );
				}
				
				VertexPos3fTex2f[] vertices = {
					new VertexPos3fTex2f( x2, y1, 0, rec.U2, rec.V1 ),
					new VertexPos3fTex2f( x2, y2, 0, rec.U2, rec.V2 ),
					new VertexPos3fTex2f( x1, y1, 0, rec.U1, rec.V1 ),
					new VertexPos3fTex2f( x1, y2, 0, rec.U1, rec.V2 ),
				};
				GraphicsApi.DrawVertices( DrawMode.TriangleStrip, vertices );
			}
		}
		
		void RenderHotbarItems() {
			for( int i = 0; i < hotbarCount; i++ ) {
				Slot slot = Window.Inventory.GetHotbarSlot( i );
				if( slot.IsEmpty || slot.Id <= 255 ) continue;
				
				int texId = Window.ItemInfo.Get2DTextureLoc( slot.Id );
				if( texId == 0 ) continue;
				
				int x = X + i * blockSize;
				TextureRectangle rec = Window.TerrainAtlas.GetTexRec( texId );
				float x1 = x + 4, y1 = Y + 4, x2 = x1 + 32, y2 = y1 + 32;
				
				VertexPos3fTex2f[] vertices = {
					new VertexPos3fTex2f( x2, y1, 0, rec.U2, rec.V1 ),
					new VertexPos3fTex2f( x2, y2, 0, rec.U2, rec.V2 ),
					new VertexPos3fTex2f( x1, y1, 0, rec.U1, rec.V1 ),
					new VertexPos3fTex2f( x1, y2, 0, rec.U1, rec.V2 ),
				};
				GraphicsApi.DrawVertices( DrawMode.TriangleStrip, vertices );
			}
		}
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref selectedBlock );
			GraphicsApi.DeleteTexture( ref slotTexture );
			GraphicsApi.DeleteTexture( iconsTexId );
		}
		
		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			X = newX;
			Y = newY;
			selectedBlock.X1 += deltaX;
			selectedBlock.Y1 += deltaY;
		}
	}
}