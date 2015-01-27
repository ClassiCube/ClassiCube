using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	// NOTE: These delegates should be removed when using versions later than NET 2.0.
	// ################################################################
	public delegate void Action<T1, T2>( T1 arg1, T2 arg2 );
	public delegate void Action();
	public delegate void Action<T1, T2, T3>( T1 arg1, T2 arg2, T3 arg3 );
	public delegate TResult Func<TResult>();
	public delegate TResult Func<T1, TResult>( T1 arg1 );
	public delegate TResult Func<T1, T2, TResult>( T1 arg1, T2 arg2 );
	// ################################################################
	
	public static class Utils {
		
		public static string AppName = "ClassicalSharp 0.4";
		
		public static int NextPowerOf2( int value ) {
			int next = 1;
			while( value > next ) {
				next <<= 1;
			}
			return next;
		}
		
		public static string StripColours( string value ) {
			if( value.IndexOf( '&' ) == -1 ) {
				return value;
			}
			
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

		public static List<DrawTextArgs> SplitText( IGraphicsApi graphics, string value, bool shadow ) {
			int code = 15;
			List<DrawTextArgs> parts = new List<DrawTextArgs>();
			for( int i = 0; i < value.Length; i++ ) {
				int nextAnd = value.IndexOf( '&', i );
				int partLength = nextAnd == -1 ? value.Length - i : nextAnd - i;
				
				if( partLength > 0 ) {
					string part = value.Substring( i, partLength );
					parts.Add( new DrawTextArgs( graphics, part, colours[code], shadow ) );
				}
				i += partLength + 1;
				code = nextAnd == -1 ? 0 : ParseHex( value[nextAnd + 1] );
			}
			return parts;
		}
		
		public static string ToHexString( byte[] array ) {
			int len = array.Length;
			char[] hexadecimal = new char[len * 2];
			for( int i = 0; i < array.Length; i++ ) {
				int value = array[i];
				int index = i << 1;
				int upperNibble = value >> 4;
				int lowerNibble = value & 0x0F;
				hexadecimal[index] = upperNibble < 10 ? (char)( upperNibble + 48 ) : (char)( upperNibble + 55 ); // 48 = index of 0, 55 = index of (A - 10).
				hexadecimal[index + 1] = lowerNibble < 10 ? (char)( lowerNibble + 48 ) : (char)( lowerNibble + 55 );
			}
			return new String( hexadecimal );
		}
		
		public static int ParseHex( char value ) {
			if( value >= '0' && value <= '9' ) {
				return (int)( value - '0' );
			} else if( value >= 'a' && value <= 'f' ) {
				return (int)( value - 'a' ) + 10;
			} else if( value >= 'A' && value <= 'F' ) {
				return (int)( value - 'A' ) + 10;
			} else {
				throw new FormatException( "Invalid hex code given: " + value );
			}
		}
		
		public static double DegreesToRadians( double degrees ) {
			return degrees * Math.PI / 180.0;
		}
		
		public static double RadiansToDegrees( double radians ) {
			return radians * 180.0 / Math.PI;
		}
		
		// Basically this works as a special unit circle in the range of [0..255].
		public static byte RadiansToPacked( double radians ) {
			return DegreesToPacked( radians * 180.0 / Math.PI );
		}
		
		public static byte DegreesToPacked( double degrees ) {
			int packed = (int)( degrees * 256.0 / 360.0 ) % 256; // 256 = period
			if( packed < 0 )
				packed += 256; // Normalise into [0..255];
			return (byte)packed;
		}
		
		public static double PackedToDegrees( byte packed ) {
			return packed * 360.0 / 256.0;
		}
		
		public static double PackedToRadians( byte packed ) {
			return PackedToDegrees( packed ) * Math.PI / 180.0;
		}
		
		public static float DistanceSquared( Vector3 p1, Vector3 p2 ) {
			float dx = p2.X - p1.X;
			float dy = p2.Y - p1.Y;
			float dz = p2.Z - p1.Z;
			return dx * dx + dy * dy + dz * dz;
		}
		
		public static float DistanceSquared( float x1, float y1, float z1, float x2, float y2, float z2 ) {
			float dx = x2 - x1;
			float dy = y2 - y1;
			float dz = z2 - z1;
			return dx * dx + dy * dy + dz * dz;
		}
		
		public static int DistanceSquared( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			int dx = x2 - x1;
			int dy = y2 - y1;
			int dz = z2 - z1;
			return dx * dx + dy * dy + dz * dz;
		}
		
		public static Vector3 GetDirectionVector( double yawRad, double pitchRad ) {
			double x = Math.Cos( pitchRad ) * -Math.Sin( yawRad );
			double y = Math.Sin( pitchRad );
			double z = Math.Cos( pitchRad ) * Math.Cos( yawRad );
			return new Vector3( (float)x, (float)y, (float)z );
		}
		
		static string SplitUppercase( string value ) {
			StringBuilder buffer = new StringBuilder( value.Length + 3 );
			for( int i = 0; i < value.Length; i++ ) {
				char c = value[i];
				if( Char.IsUpper( c ) && buffer.Length > 0 ) {
					buffer.Append( ' ' );
				}
				buffer.Append( c );
			}
			return buffer.ToString();
		}
		
		public static string GetSpacedBlockName( Block block ) {
			return block == Block.TNT ? "TNT" : SplitUppercase( block.ToString() );
		}
		
		public static void LogWarning( string text ) {
			Console.ForegroundColor = ConsoleColor.Yellow;
			Console.WriteLine( text );
			Console.ResetColor();
		}
		
		public static void LogError( string text ) {
			Console.ForegroundColor = ConsoleColor.Red;
			Console.WriteLine( text );
			Console.ResetColor();
		}
		
		public static void LogDebug( string text ) {
			#if DEBUG
			Console.ForegroundColor = ConsoleColor.DarkGray;
			Console.WriteLine( text );
			Console.ResetColor();
			#endif
		}
		
		public static void LogWarning( string text, params object[] args ) {
			LogWarning( String.Format( text, args ) );
		}
		
		public static void LogError( string text, params object[] args ) {
			LogError( String.Format( text, args ) );
		}
		
		public static void LogDebug( string text, params object[] args ) {
			#if DEBUG
			LogDebug( String.Format( text, args ) );
			#endif
		}
		
		public static int Floor( float value ) {
			return value >= 0 ? (int)value : (int)value - 1;
		}
		
		public static float Lerp( float a, float b, float t ) {
			return a + ( b - a ) * t;
		}
		
		public static float InterpAngle( float leftAngle, float rightAngle, float t ) {
			// we have to cheat a bit for angles here.
			// Consider 350* --> 0*, we only want to travel 10*,
			// but without adjusting for this case, we would interpolate back the whole 350* degrees.
			bool invertLeft = leftAngle > 270 && rightAngle < 90;
			bool invertRight = rightAngle > 270 && leftAngle < 90;
			if( invertLeft ) leftAngle = 360 - leftAngle;
			if( invertRight ) rightAngle = 360 - rightAngle;
			
			return Lerp( leftAngle, rightAngle, t );
		}
		
		public static SkinType GetSkinType( Bitmap bmp ) {
			if( bmp.Width == 64 && bmp.Height == 32 ) {
				return SkinType.Type64x32;
			} else if( bmp.Width == 64 && bmp.Height == 64 ) {
				bool isNormal = bmp.GetPixel( 54, 20 ).A >= 127;
				return isNormal ? SkinType.Type64x64 : SkinType.Type64x64Slim;
			} else {
				throw new NotSupportedException( "unsupported skin: " + bmp.Width + ", " + bmp.Height );
			}
		}
	}
}