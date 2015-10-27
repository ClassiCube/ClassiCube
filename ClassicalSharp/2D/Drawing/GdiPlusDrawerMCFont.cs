using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public sealed class GdiPlusDrawerMCFont : GdiPlusDrawer2D {
		
		// NOTE: This drawer is still a big work in progress and not close to done
		// TODO: proper shadow colour
		// TODO: italic and bold
		Bitmap fontBmp;
		FastBitmap fontPixels;
		int boxSize;
		public GdiPlusDrawerMCFont( IGraphicsApi graphics ) : base( graphics ) {
			fontBmp = new Bitmap( "default.png" );
			boxSize = fontBmp.Width / 16;
			fontPixels = new FastBitmap( fontBmp, true );
			CalculateTextWidths();
		}
		
		int[] widths = new int[256];
		void CalculateTextWidths() {
			for( int i = 0; i < 256; i++ )
				MakeTile( i, (i & 0x0F) * boxSize, (i >> 4) * boxSize );		
			widths[(int)' '] = boxSize / 2;
		}
		
		unsafe void MakeTile( int i, int tileX, int tileY ) {
			for( int x = boxSize - 1; x >= 0; x-- ) {
				for( int y = 0; y < boxSize; y++ ) {
					uint pixel = (uint)fontPixels.GetRowPtr( tileY + y )[tileX + x];
					uint a = pixel >> 24;
					if( a >= 127 ) {
						widths[i] = x + 1;
						return;
					}
				}
			}
			widths[i] = 0;
		}
		
		public override void DrawText( ref DrawTextArgs args, int x, int y ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			if( args.UseShadow ) {
				int shadowX = x + 2, shadowY = y + 2;
				for( int i = 0; i < parts.Count; i++ ) {
					TextPart part = parts[i];
					part.TextColour = FastColour.Black;
					DrawPart( ref shadowX, shadowY, args.Font.Size, part );
				}
			}
			
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				DrawPart( ref x, y, args.Font.Size, part );
			}
		}
		
		unsafe void DrawPart( ref int x, int y, float point, TextPart part ) {
			string text = part.Text;
			foreach( char c in text ) {
				int coords = ConvertToCP437( c );
				int srcX = (coords & 0x0F) * boxSize;
				int srcY = (coords >> 4) * boxSize;
				
				int srcWidth = widths[coords], dstWidth = PtToPx( point, srcWidth );
				int srcHeight = boxSize, dstHeight = PtToPx( point, srcHeight );
				for( int yy = 0; yy < dstHeight; yy++ ) {
					int fontY = srcY + yy * srcHeight / dstHeight;
					int* fontRow = fontPixels.GetRowPtr( fontY );
					
					for( int xx = 0; xx < dstWidth; xx++ ) {
						int fontX = srcX + xx * srcWidth / dstWidth;					
						FastColour col = new FastColour( fontRow[fontX] );
						if( col.A < 127 ) continue;
						
						col.R = (byte)(col.R * part.TextColour.R / 255);
						col.G = (byte)(col.G * part.TextColour.G / 255);
						col.B = (byte)(col.B * part.TextColour.B / 255);					
						curBmp.SetPixel( x + xx, y + yy, col );
					}
				}
				x += PtToPx( point, srcWidth + 1 );
			}
		}
		
		public override void DrawClippedText( ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight ) {
			throw new NotImplementedException();
		}
		
		public override Size MeasureSize( ref DrawTextArgs args ) {
			GetTextParts( args.Text );
			float point = args.Font.Size;
			Size total = new Size( 0, PtToPx( point, boxSize ) );
			
			for( int i = 0; i < parts.Count; i++ ) {
				foreach( char c in parts[i].Text ) {
					int coords = ConvertToCP437( c );
					total.Width += PtToPx( point, widths[coords] + 1 );
				}
			}
			
			if( args.UseShadow && parts.Count > 0 ) {
				total.Width += 2; total.Height += 2;
			}
			return total;
		}
		
		int ConvertToCP437( char c ) {
			if( c >= ' ' && c <= '~')
				return (int)c;
			
			int cIndex = Utils.ControlCharReplacements.IndexOf( c );
			if( cIndex >= 0 ) return cIndex;
			int eIndex = Utils.ExtendedCharReplacements.IndexOf( c );
			if( eIndex >= 0 ) return 127 + eIndex;		
			return (int)'?';
		}
		
		int PtToPx( int point ) {
			return (int)Math.Ceiling( (float)point / 72 * 96 ); // TODO: non 96 dpi?
		}
		
		int PtToPx( float point, float value ) {
			return (int)Math.Ceiling( (value / boxSize) * point / 72f * 96f );
		}
		
		public override void DisposeInstance() {
			base.DisposeInstance();
			fontPixels.Dispose();
			fontBmp.Dispose();
		}
	}
}
