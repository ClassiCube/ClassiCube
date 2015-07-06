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
		
		public static Bitmap CreatePow2Bitmap( Size size ) {
			return new Bitmap( Utils.NextPowerOf2( size.Width ), Utils.NextPowerOf2( size.Height ) );
		}
		
		const float shadowOffset = 1.3f;
		public static Size MeasureSize( string text, Font font, bool shadow ) {
			GetTextParts( text );
			SizeF total = SizeF.Empty;
			for( int i = 0; i < parts.Count; i++ ) {
				SizeF size = measuringGraphics.MeasureString( parts[i].Text, font, Int32.MaxValue, format );
				total.Height = Math.Max( total.Height, size.Height );
				total.Width += size.Width;
			}
			
			if( shadow && parts.Count > 0 ) {
				total.Width += shadowOffset;
				total.Height += shadowOffset;
			}
			return Size.Ceiling( total );
		}
		
		public static void DrawText( Graphics g, Font font, ref DrawTextArgs args, float x, float y ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			DrawTextNoCheck( g, font, ref args, x, y );
		}
		
		static void DrawTextNoCheck( Graphics g, Font font, ref DrawTextArgs args, float x, float y ) {
			g.TextRenderingHint = TextRenderingHint.AntiAlias;
			Brush shadowBrush = GetOrCreateBrush( args.ShadowColour );
			
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.TextColour );
				if( args.UseShadow ) {
					g.DrawString( part.Text, font, shadowBrush, x + shadowOffset, y + shadowOffset, format );
				}
				
				g.DrawString( part.Text, font, textBrush, x, y, format );
				x += g.MeasureString( part.Text, font, Int32.MaxValue, format ).Width;
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
			if( parts.Count == 0 )
				return new Texture( -1, x1, y1, 0, 0, 1, 1 );
			
			using( Bitmap bmp = CreatePow2Bitmap( size ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					DrawTextNoCheck( g, font, ref args, 0, 0 );
				}
				return Make2DTexture( args.Graphics, bmp, size, x1, y1 );
			}
		}
		
		public static Texture Make2DTexture( IGraphicsApi graphics, Bitmap bmp, Size used, int x1, int y1 ) {
			int texId = graphics.LoadTexture( bmp );
			return new Texture( texId, x1, y1, used.Width, used.Height,
			                   (float)used.Width / bmp.Width, (float)used.Height / bmp.Height );
		}
		
		public static void Dispose() {
			measuringBmp.Dispose();
			measuringGraphics.Dispose();
			foreach( var pair in brushCache ) {
				pair.Value.Dispose();
			}
		}
		
		static List<TextPart> parts = new List<TextPart>( 64 );
		static Color white = Color.FromArgb( 255, 255, 255 );
		struct TextPart {
			public string Text;
			public Color TextColour;
			
			public TextPart( string text, Color col ) {
				Text = text;
				TextColour = col;
			}
		}
		
		static void GetTextParts( string value ) {
			parts.Clear();
			if( String.IsNullOrEmpty( value ) ) {
			} else if( value.IndexOf( '&' ) == -1 ) {
				parts.Add( new TextPart( value, white ) );
			} else {
				SplitText( value );
			}
			lastSplitText = value;
		}
		
		static string lastSplitText;
		static void SplitText( string value ) {
			int code = 0xF;
			for( int i = 0; i < value.Length; i++ ) {
				int nextAnd = value.IndexOf( '&', i );
				int partLength = nextAnd == -1 ? value.Length - i : nextAnd - i;
				
				if( partLength > 0 ) {
					string part = value.Substring( i, partLength );
					Color col = Color.FromArgb(
						191 * ( ( code >> 2 ) & 0x1 ) + 64 * ( code >> 3 ),
						191 * ( ( code >> 1 ) & 0x1 ) + 64 * ( code >> 3 ),
						191 * ( ( code >> 0 ) & 0x1 ) + 64 * ( code >> 3 ) );
					parts.Add( new TextPart( part, col ) );
				}
				i += partLength + 1;
				
				if( nextAnd >= 0 && nextAnd + 1 < value.Length ) {
					try {
						code = Utils.ParseHex( value[nextAnd + 1] );
					} catch( FormatException ) {
						code = 0xF;
						i--; // include the character that isn't a colour code.
					}
				}
			}
		}
	}
}
