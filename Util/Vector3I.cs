using System;
using OpenTK;

namespace ClassicalSharp {	

	public struct Vector3I {
		
		public static Vector3I Zero = new Vector3I( 0, 0, 0 );
		
		public int X, Y, Z;
		
		public float Length {
			get { return (float)Math.Sqrt( X * X + Y * Y + Z + Z ); }
		}
		
		public int LengthSquared {
			get { return X * X + Y * Y + Z + Z; }
		}
		
		public Vector3 Normalise() {
			float len = Length;
			return new Vector3( X / len, Y / len, Z / len );
		}
		
		public Vector3I( int x, int y, int z ) {
			X = x;
			Y = y;
			Z = z;
		}
		
		public Vector3I( int value ) {
			X = value;
			Y = value;
			Z = value;
		}
		
		public static Vector3I operator + ( Vector3I left, Vector3I right ) {
			return new Vector3I( left.X + right.X, left.Y + right.Y, left.Z + right.Z );
		}
		
		public static Vector3I operator - ( Vector3I left, Vector3I right ) {
			return new Vector3I( left.X - right.X, left.Y - right.Y, left.Z - right.Z );
		}
		
		public static explicit operator Vector3I( Vector3 value ) {
			return Floor( value );
		}
		
		public static explicit operator Vector3( Vector3I value ) {
			return new Vector3( value.X, value.Y, value.Z );
		}
		
		public override bool Equals( object obj ) {
			return obj is Vector3I && Equals( (Vector3I)obj );
		}
		
		public bool Equals( Vector3I other ) {
			return X == other.X && Y == other.Y && Z == other.Z;
		}
		
		public override int GetHashCode(){
			int hashCode = 1000000007 * X;
			hashCode += 1000000009 * Y;
			hashCode += 1000000021 * Z;
			return hashCode;
		}
		
		public static bool operator == ( Vector3I lhs, Vector3I rhs ) {
			return lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z;
		}
		
		public static bool operator != (Vector3I lhs, Vector3I rhs) {
			return !( lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z );
		}
		
		public static Vector3I Truncate( Vector3 vector ) {
			int x = (int)vector.X;
			int y = (int)vector.Y;
			int z = (int)vector.Z;
			return new Vector3I( x, y, z );
		}
		
		public static Vector3I Floor( Vector3 value ) {
			return new Vector3I( FastFloor( value.X ), FastFloor( value.Y ), FastFloor( value.Z ) );
		}
		
		public static Vector3I Floor( float x, float y, float z ) {
			return new Vector3I( FastFloor( x ), FastFloor( y ), FastFloor( z ) );
		}
		
		private static int FastFloor( float value ) {
			return value >= 0 ? (int)value : (int)value - 1;
		}
		
		public static Vector3I Min( Vector3I p1, Vector3I p2 ) {
			return new Vector3I( Math.Min( p1.X, p2.X ), Math.Min( p1.Y, p2.Y ), Math.Min( p1.Z, p2.Z ) );
		}
		
		public static Vector3I Min( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			return new Vector3I( Math.Min( x1, x2 ), Math.Min( y1, y2 ), Math.Min( z1, z2 ) );
		}
		
		public static Vector3I Max( Vector3I p1, Vector3I p2 ) {
			return new Vector3I( Math.Max( p1.X, p2.X ), Math.Max( p1.Y, p2.Y ), Math.Max( p1.Z, p2.Z ) );
		}
		
		public static Vector3I Max( int x1, int y1, int z1, int x2, int y2, int z2 ) {
			return new Vector3I( Math.Max( x1, x2 ), Math.Max( y1, y2 ), Math.Max( z1, z2 ) );
		}
		
		public override string ToString() {
			return X + "," + Y + "," + Z;
		}
	}
}