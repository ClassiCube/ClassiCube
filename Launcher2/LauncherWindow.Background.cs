using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed partial class LauncherWindow {
		
		internal FastColour clearColour = new FastColour( 30, 30, 30 );
		public void MakeBackground() {
			if( Framebuffer != null )
				Framebuffer.Dispose();
			
			Framebuffer = new Bitmap( Width, Height );
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( Framebuffer );
				//ClearDirtTexture( drawer );
				ClearColour( drawer );
				
				DrawTextArgs args1 = new DrawTextArgs( "&eClassical", logoItalicFont, true );
				Size size1 = drawer.MeasureSize( ref args1 );
				DrawTextArgs args2 = new DrawTextArgs( "&eSharp", logoFont, true );
				Size size2 = drawer.MeasureSize( ref args2 );
				
				int xStart = Width / 2 - (size1.Width + size2.Width ) / 2;
				drawer.DrawText( ref args1, xStart, 20 );
				drawer.DrawText( ref args2, xStart + size1.Width, 20 );
			}
			Dirty = true;
		}
		
		void ClearColour( IDrawer2D drawer ) {
			drawer.Clear( clearColour );
		}
		
		void ClearDirtTexture( IDrawer2D drawer ) {
			Bitmap bmp = new Bitmap( "dirt.png" );
			drawer.ConvertTo32Bpp( ref bmp );
			
			using( FastBitmap dst = new FastBitmap( Framebuffer, true ),
			      src = new FastBitmap( bmp, true ) ) {
				Rectangle srcRect = new Rectangle( 0, 0, 16, 16 );
				const int tileSize = 64;
				for( int y = 0; y < Height; y += tileSize ) {
					for( int x = 0; x < Width; x += tileSize ) {
						int x2 = Math.Min( x + tileSize, Width );
						int y2 = Math.Min( y + tileSize, Height );
						
						Size size = new Size( tileSize, tileSize );
						Rectangle dstRect = new Rectangle( x, y, x2 - x, y2 - y );
						FastBitmap.CopyScaledPixels( src, dst, size, srcRect, dstRect, 128 );
					}
				}
			}
		}
	}
}
