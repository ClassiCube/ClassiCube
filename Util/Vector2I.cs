using System;
using OpenTK;

namespace ClassicalSharp {	

	public struct Vector2I {
		
		public static Vector2I Zero = new Vector2I( 0, 0 );
		
		public int X, Y;
		
		public float Length {
			get { return (float)Math.Sqrt( X * X + Y * Y ); }
		}
		
		public int LengthSquared {
			get { return X * X + Y * Y; }
		}
		
		public Vector2I( int x, int y ) {
			X = x;
			Y = y;
		}
		
		public Vector2I( int value ) {
			X = value;
			Y = value;
		}
		
		public static Vector2I operator + ( Vector2I left, Vector2I right ) {
			return new Vector2I( left.X + right.X, left.Y + right.Y );
		}
		
		public static Vector2I operator - ( Vector3I left, Vector2I right ) {
			return new Vector2I( left.X - right.X, left.Y - right.Y );
		}
		
		public override bool Equals( object obj ) {
			return obj is Vector2I && Equals( (Vector2I)obj );
		}
		
		public bool Equals( Vector2I other ) {
			return X == other.X && Y == other.Y;
		}
		
		public override int GetHashCode(){
			int hashCode = 1000000007 * X;
			hashCode += 1000000009 * Y;
			return hashCode;
		}
		
		public static bool operator == ( Vector2I lhs, Vector2I rhs ) {
			return lhs.X == rhs.X && lhs.Y == rhs.Y;
		}
		
		public static bool operator != (Vector2I lhs, Vector2I rhs) {
			return !( lhs.X == rhs.X && lhs.Y == rhs.Y );
		}		
		
		public override string ToString() {
			return X + "," + Y;
		}
	}
}