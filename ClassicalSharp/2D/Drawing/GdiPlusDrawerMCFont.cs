using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public sealed class GdiPlusDrawerMCFont : GdiPlusDrawer2D {
		
		// NOTE: This drawer is still a big work in progress and not close to done
		// TODO: proper shadow colour
		// TODO: italic and bold
		Bitmap realBmp;
		public GdiPlusDrawerMCFont( IGraphicsApi graphics ) : base( graphics ) {
			realBmp = new Bitmap( "default.png" );
			CalculateTextWidths();
		}
		
		int[] widths = new int[256];
		unsafe void CalculateTextWidths() {
			using( FastBitmap fastBmp = new FastBitmap( realBmp, true ) ) {
				for( int i = 0; i < 256; i++ ) {
					int tileX = (i & 0x0F) * 8;
					int tileY = (i >> 4) * 8;
					MakeTile( fastBmp, i, tileX, tileY );
				}
			}
			widths[(int)' '] = 4;
		}
		
		unsafe void MakeTile( FastBitmap fastBmp, int i, int tileX, int tileY ) {
			for( int x = 7; x >= 0; x-- ) {
				for( int y = 0; y < 8; y++ ) {
					uint pixel = (uint)fastBmp.GetRowPtr( tileY + y )[tileX + x];
					uint a = pixel >> 24;
					if( a >= 127 ) {
						widths[i] = x + 1;
						Console.WriteLine( widths[i] );
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
				int shadowX = x + 1, shadowY = y + 1;
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
		
		void DrawPart( ref int x, int y, float point, TextPart part ) {
			string text = part.Text;
			foreach( char c in text ) {
				int coords = (int)c;
				int srcX = (coords & 0x0F) * 8;
				int srcY = (coords >> 4) * 8;
				
				for( int yy = 0; yy < 8; yy++ ) {
					for( int xx = 0; xx < widths[coords]; xx++ ) {
						FastColour col = realBmp.GetPixel( srcX + xx, srcY + yy );
						if( col.A < 127 ) continue;
						col.R = (byte)(col.R * part.TextColour.R / 255);
						col.G = (byte)(col.G * part.TextColour.G / 255);
						col.B = (byte)(col.B * part.TextColour.B / 255);
						
						curBmp.SetPixel( x + xx, y + yy, col );
					}
				}
				x += widths[coords] + 1;
				/*int rawWidth = widths[coords] + 1;
				int rawHeight = 8;
				g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
				g.DrawImage( realBmp,
				            new Rectangle( x, y, PtToPx( point, rawWidth ), PtToPx( point, rawHeight ) ),
				            new Rectangle( srcX, srcY, 8, 8 ),
				            GraphicsUnit.Pixel );
				
				for( int yy = 0; yy < PtToPx( point, rawHeight ); yy++ ) {
					for( int xx = 0; xx < PtToPx( point, rawWidth ); xx++ ) {
						FastColour col = curBmp.GetPixel( x + xx, y + yy );
						if( col.A < 127 ) continue;
						col.R = (byte)(col.R * part.TextColour.R / 255);
						col.G = (byte)(col.G * part.TextColour.G / 255);
						col.B = (byte)(col.B * part.TextColour.B / 255);
						
						curBmp.SetPixel( x + xx, y + yy, col );
					}
				}
				x += PtToPx( point, rawWidth );*/
			}
		}
		
		public override void DrawClippedText( ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight ) {
			throw new NotImplementedException();
		}
		
		public override Size MeasureSize( ref DrawTextArgs args ) {
			GetTextParts( args.Text );
			float point = args.Font.Size;
			//Size total = new Size( 0, PtToPx( point, 8 ) );
			Size total = new Size( 0, 8 );
			
			for( int i = 0; i < parts.Count; i++ ) {
				foreach( char c in parts[i].Text ) {
					total.Width += widths[(int)c] + 1;//PtToPx( point, widths[(int)c] + 1 );
				}
			}
			
			if( args.UseShadow && parts.Count > 0 ) {
				total.Width += 1; total.Height += 1;
			}
			return total;
		}
		
		int PtToPx( int point ) {
			return (int)Math.Ceiling( (float)point / 72 * 96 ); // TODO: non 96 dpi?
		}
		
		int PtToPx( float point, float value ) {
			return (int)Math.Ceiling( (value / 8f) * point / 72f * 96f );
		}
		
		public override void DisposeInstance() {
			base.DisposeInstance();
		}
	}
}
