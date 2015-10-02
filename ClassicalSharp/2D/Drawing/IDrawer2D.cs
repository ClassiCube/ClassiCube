using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public abstract class IDrawer2D : IDisposable {
		
		protected IGraphicsApi graphics;
		protected const float shadowOffset = 1.3f;
		
		/// <summary> Sets the underlying bitmap that drawing operations will be performed on. </summary>
		public abstract void SetBitmap( Bitmap bmp );
		
		public abstract void DrawText( Font font, ref DrawTextArgs args, float x, float y );
		
		public abstract void DrawRect( Color colour, int x, int y, int width, int height );
		
		public abstract void DrawRectBounds( Color colour, float lineWidth, int x, int y, int width, int height );
		
		public abstract void DrawRoundedRect( Color colour, float radius, float x, float y, float width, float height );
		
		/// <summary> Disposes of any resources used by this class that are associated with the underlying bitmap. </summary>
		public abstract void Dispose();
		
		public abstract Bitmap ConvertTo32Bpp( Bitmap src );
		
		public abstract Size MeasureSize( string text, Font font, bool shadow );
		
		/// <summary> Disposes of all native resources used by this class. </summary>
		/// <remarks> You will no longer be able to perform measuring or drawing calls after this. </remarks>
		public abstract void DisposeInstance();
		
		public Texture MakeTextTexture( Font font, int screenX, int screenY, ref DrawTextArgs args ) {
			Size size = MeasureSize( args.Text, font, args.UseShadow );
			if( parts.Count == 0 )
				return new Texture( -1, screenX, screenY, 0, 0, 1, 1 );
			
			using( Bitmap bmp = CreatePow2Bitmap( size ) ) {
				SetBitmap( bmp );
				args.SkipPartsCheck = true;
				
				DrawText( font, ref args, 0, 0 );
				Dispose();
				return Make2DTexture( bmp, size, screenX, screenY );
			}
		}
		
		public Texture Make2DTexture( Bitmap bmp, Size used, int screenX, int screenY ) {
			int texId = graphics.CreateTexture( bmp );
			return new Texture( texId, screenX, screenY, used.Width, used.Height,
			                   (float)used.Width / bmp.Width, (float)used.Height / bmp.Height );
		}
		
		
		public static Bitmap CreatePow2Bitmap( Size size ) {
			return new Bitmap( Utils.NextPowerOf2( size.Width ), Utils.NextPowerOf2( size.Height ) );
		}		
		
		protected List<TextPart> parts = new List<TextPart>( 64 );
		protected static Color white = Color.FromArgb( 255, 255, 255 );
		protected struct TextPart {
			public string Text;
			public Color TextColour;
			
			public TextPart( string text, Color col ) {
				Text = text;
				TextColour = col;
			}
		}
		
		protected void GetTextParts( string value ) {
			parts.Clear();
			if( String.IsNullOrEmpty( value ) ) {
			} else if( value.IndexOf( '&' ) == -1 ) {
				parts.Add( new TextPart( value, white ) );
			} else {
				SplitText( value );
			}
		}
		
		protected void SplitText( string value ) {
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
