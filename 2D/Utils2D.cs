using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Text;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public static class Utils2D {
		
		public static StringFormat format;
		static Bitmap measuringBmp;
		static Graphics measuringGraphics;
		static Dictionary<int, SolidBrush> brushCache = new Dictionary<int, SolidBrush>( 16 );
		internal static bool needWinXpFix;
		internal static char[] trimChars = { '\0' };
		
		static Utils2D() {
			format = StringFormat.GenericTypographic;
			format.FormatFlags |= StringFormatFlags.MeasureTrailingSpaces;
			format.Trimming = StringTrimming.None;
			//format.FormatFlags |= StringFormatFlags.NoWrap;
			//format.FormatFlags |= StringFormatFlags.NoClip;
			measuringBmp = new Bitmap( 1, 1 );
			measuringGraphics = Graphics.FromImage( measuringBmp );
			measuringGraphics.TextRenderingHint = TextRenderingHint.AntiAlias;
			OperatingSystem os = Environment.OSVersion;
			needWinXpFix = os.Platform == PlatformID.Win32NT && os.Version.Major < 6;
		}
		
		static SolidBrush GetOrCreateBrush( Color color ) {
			int key = color.ToArgb();
			SolidBrush brush;
			if( brushCache.TryGetValue( key, out brush ) ) {
				return brush;
			}
			brush = new SolidBrush( color );
			brushCache[key] = brush;
			return brush;
		}
		
		const float shadowOffset = 1.3f;
		public static Size MeasureSize( string text, Font font, bool shadow ) {
			SizeF size = measuringGraphics.MeasureString( text, font, Int32.MaxValue, format );
			if( shadow ) {
				size.Width += shadowOffset;
				size.Height += shadowOffset;
			}
			return Size.Ceiling( size );
		}
		
		public static Size MeasureSize( List<DrawTextArgs> parts, Font font, bool shadow ) {
			SizeF total = SizeF.Empty;
			for( int i = 0; i < parts.Count; i++ ) {
				SizeF size = measuringGraphics.MeasureString( parts[i].Text, font, Int32.MaxValue, format );
				total.Height = Math.Max( total.Height, size.Height );
				total.Width += size.Width;
			}
			if( shadow ) {
				total.Width += shadowOffset;
				total.Height += shadowOffset;
			}
			return Size.Ceiling( total );
		}
		
		public static void DrawText( Graphics g, Font font, ref DrawTextArgs args, float x, float y ) {
			Brush textBrush = GetOrCreateBrush( args.TextColour );
			Brush shadowBrush = GetOrCreateBrush( args.ShadowColour );
			g.TextRenderingHint = TextRenderingHint.AntiAlias;
			
			if( args.UseShadow ) {
				g.DrawString( args.Text, font, shadowBrush, x + shadowOffset, y + shadowOffset, format );
			}
			g.DrawString( args.Text, font, textBrush, x, y, format );
		}
		
		public static void DrawText( Graphics g, List<DrawTextArgs> parts, Font font, float x, float y ) {
			for( int i = 0; i < parts.Count; i++ ) {
				DrawTextArgs part = parts[i];
				DrawText( g, font, ref part, x, y );
				SizeF partSize = g.MeasureString( part.Text, font, Int32.MaxValue, format );
				x += partSize.Width;
			}
		}
		
		public static void DrawRect( Graphics g, Color colour, int x, int y, int width, int height ) {
			Brush brush = GetOrCreateBrush( colour );
			g.FillRectangle( brush, x, y, width, height );
		}
		
		public static void DrawRectBounds( Graphics g, Color colour, float lineWidth, int x, int y, int width, int height ) {
			using( Pen pen = new Pen( colour, lineWidth ) ) {
				g.DrawRectangle( pen, x, y, width, height );
			}
		}
		
		public static Texture MakeTextTexture( Font font, int x1, int y1, ref DrawTextArgs args ) {
			Size size = MeasureSize( args.Text, font, args.UseShadow );
			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					DrawText( g, font, ref args, 0, 0 );
				}
				return Make2DTexture( args.Graphics, bmp, x1, y1 );
			}
		}
		
		public static Texture MakeTextTexture( List<DrawTextArgs> parts, Font font, Size size, int x1, int y1 ) {
			if( parts.Count == 0 ) return new Texture( -1, x1, y1, 0, 0, 1, 1 );
			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					DrawText( g, parts, font, 0, 0 );
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
		
		public static Bitmap ResizeToPower2( Bitmap bmp ) {
			int adjWidth = Utils.NextPowerOf2( bmp.Width );
			int adjHeight = Utils.NextPowerOf2( bmp.Height );
			Bitmap adjBmp = new Bitmap( adjWidth, adjHeight );
			using( Graphics g = Graphics.FromImage( adjBmp ) ) {
				g.DrawImage( bmp, 0, 0 );
			}
			return adjBmp;
		}
		
		public static void Dispose() {
			measuringBmp.Dispose();
			measuringGraphics.Dispose();
			foreach( var pair in brushCache ) {
				pair.Value.Dispose();
			}
		}
		
		static Color[] colours = new Color[] {
			Color.FromArgb( 0, 0, 0 ), // black
			Color.FromArgb( 0, 0, 191 ), // dark blue
			Color.FromArgb( 0, 191, 0 ), // dark green
			Color.FromArgb( 0, 191, 191 ), // dark teal
			Color.FromArgb( 191, 0, 0 ), // dark red
			Color.FromArgb( 191, 0, 191 ), // purple
			Color.FromArgb( 191, 191, 0 ), // gold
			Color.FromArgb( 191, 191, 191 ), // gray
			Color.FromArgb( 64, 64, 64 ), // dark gray
			Color.FromArgb( 64, 64, 255 ), // blue
			Color.FromArgb( 64, 255, 64 ), // lime
			Color.FromArgb( 64, 255, 255 ), // teal
			Color.FromArgb( 255, 64, 64 ), // red
			Color.FromArgb( 255, 64, 255 ), // pink
			Color.FromArgb( 255, 255, 64 ), // yellow
			Color.FromArgb( 255, 255, 255 ), // white
		};

		static List<DrawTextArgs> parts = new List<DrawTextArgs>( 64 );
		public static List<DrawTextArgs> SplitText( IGraphicsApi graphics, string value, bool shadow ) {
			int code = 15;
			parts.Clear();
			for( int i = 0; i < value.Length; i++ ) {
				int nextAnd = value.IndexOf( '&', i );
				int partLength = nextAnd == -1 ? value.Length - i : nextAnd - i;
				
				if( partLength > 0 ) {
					string part = value.Substring( i, partLength );
					parts.Add( new DrawTextArgs( graphics, part, colours[code], shadow ) );
				}
				i += partLength + 1;
				code = nextAnd == -1 ? 0 : Utils.ParseHex( value[nextAnd + 1] );
			}
			return parts;
		}
	}
}
