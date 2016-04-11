// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {

	/// <summary> Class responsible for performing drawing operations on bitmaps
	/// and for converting bitmaps into graphics api textures. </summary>
	/// <remarks> Uses GDI+ on Windows, uses Cairo on Mono. </remarks>
	unsafe partial class IDrawer2D {
		
		public Bitmap FontBitmap;
		
		/// <summary> Sets the bitmap that contains the bitmapped font characters as an atlas. </summary>
		public void SetFontBitmap( Bitmap bmp ) {
			FontBitmap = bmp;
			boxSize = FontBitmap.Width / 16;
			fontPixels = new FastBitmap( FontBitmap, true, true );
			CalculateTextWidths();
		}
		
		protected FastBitmap fontPixels;
		protected int boxSize;
		protected const int italicSize = 8;
		protected int[] widths = new int[256];
		void CalculateTextWidths() {
			for( int i = 0; i < 256; i++ )
				MakeTile( i, (i & 0x0F) * boxSize, (i >> 4) * boxSize );
			widths[(int)' '] = boxSize / 4;
		}
		
		void MakeTile( int i, int tileX, int tileY ) {
			// find first column (from right) where there is a solid pixel
			for( int x = boxSize - 1; x >= 0; x-- )
				for( int y = 0; y < boxSize; y++ )
			{
				int pixel = fontPixels.GetRowPtr( tileY + y )[tileX + x];
				byte a = (byte)(pixel >> 24);
				if( a >= 127 ) { // found a solid pixel
					widths[i] = x + 1;
					return;
				}
			}
			widths[i] = 0;
		}
		
		protected void DrawBitmapTextImpl( FastBitmap dst, ref DrawTextArgs args, int x, int y ) {
			bool underline = args.Font.Style == FontStyle.Underline;
			if( args.UseShadow ) {
				int offset = ShadowOffset( args.Font.Size );
				int shadowX = x + offset, shadowY = y + offset;
				
				DrawPart( dst, ref args, ref shadowX, shadowY, true );
				if( underline )
					DrawUnderline( dst, x + offset, 0, ref args, true );
			}

			int orignX = x;
			DrawPart( dst, ref args, ref x, y, false );
			if( underline )
				DrawUnderline( dst, orignX, -2, ref args, false );
		}
		
		void DrawPart( FastBitmap dst, ref DrawTextArgs args, ref int x, int y, bool shadowCol ) {
			FastColour textCol = FastColour.White;
			float point = args.Font.Size;
			int xMul = args.Font.Style == FontStyle.Italic ? 1 : 0;
			string text = args.Text;
			
			for( int i = 0; i < text.Length; i++ ) {
				char c = text[i];
				bool code = c == '&' && i < text.Length - 1;
				if( code && ValidColour( text[i + 1] ) ) {
					textCol = Colours[text[i + 1]];
					i++; continue; // Skip over the colour code.
				}
				if( shadowCol ) textCol = FastColour.Black;
				
				int coords = ConvertToCP437( c );
				int srcX = (coords & 0x0F) * boxSize;
				int srcY = (coords >> 4) * boxSize;
				
				int srcWidth = widths[coords], dstWidth = PtToPx( point, srcWidth );
				int srcHeight = boxSize, dstHeight = PtToPx( point, srcHeight );
				for( int yy = 0; yy < dstHeight; yy++ ) {
					int fontY = srcY + yy * srcHeight / dstHeight;
					int* fontRow = fontPixels.GetRowPtr( fontY );
					int dstY = y + yy;
					if( dstY >= dst.Height ) continue;
					
					int* dstRow = dst.GetRowPtr( dstY );
					int xOffset = xMul * ((dstHeight - 1 - yy) / italicSize);
					for( int xx = 0; xx < dstWidth; xx++ ) {
						int fontX = srcX + xx * srcWidth / dstWidth;
						int pixel = fontRow[fontX];
						if( (byte)(pixel >> 24) == 0 ) continue;
						int dstX = x + xx + xOffset;
						if( dstX >= dst.Width ) continue;
						
						int col = pixel & ~0xFFFFFF;
						col |= ((pixel & 0xFF) * textCol.B / 255);
						col |= (((pixel >> 8) & 0xFF) * textCol.G / 255) << 8;
						col |= (((pixel >> 16) & 0xFF) * textCol.R / 255) << 16;
						dstRow[dstX] = col;
					}
				}
				x += PtToPx( point, srcWidth + 1 );
			}
		}
		
		void DrawUnderline( FastBitmap dst, int x, int yOffset, ref DrawTextArgs args, bool shadowCol ) {
			int height = PtToPx( args.Font.Size, boxSize );
			int offset = ShadowOffset( args.Font.Size );
			int col = FastColour.White.ToArgb();
			float point = args.Font.Size;			
			string text = args.Text;
			if( args.UseShadow ) height += offset;
					
			for( int yy = height - offset; yy < height; yy++ ) {
				int* dstRow = dst.GetRowPtr( yy + yOffset );
				int startX = x;
				
				for( int i = 0; i < text.Length; i++ ) {
					char c = text[i];
					bool code = c == '&' && i < text.Length - 1;
					if( code && ValidColour( text[i + 1] ) ) {
						col = Colours[text[i + 1]].ToArgb();
						i++; continue; // Skip over the colour code.
					}
					if( shadowCol ) col = FastColour.Black.ToArgb();					
					
					int coords = ConvertToCP437( c );
					int width = PtToPx( point, widths[coords] + 1 );
					for( int xx = 0; xx < width; xx++ )
						dstRow[x + xx] = col;
					x += width;
				}
				x = startX;
				col = FastColour.White.ToArgb();
			}
		}
		
		protected Size MeasureBitmappedSizeImpl( ref DrawTextArgs args ) {
			if( String.IsNullOrEmpty( args.Text ) ) return Size.Empty;
			float point = args.Font.Size;
			Size total = new Size( 0, PtToPx( point, boxSize ) );
			
			for( int i = 0; i < args.Text.Length; i++ ) {
				char c = args.Text[i];
				bool code = c == '&' && i < args.Text.Length - 1;
				if( code && ValidColour( args.Text[i + 1] ) ) {
					i++; continue; // Skip over the colour code.
				}
				
				int coords = ConvertToCP437( c );
				total.Width += PtToPx( point, widths[coords] + 1 );
			}
			
			if( args.Font.Style == FontStyle.Italic )
				total.Width += Utils.CeilDiv( total.Height, italicSize );
			if( args.UseShadow ) {
				int offset = ShadowOffset( args.Font.Size );
				total.Width += offset; total.Height += offset;
			}
			return total;
		}
		
		protected static int ConvertToCP437( char c ) {
			if( c >= ' ' && c <= '~')
				return (int)c;
			
			int cIndex = Utils.ControlCharReplacements.IndexOf( c );
			if( cIndex >= 0 ) return cIndex;
			int eIndex = Utils.ExtendedCharReplacements.IndexOf( c );
			if( eIndex >= 0 ) return 127 + eIndex;
			return (int)'?';
		}
		
		protected static int ShadowOffset( float fontSize ) {
			if( fontSize < 9.9f ) return 1;
			if( fontSize < 24.9f ) return 2;
			return 3;
		}
		
		protected int PtToPx( int point ) {
			return (int)Math.Ceiling( (float)point / 72 * 96 ); // TODO: non 96 dpi?
		}
		
		protected int PtToPx( float point, float value ) {
			return (int)Math.Ceiling( (value / boxSize) * point / 72f * 96f );
		}
		
		protected void DisposeBitmappedText() {
			fontPixels.Dispose();
			FontBitmap.Dispose();
		}
	}
}
