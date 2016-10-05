// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Model;
using OpenTK;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
using AndroidColor = Android.Graphics.Color;
#endif

namespace ClassicalSharp {

	// NOTE: These delegates should be removed when using versions later than NET 2.0.
	// ################################################################
	public delegate void Action();
	public delegate void Action<T1, T2>( T1 arg1, T2 arg2 );
	public delegate void Action<T1, T2, T3>( T1 arg1, T2 arg2, T3 arg3 );
	public delegate void Action<T1, T2, T3, T4>( T1 arg1, T2 arg2, T3 arg3, T4 arg4 );
	public delegate TResult Func<TResult>();
	public delegate TResult Func<T1, TResult>( T1 arg1 );
	public delegate TResult Func<T1, T2, TResult>( T1 arg1, T2 arg2 );
	public delegate TResult Func<T1, T2, T3, TResult>( T1 arg1, T2 arg2, T3 arg3 );
	// ################################################################
	
	public static partial class Utils {
		
		public const int StringLength = 64;
		
		/// <summary> Returns a string with all the colour codes stripped from it. </summary>
		public static string StripColours( string value ) {
			if( value.IndexOf( '&' ) == -1 ) return value;
			char[] output = new char[value.Length];
			int usedChars = 0;
			
			for( int i = 0; i < value.Length; i++ ) {
				char token = value[i];
				if( token == '&' ) {
					i++; // Skip over the following colour code.
				} else {
					output[usedChars++] = token;
				}
			}
			return new String( output, 0, usedChars );
		}
		
		/// <summary> Returns a string with a + removed if it is the last character in the string. </summary>
		public static string RemoveEndPlus( string value ) {
			// Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts
			// from minecraft.net accounts. Unfortunately they also send this ending + to the client.
			if( String.IsNullOrEmpty( value ) ) return value;
			
			return value[value.Length - 1] == '+' ?
				value.Substring( 0, value.Length - 1 ) : value;
		}
		
		const StringComparison comp = StringComparison.OrdinalIgnoreCase;
		/// <summary> Returns whether a equals b, ignoring any case differences. </summary>
		public static bool CaselessEquals( string a, string b ) { return a.Equals( b, comp ); }
		
		/// <summary> Returns whether a starts with b, ignoring any case differences. </summary>
		public static bool CaselessStarts( string a, string b ) { return a.StartsWith( b, comp ); }
		
		/// <summary> Returns whether a ends with b, ignoring any case differences. </summary>
		public static bool CaselessEnds( string a, string b ) { return a.EndsWith( b, comp ); }
		
		/// <summary> Converts the given byte array of length N to a hex string of length 2N. </summary>
		public static string ToHexString( byte[] array ) {
			int len = array.Length;
			char[] hex = new char[len * 2];
			for( int i = 0; i < array.Length; i++ ) {
				int value = array[i];
				int hi = value >> 4, lo = value & 0x0F;
				
				// 48 = index of 0, 55 = index of (A - 10).
				hex[i * 2 + 0] = hi < 10 ? (char)(hi + 48) : (char)(hi + 55);
				hex[i * 2 + 1] = lo < 10 ? (char)(lo + 48) : (char)(lo + 55);
			}
			return new String( hex );
		}
		
		/// <summary> Returns the hex code represented by the given character.
		/// Throws FormatException if the input character isn't a hex code. </summary>
		public static int ParseHex( char value ) {
			int hex;
			if( !TryParseHex( value, out hex ) )
				throw new FormatException( "Invalid hex code given: " + value );
			return hex;
		}
		
		/// <summary> Attempts to return the hex code represented by the given character. </summary>
		public static bool TryParseHex( char value, out int hex ) {
			hex = 0;
			if( value >= '0' && value <= '9') {
				hex = (int)(value - '0');
			} else if( value >= 'a' && value <= 'f') {
				hex = (int)(value - 'a') + 10;
			} else if( value >= 'A' && value <= 'F') {
				hex = (int)(value - 'A') + 10;
			} else {
				return false;
			}
			return true;
		}

		/// <summary> Attempts to caselessly parse the given string as a Key enum member,
		/// returning defValue if there was an error parsing. </summary>
		public static bool TryParseEnum<T>( string value, T defValue, out T result ) {
			T mapping;
			try {
				mapping = (T)Enum.Parse( typeof(T), value, true );
			} catch( ArgumentException ) {
				result = defValue;
				return false;
			}
			result = mapping;
			return true;
		}
		
		public static void LogDebug( string text ) {
			Console.WriteLine( text );
		}
		
		public static void LogDebug( string text, params object[] args ) {
			Console.WriteLine( String.Format( text, args ) );
		}
		
		public static int AdjViewDist( float value ) {
			return (int)(1.4142135 * value);
		}
		
		/// <summary> Returns the number of vertices needed to subdivide a quad. </summary>
		internal static int CountVertices( int axis1Len, int axis2Len, int axisSize ) {
			return CeilDiv( axis1Len, axisSize ) * CeilDiv( axis2Len, axisSize ) * 4;
		}
		
		internal static byte FastByte( string s ) {
			int sum = 0;
			switch( s.Length ) {
				case 1: sum = (s[0] - '0'); break;
				case 2: sum = (s[0] - '0') * 10 + (s[1] - '0'); break;
				case 3: sum = (s[0] - '0') * 100 + (s[1] - '0') * 10 + (s[2] - '0'); break;
			}
			return (byte)sum;
		}
		
		/// <summary> Determines the skin type of the specified bitmap. </summary>
		public static SkinType GetSkinType( Bitmap bmp ) {
			if( bmp.Width == bmp.Height * 2 ) {
				return SkinType.Type64x32;
			} else if( bmp.Width == bmp.Height ) {
				// Minecraft alex skins have this particular pixel with alpha of 0.
				int scale = bmp.Width / 64;
				
				#if !ANDROID
				int alpha = bmp.GetPixel( 54 * scale, 20 * scale ).A;
				#else
				int alpha = AndroidColor.GetAlphaComponent( bmp.GetPixel( 54 * scale, 20 * scale ) );
				#endif
				return alpha >= 127 ? SkinType.Type64x64 : SkinType.Type64x64Slim;
			}
			return SkinType.Invalid;
		}
		
		/// <summary> Returns whether the specified string starts with http:// or https:// </summary>
		public static bool IsUrlPrefix( string value, int index ) {
			int http = value.IndexOf( "http://", index );
			int https = value.IndexOf( "https://", index );
			return http == index || https == index;
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
		
		/// <summary> Conversion for code page 437 characters from index 0 to 31 to unicode. </summary>
		public const string ControlCharReplacements = "\0☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼";
		
		/// <summary> Conversion for code page 437 characters from index 127 to 255 to unicode. </summary>
		public const string ExtendedCharReplacements = "⌂ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»" +
			"░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌" +
			"█▄▌▐▀αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■\u00a0";
		
		public unsafe static string ToLower( string value ) {
			fixed( char* ptr = value ) {
				for( int i = 0; i < value.Length; i++ ) {
					char c = ptr[i];
					if( c < 'A' || c > 'Z' ) continue;			
					c += ' '; ptr[i] = c;
				}
			}
			return value;
		}
	}
}