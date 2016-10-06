// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {

	/// <summary> Class responsible for performing drawing operations on bitmaps
	/// and for converting bitmaps into graphics api textures. </summary>
	/// <remarks> Uses GDI+ on Windows, uses Cairo on Mono. </remarks>
	public abstract partial class IDrawer2D : IDisposable {
		
		protected IGraphicsApi graphics;
		public const float Offset = 1.3f;
		
		/// <summary>Whether chat text should be drawn and measuring using the currently bitmapped font, 
		/// false uses the font supplied as the DrawTextArgs argument supplied to the function. </summary>
		public bool UseBitmappedChat = false;
		
		/// <summary> Whether the shadows behind text (that uses shadows) is fully black. </summary>
		public bool BlackTextShadows;
		
		/// <summary> Sets the underlying bitmap that drawing operations will be performed on. </summary>
		public abstract void SetBitmap( Bitmap bmp );
		
		/// <summary> Draws a 2D flat rectangle of the specified dimensions at the
		/// specified coordinates in the currently bound bitmap. </summary>
		public abstract void DrawRect( FastColour colour, int x, int y, int width, int height );
		
		/// <summary> Draws the outline of a 2D flat rectangle of the specified dimensions
		/// at the specified coordinates in the currently bound bitmap. </summary>
		public abstract void DrawRectBounds( FastColour colour, float lineWidth, int x, int y, int width, int height );
		
		/// <summary> Clears the entire bound bitmap to the specified colour. </summary>
		public abstract void Clear( FastColour colour );
		
		/// <summary> Clears the entire given area to the specified colour. </summary>
		public abstract void Clear( FastColour colour, int x, int y, int width, int height );
		
		/// <summary> Disposes of any resources used by this class that are associated with the underlying bitmap. </summary>
		public abstract void Dispose();
		
		/// <summary> Returns a new bitmap that has 32-bpp pixel format. </summary>
		public abstract Bitmap ConvertTo32Bpp( Bitmap src );
		
		/// <summary> Returns a new bitmap that has 32-bpp pixel format. </summary>
		public void ConvertTo32Bpp( ref Bitmap src ) {
			Bitmap newBmp = ConvertTo32Bpp( src );
			src.Dispose();
			src = newBmp;
		}
		
		/// <summary> Draws a string using the specified arguments and font at the
		/// specified coordinates in the currently bound bitmap. </summary>
		public abstract void DrawText( ref DrawTextArgs args, int x, int y );
		
		/// <summary> Draws a string using the specified arguments and fonts at the
		/// specified coordinates in the currently bound bitmap, clipping if necessary. </summary>
		public abstract void DrawClippedText( ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight );
		
		/// <summary> Draws a string using the specified arguments and the current bitmapped font at the
		/// specified coordinates in the currently bound bitmap. </summary>
		public abstract void DrawBitmappedText( ref DrawTextArgs args, int x, int y );
		
		/// <summary> Draws a string using the specified arguments, using the specified font or 
		/// the current bitmapped font depending on 'UseBitmappedChat', at the
		/// specified coordinates in the currently bound bitmap. </summary>
		public void DrawChatText( ref DrawTextArgs args, int windowX, int windowY ) {
			if( !UseBitmappedChat )
				DrawText( ref args, windowX, windowY );
			else
				DrawBitmappedText( ref args, windowX, windowY );
		}
		
		/// <summary> Returns the size of a bitmap needed to contain the specified text with the given arguments. </summary>
		public abstract Size MeasureSize( ref DrawTextArgs args );
		
		/// <summary> Returns the size of a bitmap needed to contain the specified text with the given arguments,
		/// when drawn with the current bitmapped font. </summary>
		public abstract Size MeasureBitmappedSize( ref DrawTextArgs args );
		
		/// <summary> Returns the size of a bitmap needed to contain the specified text with the given arguments,
		/// when drawn with the specified font or the current bitmapped font depending on 'UseBitmappedChat'. </summary>
		public Size MeasureChatSize( ref DrawTextArgs args ) {
			return !UseBitmappedChat ? MeasureSize( ref args ) : 
				MeasureBitmappedSize( ref args );
		}
		
		/// <summary> Draws the specified string from the arguments into a new bitmap,
		/// then creates a 2D texture with origin at the specified window coordinates. </summary>
		public Texture MakeTextTexture( ref DrawTextArgs args, int windowX, int windowY ) {
			return MakeTextureImpl( ref args, windowX, windowY, false );
		}
		
		/// <summary> Draws the specified string from the arguments into a new bitmap,
		/// using the current bitmap font, then creates a 2D texture with origin at the 
		/// specified window coordinates. </summary>
		public Texture MakeBitmappedTextTexture( ref DrawTextArgs args, int windowX, int windowY ) {
			return MakeTextureImpl( ref args, windowX, windowY, true );
		}
		
		/// <summary> Draws the specified string from the arguments into a new bitmap,
		/// using the specified font or the current bitmapped font depending on 'UseBitmappedChat',
		/// then creates a 2D texture with origin at the specified window coordinates. </summary>
		public Texture MakeChatTextTexture( ref DrawTextArgs args, int windowX, int windowY ) {
			return MakeTextureImpl( ref args, windowX, windowY, UseBitmappedChat );
		}
		
		Texture MakeTextureImpl( ref DrawTextArgs args, int windowX, int windowY, bool bitmapped ) {
			Size size = bitmapped ? MeasureBitmappedSize( ref args ) : MeasureSize( ref args );
			if( size == Size.Empty )
				return new Texture( -1, windowX, windowY, 0, 0, 1, 1 );
			
			using( Bitmap bmp = CreatePow2Bitmap( size ) ) {
				SetBitmap( bmp );
				args.SkipPartsCheck = true;
				
				if( !bitmapped )
					DrawText( ref args, 0, 0 );
				else
					DrawBitmappedText( ref args, 0, 0 );
				Dispose();
				return Make2DTexture( bmp, size, windowX, windowY );
			}
		}
		
		
		/// <summary> Disposes of all native resources used by this class. </summary>
		/// <remarks> You will no longer be able to perform measuring or drawing calls after this. </remarks>
		public abstract void DisposeInstance();
		
		/// <summary> Creates a 2D texture with origin at the specified window coordinates. </summary>
		public Texture Make2DTexture( Bitmap bmp, Size used, int windowX, int windowY ) {			
			int texId = graphics.CreateTexture( bmp, true );
			return new Texture( texId, windowX, windowY, used.Width, used.Height,
			                   (float)used.Width / bmp.Width, (float)used.Height / bmp.Height );
		}
		
		/// <summary> Creates a power-of-2 sized bitmap larger or equal to to the given size. </summary>
		public static Bitmap CreatePow2Bitmap( Size size ) {
			return Platform.CreateBmp( Utils.NextPowerOf2( size.Width ), Utils.NextPowerOf2( size.Height ) );
		}
		
		public FastColour[] Colours = new FastColour[256];
		
		public IDrawer2D() { InitColours(); }
		
		public void InitColours() {
			for( int i = 0; i < Colours.Length; i++ )
				Colours[i] = default(FastColour);
			
			for( int i = 0; i <= 9; i++ )
				Colours['0' + i] = FastColour.GetHexEncodedCol( i, 191, 64 );
			for( int i = 10; i <= 15; i++) {
				Colours['a' + i - 10] = FastColour.GetHexEncodedCol( i, 191, 64 );
				Colours['A' + i - 10] = Colours['a' + i - 10];
			}
		}
		
		protected List<TextPart> parts = new List<TextPart>( 64 );
		protected struct TextPart {
			public string Text;
			public FastColour Col;
			
			public TextPart( string text, FastColour col ) {
				Text = text;
				Col = col;
			}
		}
		
		protected void GetTextParts( string value ) {
			parts.Clear();
			if( String.IsNullOrEmpty( value ) ) {
			} else if( value.IndexOf( '&' ) == -1 ) {
				parts.Add( new TextPart( value, Colours['f'] ) );
			} else {
				SplitText( value );
			}
		}
		
		/// <summary> Splits the input string by recognised colour codes. (e.g &amp;f) </summary>
		protected void SplitText( string value ) {
			char code = 'f';
			for( int i = 0; i < value.Length; i++ ) {
				int nextAnd = value.IndexOf( '&', i );
				int partLength = nextAnd == -1 ? value.Length - i : nextAnd - i;
				
				if( partLength > 0 ) {
					string part = value.Substring( i, partLength );
					parts.Add( new TextPart( part, Colours[code] ) );
				}
				i += partLength + 1;
				
				if( nextAnd >= 0 && nextAnd + 1 < value.Length ) {
					if( !ValidColour( value[nextAnd + 1] ) ) {
						i--; // include character that isn't a valid colour code.
					} else {
						code = value[nextAnd + 1];
					}
				}
			}
		}
		
		/// <summary> Returns whenever the given character is a valid colour code. </summary>
		public bool ValidColour( char c ) {
			return (int)c < 256 && Colours[c].A > 0;
		}
		
		/// <summary> Returns the last valid colour code in the given input, 
		/// or \0 if no valid colour code was found. </summary>
		public char LastColour( string input, int start ) {
			if( start >= input.Length )
				start = input.Length - 1;
			
			for( int i = start; i >= 0; i--) {
				if( input[i] != '&' ) continue;
				if( i < input.Length - 1 && ValidColour( input[i + 1] ) )
					return input[i + 1];
			}
			return '\0';
		}
		
		public static bool IsWhiteColour( char c ) {
			return c == '\0' || c == 'f' || c == 'F';
		}
		
		public void ReducePadding( ref Texture tex, int point ) {
			ReducePadding( ref tex, point, 4 );
		}

		public void ReducePadding( ref int height, int point ) {
			ReducePadding( ref height, point, 4 );
		}
		
		public void ReducePadding( ref Texture tex, int point, int scale ) {
			if( !UseBitmappedChat ) return;
			point = AdjTextSize( point );
			
			int padding = (tex.Height - point) / scale;
			float vAdj = (float)padding / Utils.NextPowerOf2( tex.Height );
			tex.V1 += vAdj; tex.V2 -= vAdj;
			tex.Height -= (short)(padding * 2);
		}

		public void ReducePadding( ref int height, int point, int scale ) {
			if( !UseBitmappedChat ) return;
			point = AdjTextSize( point );
			
			int padding = (height - point) / scale;
			height -= padding * 2;
		}
	}
}
