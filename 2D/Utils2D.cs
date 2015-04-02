using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Text;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public static class Utils2D {
		
		public static StringFormat format;
		static Utils2D() {
			format = StringFormat.GenericTypographic;
			format.FormatFlags |= StringFormatFlags.MeasureTrailingSpaces;
			format.Trimming = StringTrimming.None;
			//format.FormatFlags |= StringFormatFlags.NoWrap;
			//format.FormatFlags |= StringFormatFlags.NoClip;
		}
		
		const float shadowOffset = 1.3f;
		public static Size MeasureSize( string text, string fontName, float fontSize, bool shadow ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				return MeasureSize( text, font, shadow );
			}
		}
		
		public static Size MeasureSize( string text, string fontName, float fontSize, FontStyle style, bool shadow ) {
			using( Font font = new Font( fontName, fontSize, style ) ) {
				return MeasureSize( text, font, shadow );
			}
		}
		
		public static Size MeasureSize( string text, Font font, bool shadow ) {
			using( Bitmap measuringBmp = new Bitmap( 1, 1 ) ) {
				using( Graphics g = Graphics.FromImage( measuringBmp ) ) {
					SizeF size = g.MeasureString( text, font, Int32.MaxValue, format );
					if( shadow ) {
						size.Width += shadowOffset;
						size.Height += shadowOffset;
					}
					return Size.Ceiling( size );
				}
			}
		}
		
		public static Size MeasureSize( List<DrawTextArgs> parts, string fontName, float fontSize, bool shadow ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				return MeasureSize( parts, font, shadow );
			}
		}
		
		public static Size MeasureSize( List<DrawTextArgs> parts, Font font, bool shadow ) {
			SizeF total = SizeF.Empty;
			using( Bitmap measuringBmp = new Bitmap( 1, 1 ) ) {
				using( Graphics g = Graphics.FromImage( measuringBmp ) ) {
					for( int i = 0; i < parts.Count; i++ ) {
						SizeF size = g.MeasureString( parts[i].Text, font, Int32.MaxValue, format );
						total.Height = Math.Max( total.Height, size.Height );
						total.Width += size.Width;
					}
				}
			}
			if( shadow ) {
				total.Width += shadowOffset;
				total.Height += shadowOffset;
			}
			return Size.Ceiling( total );
		}
		
		
		public static void DrawText( Graphics g, string fontName, float fontSize, DrawTextArgs args ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				DrawText( g, font, args );
			}
		}
		
		public static void DrawText( Graphics g, string fontName, float fontSize, FontStyle style, DrawTextArgs args ) {
			using( Font font = new Font( fontName, fontSize, style ) ) {
				DrawText( g, font, args );
			}
		}
		
		public static void DrawText( Graphics g, Font font, DrawTextArgs args ) {
			float x = args.XOffset, y = args.YOffset;
			using( Brush textBrush = new SolidBrush( args.TextColour ),
			      shadowBrush = new SolidBrush( args.ShadowColour ) ) {
				g.TextRenderingHint = TextRenderingHint.AntiAlias;
				if( args.UseShadow ) {
					g.DrawString( args.Text, font, shadowBrush, x + shadowOffset, y + shadowOffset, format );
				}
				g.DrawString( args.Text, font, textBrush, x, y, format );
			}
		}
		
		public static void DrawText( Graphics g, List<DrawTextArgs> parts, string fontName, float fontSize ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				DrawText( g, parts, font );
			}
		}
		
		public static void DrawText( Graphics g, List<DrawTextArgs> parts, string fontName, float fontSize, float x, float y ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				DrawText( g, parts, font, x, y );
			}
		}
		
		public static void DrawText( Graphics g, List<DrawTextArgs> parts, Font font ) {
			DrawText( g, parts, font, 0, 0 );
		}
		
		public static void DrawText( Graphics g, List<DrawTextArgs> parts, Font font, float x, float y ) {
			for( int i = 0; i < parts.Count; i++ ) {
				DrawTextArgs part = parts[i];
				part.XOffset = x;
				part.YOffset = y;
				DrawText( g, font, part );
				SizeF partSize = g.MeasureString( part.Text, font, Int32.MaxValue, format );
				x += partSize.Width;
			}
		}
		
		public static void DrawRect( Graphics g, Color colour, int x, int y, int width, int height ) {
			using( Brush brush = new SolidBrush( colour ) ) {
				g.FillRectangle( brush, x, y, width, height );
			}
		}
		
		public static void DrawRect( Graphics g, Color colour, int width, int height ) {
			DrawRect( g, colour, 0, 0, width, height );
		}
		
		public static void DrawRectBounds( Graphics g, Color colour, float lineWidth, int x, int y, int width, int height ) {
			using( Pen pen = new Pen( colour, lineWidth ) ) {
				g.DrawRectangle( pen, x, y, width, height );
			}
		}
		
		public static void DrawRectBounds( Graphics g, Color colour, float lineWidth, int width, int height ) {
			DrawRectBounds( g, colour, lineWidth, 0, 0, width, height );
		}

		
		public static Texture MakeTextTexture( string fontName, float fontSize, int x1, int y1, DrawTextArgs args ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				return MakeTextTexture( font, x1, y1, args );
			}
		}
		
		public static Texture MakeTextTexture( string fontName, float fontSize, FontStyle style, int x1, int y1, DrawTextArgs args ) {
			using( Font font = new Font( fontName, fontSize, style ) ) {
				return MakeTextTexture( font, x1, y1, args );
			}
		}
		
		public static Texture MakeTextTexture( Font font, int x1, int y1, DrawTextArgs args ) {
			Size size = MeasureSize( args.Text, font, args.UseShadow );
			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					DrawText( g, font, args );
				}
				return Make2DTexture( args.Graphics, bmp, x1, y1 );
			}
		}
		
		public static Texture MakeTextTexture( List<DrawTextArgs> parts, string fontName, float fontSize, Size size, int x1, int y1 ) {
			using( Font font = new Font( fontName, fontSize ) ) {
				return MakeTextTexture( parts, font, size, x1, y1 );
			}
		}
		
		public static Texture MakeTextTexture( List<DrawTextArgs> parts, string fontName, float fontSize, FontStyle style, Size size, int x1, int y1 ) {
			using( Font font = new Font( fontName, fontSize, style ) ) {
				return MakeTextTexture( parts, font, size, x1, y1 );
			}
		}
		
		public static Texture MakeTextTexture( List<DrawTextArgs> parts, Font font, Size size, int x1, int y1 ) {
			if( parts.Count == 0 ) return new Texture( -1, x1, y1, 0, 0, 1, 1 );
			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					DrawText( g, parts, font );
				}
				return Make2DTexture( parts[0].Graphics, bmp, x1, y1 );
			}
		}
		
		public static Texture Make2DTexture( IGraphicsApi graphics, Bitmap bmp, int x1, int y1 ) {
			if( graphics.SupportsNonPowerOf2Textures ) {
				int textureID = graphics.LoadTexture( bmp );
				return new Texture( textureID, x1, y1, bmp.Width, bmp.Height, 1f, 1f );
			} else {
				using( Bitmap adjBmp = ResizeToPower2( bmp ) )  {
					int textureID = graphics.LoadTexture( adjBmp );
					return new Texture( textureID, x1, y1, bmp.Width, bmp.Height,
					                   (float)bmp.Width / adjBmp.Width, (float)bmp.Height / adjBmp.Height );
				}
			}
		}
		
		public static bool Contains( int recX, int recY, int width, int height, int x, int y ) {
			return x >= recX && y >= recY && x < recX + width && y < recY + height;
		}
		
		public static Bitmap ResizeToPower2( Bitmap bmp ) {
			int adjWidth = Utils.NextPowerOf2( bmp.Width );
			int adjHeight = Utils.NextPowerOf2( bmp.Height );
			Bitmap adjBmp = new Bitmap( adjWidth, adjHeight );
			using( Graphics g = Graphics.FromImage( adjBmp ) ) {
				g.DrawImage( bmp, 0, 0 );
			}
			return adjBmp;
		}
	}
	
	public struct Texture {
		public int ID;
		public int X1, Y1;
		public int Width, Height;
		public float U1, V1;
		// Normally just used for graphics cards that don't support non power of two textures.
		public float U2, V2;

		public Texture( int id, int x, int y, int width, int height, float u2, float v2 )
			: this( id, x, y, width, height, 0, u2, 0, v2 )	{
		}
		
		public Texture( int id, int x, int y, int width, int height, TextureRectangle rec )
			: this( id, x, y, width, height, rec.U1, rec.U2, rec.V1, rec.V2 )	{
		}
		
		public Texture( int id, int x, int y, int width, int height, float u1, float u2, float v1, float v2 ) {
			ID = id;
			X1 = x;
			Y1 = y;
			Width = width;
			Height = height;
			U1 = u1;
			V1 = v1;
			U2 = u2;
			V2 = v2;
		}
		
		public bool IsValid {
			get { return ID > 0; }
		}
		
		public void Render( IGraphicsApi graphics ) {
			graphics.Texturing = true;
			graphics.Bind2DTexture( ID );
			RenderNoBind( graphics );
			graphics.Texturing = false;
		}
		
		public void RenderNoBind( IGraphicsApi graphics ) {
			graphics.Draw2DTextureVertices( ref this );
		}
		
		public int X2 {
			get { return X1 + Width; }
			//set { X1 = value - Width; }
		}
		
		public int Y2 {
			get { return Y1 + Height; }
			//set { Y1 = value - Height; }
		}
		
		public override string ToString() {
			return ID + String.Format( "({0}, {1} -> {2},{3}", X1, Y1, Width, Height );
		}
	}
}
