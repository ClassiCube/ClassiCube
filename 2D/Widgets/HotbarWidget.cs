using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class HotbarWidget : Widget {
		
		public HotbarWidget( Game window ) : base( window ) {
			HorizontalDocking = Docking.Centre;
			VerticalDocking = Docking.BottomOrRight;
		}
		
		Texture selectedBlock, slotTexture;
		const int hotbarCount = 9;
		SlotWidget[] hotbarSlots = new SlotWidget[hotbarCount];
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
			for( int i = 0; i < hotbarCount; i++ ) {
				int index = i; // must capture the variable.
				SlotWidget widget = new SlotWidget( Window, slotTexture, 
				                                   () => Window.Inventory.GetHotbarSlot( index ) );
				widget.Init();
				widget.MoveTo( x + i * blockSize, y );
				hotbarSlots[i] = widget;
			}
			X = x;
			Y = y;
			Width = blockSize * hotbarCount;
			Height = blockSize;
		}
		
		public override void Render( double delta ) {
			GraphicsApi.Texturing = true;
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].RenderBackground();
			}
			selectedBlock.X1 = X + blockSize * Window.Inventory.HeldSlotIndex;
			selectedBlock.Y1 = Y;
			GraphicsApi.Bind2DTexture( selectedBlock.ID );
			selectedBlock.RenderNoBind( GraphicsApi );
			
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].RenderItem();
			}
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
		
		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref selectedBlock );
			GraphicsApi.DeleteTexture( ref slotTexture );
			GraphicsApi.DeleteTexture( iconsTexId );
		}
		
		public override void MoveTo( int newX, int newY ) {
			X = newX;
			Y = newY;
			for( int i = 0; i < hotbarCount; i++ ) {
				hotbarSlots[i].MoveTo( newX + i * blockSize, newY );
			}
		}
	}
}