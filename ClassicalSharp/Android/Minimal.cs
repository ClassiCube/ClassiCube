#if ANDROID
using System;

namespace System.Drawing {

	public struct Rectangle {
		
		public static readonly Rectangle Empty = default(Rectangle);
		public int X, Y;
		public int Width, Height;
		
		public Point Location {
			get { return new Point( X, Y ); }
			set { X = value.X; Y = value.Y; }
		}
		
		public Size Size {
			get { return new Size( Width, Height ); }
			set { Width = value.Width; Height = value.Height; }
		}

		public int Left {
			get { return X; }
		}
		
		public int Top {
			get { return Y; }
		}
		
		public int Right {
			get { return X + Width; }
		}

		public int Bottom {
			get { return Y + Height; }
		}
		
		public bool IsEmpty {
			get { return Height == 0 && Width == 0 && X == 0 && Y == 0; }
		}
		
		public Rectangle( int x, int y, int width, int height ) {
			X = x; Y = y;
			Width = width; Height = height;
		}
		
		public Rectangle( Point loc, Size size ) {
			X = loc.X; Y = loc.Y;
			Width = size.Width; Height = size.Height;
		}
		
		public static Rectangle FromLTRB( int left, int top, int right, int bottom ) {
			return new Rectangle( left, top, right - left, bottom - top );
		}
		
		public override bool Equals( object obj ) {
			return (obj is Rectangle) && Equals( (Rectangle)obj );
		}
		
		public bool Equals( Rectangle other ) {
			return X == other.X && Y == other.Y && Width == other.Width && Height == other.Height;
		}
		
		public override int GetHashCode() {
			return 1000000007 * X + 1000000009 * Y +
				1000000021 * Width + 1000000033 * Height;
		}
		
		public static bool operator == ( Rectangle lhs, Rectangle rhs ) {
			return lhs.Equals( rhs );
		}
		
		public static bool operator != ( Rectangle lhs, Rectangle rhs ) {
			return !(lhs == rhs);
		}

		public bool Contains(int x, int y) {
			return X <= x && x < X + Width && Y <= y && y < Y + Height;
		}
		
		public bool Contains( Point pt ) {
			return Contains( pt.X, pt.Y );
		}
		
		public bool Contains( Rectangle rect ) {
			return X <= rect.X && rect.X + rect.Width <= X + Width && Y <= rect.Y && rect.Y + rect.Height <= Y + Height;
		}
		
		public override string ToString() {
			return X + ", " + Y + " : " + Width + "," + Height;
		}
	}
	
	public struct Size {
		
		public static readonly Size Empty = default(Size);
		public int Width, Height;

		public bool IsEmpty {
			get { return Width == 0 && Height == 0; }
		}

		public Size( Point point ) {
			Width = point.X; Height = point.Y;
		}
		
		public Size( int width, int height ) {
			Width = width; Height = height;
		}
		
		public static Size Ceiling( SizeF value ) {
			return new Size( (int)Math.Ceiling( value.Width ), (int)Math.Ceiling( value.Height ) );
		}
		
		public override bool Equals( object obj ) {
			return (obj is Size) && Equals( (Size)obj );
		}
		
		public bool Equals( Size other ) {
			return Width == other.Width && Height == other.Height;
		}
		
		public override int GetHashCode() {
			return 1000000007 * Width + 1000000009 * Height;
		}
		
		public static bool operator == ( Size lhs, Size rhs ) {
			return lhs.Width == rhs.Width && lhs.Height == rhs.Height;
		}
		
		public static bool operator != ( Size lhs, Size rhs ) {
			return !(lhs == rhs);
		}
		
		public override string ToString() {
			return Width + "," + Height;
		}
	}
	
	public struct SizeF {
		
		public static readonly SizeF Empty = default(SizeF);
		public float Width, Height;

		public bool IsEmpty {
			get { return Width == 0 && Height == 0; }
		}
		
		public SizeF( float width, float height ) {
			Width = width; Height = height;
		}
		
		public override bool Equals( object obj ) {
			return (obj is SizeF) && Equals( (SizeF)obj );
		}
		
		public bool Equals( SizeF other ) {
			return Width == other.Width && Height == other.Height;
		}
		
		public override int GetHashCode() {
			return 1000000007 * Width.GetHashCode() +
				1000000009 * Height.GetHashCode();
		}
		
		public static bool operator == ( SizeF lhs, SizeF rhs ) {
			return lhs.Width == rhs.Width && lhs.Height == rhs.Height;
		}
		
		public static bool operator != ( SizeF lhs, SizeF rhs ) {
			return !(lhs == rhs);
		}
		
		public override string ToString() {
			return Width + "," + Height;
		}
	}
	
	public struct Point {
		
		public static readonly Point Empty = default(Point);
		public int X, Y;

		public bool IsEmpty {
			get { return X == 0 && Y == 0; }
		}

		public Point( Size size ) {
			X = size.Width; Y = size.Height;
		}
		
		public Point( int x, int y ) {
			X = x; Y = y;
		}
		
		public override bool Equals( object obj ) {
			return (obj is Point) && Equals( (Point)obj );
		}
		
		public bool Equals( Point other ) {
			return X == other.X && Y == other.Y;
		}
		
		public override int GetHashCode() {
			return 1000000007 * X + 1000000009 * Y;
		}
		
		public static bool operator == ( Point lhs, Point rhs ) {
			return lhs.X == rhs.X && lhs.Y == rhs.Y;
		}
		
		public static bool operator != ( Point lhs, Point rhs ) {
			return !(lhs == rhs);
		}
		
		public override string ToString() {
			return X + "," + Y;
		}
	}
}
#endif