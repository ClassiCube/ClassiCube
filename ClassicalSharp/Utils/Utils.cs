using System;
using System.Drawing;
using OpenTK;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
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
	public delegate bool TryParseFunc<T>( string s, out T value );
	// ################################################################
	
	public static class Utils {
		
		/// <summary> Clamps that specified value such that min ≤ value ≤ max </summary>
		public static void Clamp( ref float value, float min, float max ) {
			if( value < min ) value = min;
			if( value > max ) value = max;
		}
		
		/// <summary> Clamps that specified value such that min ≤ value ≤ max </summary>
		public static void Clamp( ref int value, int min, int max ) {
			if( value < min ) value = min;
			if( value > max ) value = max;
		}
		
		/// <summary> Returns the next highest power of 2 that is ≥ to the given value. </summary>
		public static int NextPowerOf2( int value ) {
			int next = 1;
			while( value > next ) {
				next <<= 1;
			}
			return next;
		}
		
		/// <summary> Returns whether the given value is a power of 2. </summary>
		public static bool IsPowerOf2( int value ) {
			return value != 0 && (value & (value - 1)) == 0;
		}
		
		/// <summary> Returns a string with all the colour codes stripped from it. </summary>
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
		
		/// <summary> Returns a string with a + removed if it is the last character in the string. </summary>
		public static string RemoveEndPlus( string value ) {
			// Workaround for MCDzienny (and others) use a '+' at the end to distinguish classicube.net accounts
			// from minecraft.net accounts. Unfortunately they also send this ending + to the client.
			if( String.IsNullOrEmpty( value ) ) return value;
			
			return value[value.Length - 1] == '+' ?
				value.Substring( 0, value.Length - 1 ) : value;
		}
		
		/// <summary> Returns whether a equals b, ignoring any case differences. </summary>
		public static bool CaselessEquals( string a, string b ) {
			return a.Equals( b, StringComparison.OrdinalIgnoreCase );
		}
		
		/// <summary> Returns whether a starts with b, ignoring any case differences. </summary>
		public static bool CaselessStarts( string a, string b ) {
			return a.StartsWith( b, StringComparison.OrdinalIgnoreCase );
		}
		
		/// <summary> Converts the given byte array of length N to a hex string of length 2N. </summary>
		public static string ToHexString( byte[] array ) {
			int len = array.Length;
			char[] hexadecimal = new char[len * 2];
			for( int i = 0; i < array.Length; i++ ) {
				int value = array[i];
				int upper = value >> 4;
				int lower = value & 0x0F;
				
				// 48 = index of 0, 55 = index of (A - 10).
				hexadecimal[i * 2] = upper < 10 ? (char)(upper + 48) : (char)(upper + 55);
				hexadecimal[i * 2 + 1] = lower < 10 ? (char)(lower + 48) : (char)(lower + 55);
			}
			return new String( hexadecimal );
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
		
		/// <summary> Multiply a value in degrees by this to get its value in radians. </summary>
		public const float Deg2Rad = (float)(Math.PI / 180);
		/// <summary> Multiply a value in radians by this to get its value in degrees. </summary>
		public const float Rad2Deg = (float)(180 / Math.PI);
		
		public static int DegreesToPacked( double degrees, int period ) {
			return (int)(degrees * period / 360.0) % period;
		}
		
		public static double PackedToDegrees( byte packed ) {
			return packed * 360.0 / 256.0;
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static Vector3 RotateY( Vector3 v, float angle ) {
			float cosA = (float)Math.Cos( angle );
			float sinA = (float)Math.Sin( angle );
			return new Vector3( cosA * v.X - sinA * v.Z, v.Y, sinA * v.X + cosA * v.Z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static Vector3 RotateY( float x, float y, float z, float angle ) {
			float cosA = (float)Math.Cos( angle );
			float sinA = (float)Math.Sin( angle );
			return new Vector3( cosA * x - sinA * z, y, sinA * x + cosA * z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the x axis. </summary>
		public static Vector3 RotateX( Vector3 p, float cosA, float sinA ) {
			return new Vector3( p.X, cosA * p.Y + sinA * p.Z, -sinA * p.Y + cosA * p.Z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the x axis. </summary>
		public static Vector3 RotateX( float x, float y, float z, float cosA, float sinA ) {
			return new Vector3( x, cosA * y + sinA * z, -sinA * y + cosA * z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static Vector3 RotateY( Vector3 p, float cosA, float sinA ) {
			return new Vector3( cosA * p.X - sinA * p.Z, p.Y, sinA * p.X + cosA * p.Z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the y axis. </summary>
		public static Vector3 RotateY( float x, float y, float z, float cosA, float sinA ) {
			return new Vector3( cosA * x - sinA * z, y, sinA * x + cosA * z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the z axis. </summary>
		public static Vector3 RotateZ( Vector3 p, float cosA, float sinA ) {
			return new Vector3( cosA * p.X + sinA * p.Y, -sinA * p.X + cosA * p.Y, p.Z );
		}
		
		/// <summary> Rotates the given 3D coordinates around the z axis. </summary>
		public static Vector3 RotateZ( float x, float y, float z, float cosA, float sinA ) {
			return new Vector3( cosA * x + sinA * y, -sinA * x + cosA * y, z );
		}
		
		/// <summary> Returns the square of the euclidean distance between two points. </summary>
		public static float DistanceSquared( Vector3 p1, Vector3 p2 ) {
			float dx = p2.X - p1.X;
			float dy = p2.Y - p1.Y;
			float dz = p2.Z - p1.Z;
			return dx * dx + dy * dy + dz * dz;
		}
		
		/// <summary> Returns the square of the euclidean distance between two points. </summary>
		public static float DistanceSquared( float x1, float y1, float z1, float x2, float y2, float z2 ) {
			float dx = x2 - x1;
			float dy = y2 - y1;
			float dz = z2 - z1;
			return dx * dx + dy * dy + dz * dz;
		}
		
		/// <summary> Returns the square of the euclidean distance between two points. </summary>
		public static int DistanceSquared( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			int dx = x2 - x1;
			int dy = y2 - y1;
			int dz = z2 - z1;
			return dx * dx + dy * dy + dz * dz;
		}
		
		/// <summary> Returns a normalised vector that faces in the direction
		/// described by the given yaw and pitch. </summary>
		public static Vector3 GetDirVector( double yawRad, double pitchRad ) {
			double x = -Math.Cos( pitchRad ) * -Math.Sin( yawRad );
			double y = -Math.Sin( pitchRad );
			double z = -Math.Cos( pitchRad ) * Math.Cos( yawRad );
			return new Vector3( (float)x, (float)y, (float)z );
		}
		
		public static void GetHeading( Vector3 dir, out double yawRad, out double pitchRad ) {
			pitchRad = Math.Asin( -dir.Y );
			yawRad = Math.Atan2( dir.Z, dir.X );
		}
		
		public static void LogDebug( string text ) {
			Console.WriteLine( text );
		}
		
		public static void LogDebug( string text, params object[] args ) {
			Console.WriteLine( String.Format( text, args ) );
		}
		
		public static int Floor( float value ) {
			int valueI = (int)value;
			return value < valueI ? valueI - 1 : valueI;
		}
		
		public static int AdjViewDist( int value ) {
			return (int)(1.4142135 * value);
		}
		
		/// <summary> Returns the number of vertices needed to subdivide a quad. </summary>
		internal static int CountVertices( int axis1Len, int axis2Len, int axisSize ) {
			return CeilDiv( axis1Len, axisSize ) * CeilDiv( axis2Len, axisSize ) * 4;
		}
		
		/// <summary> Performs rounding upwards integer division. </summary>
		public static int CeilDiv( int a, int b ) {
			return a / b + (a % b != 0 ? 1 : 0);
		}
		
		/// <summary> Performs linear interpolation between two values. </summary>
		public static float Lerp( float a, float b, float t ) {
			return a + (b - a) * t;
		}
		
		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		public static void CalcBillboardPoints( Vector2 size, Vector3 position, ref Matrix4 view, out Vector3 p111,
		                                       out Vector3 p121, out Vector3 p212, out Vector3 p222 ) {
			Vector3 centre = position; centre.Y += size.Y / 2;
			Vector3 right = new Vector3( view.Row0.X, view.Row1.X, view.Row2.X );
			Vector3 up = new Vector3( view.Row0.Y, view.Row1.Y, view.Row2.Y );
			
			p111 = Transform( -0.5f, -0.5f, ref size, ref centre, ref up, ref right );
			p121 = Transform( -0.5f, 0.5f, ref size, ref centre, ref up, ref right );
			p212 = Transform( 0.5f, -0.5f, ref size, ref centre, ref up, ref right );
			p222 = Transform( 0.5f, 0.5f, ref size, ref centre, ref up, ref right );
		}
		
		static Vector3 Transform( float x, float y, ref Vector2 size,
		                         ref Vector3 centre, ref Vector3 up, ref Vector3 right ) {
			return centre + right * x * size.X + up * y * size.Y;
		}
		
		
		/// <summary> Linearly interpolates between a given angle range, adjusting if necessary. </summary>
		public static float LerpAngle( float leftAngle, float rightAngle, float t ) {
			// we have to cheat a bit for angles here.
			// Consider 350* --> 0*, we only want to travel 10*,
			// but without adjusting for this case, we would interpolate back the whole 350* degrees.
			bool invertLeft = leftAngle > 270 && rightAngle < 90;
			bool invertRight = rightAngle > 270 && leftAngle < 90;
			if( invertLeft ) leftAngle = leftAngle - 360;
			if( invertRight ) rightAngle = rightAngle - 360;
			
			return Lerp( leftAngle, rightAngle, t );
		}
		
		/// <summary> Determines the skin type of the specified bitmap. </summary>
		public static SkinType GetSkinType( Bitmap bmp ) {
			if( bmp.Width == bmp.Height * 2 ) {
				return SkinType.Type64x32;
			} else if( bmp.Width == bmp.Height ) {
				// Minecraft alex skins have this particular pixel with alpha of 0.
				if( bmp.Width == 64 ) {
					int scale = bmp.Width / 64;
					bool isNormal = bmp.GetPixel( 54 * scale, 20 * scale ).A >= 127;
					return isNormal ? SkinType.Type64x64 : SkinType.Type64x64Slim;
				} else {
					return SkinType.Type64x64;
				}
			} else {
				throw new NotSupportedException( "unsupported skin dimensions: " + bmp.Width + ", " + bmp.Height );
			}
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
	}
}