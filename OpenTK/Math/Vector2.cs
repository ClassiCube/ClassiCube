#region --- License ---
/*
Copyright (c) 2006 - 2008 The Open Toolkit library.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#endregion

using System;
using System.Runtime.InteropServices;

namespace OpenTK {
	
	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2 : IEquatable<Vector2>
	{
		public float X;

		public float Y;

		public Vector2(float x, float y) {
			X = x;
			Y = y;
		}
		
		[Obsolete]
		public Vector2(Vector2 v) {
			X = v.X;
			Y = v.Y;
		}

		[Obsolete]
		public Vector2(Vector3 v) {
			X = v.X;
			Y = v.Y;
		}

		[Obsolete]
		public Vector2(Vector4 v) {
			X = v.X;
			Y = v.Y;
		}

		public float Length {
			get { return (float)Math.Sqrt(X * X + Y * Y); }
		}

		public float LengthSquared {
			get { return X * X + Y * Y; }
		}

		public static readonly Vector2 UnitX = new Vector2(1, 0);
		
		public static readonly Vector2 UnitY = new Vector2(0, 1);

		public static readonly Vector2 Zero = new Vector2(0, 0);

		public static readonly Vector2 One = new Vector2(1, 1);

		public const int SizeInBytes = 2 * sizeof( float );

		public static void Add(ref Vector2 a, ref Vector2 b, out Vector2 result) {
			result = new Vector2(a.X + b.X, a.Y + b.Y);
		}

		public static void Subtract(ref Vector2 a, ref Vector2 b, out Vector2 result) {
			result = new Vector2(a.X - b.X, a.Y - b.Y);
		}

		public static void Multiply(ref Vector2 vector, float scale, out Vector2 result) {
			result = new Vector2(vector.X * scale, vector.Y * scale);
		}

		public static void Multiply(ref Vector2 vector, ref Vector2 scale, out Vector2 result) {
			result = new Vector2(vector.X * scale.X, vector.Y * scale.Y);
		}

		public static void Divide(ref Vector2 vector, float scale, out Vector2 result) {
			float invScale = 1 / scale;
			result = new Vector2(vector.X * invScale, vector.Y * invScale);
		}

		public static void Divide(ref Vector2 vector, ref Vector2 scale, out Vector2 result) {
			result = new Vector2(vector.X / scale.X, vector.Y / scale.Y);
		}

		public static Vector2 Lerp(Vector2 a, Vector2 b, float blend) {
			a.X = blend * (b.X - a.X) + a.X;
			a.Y = blend * (b.Y - a.Y) + a.Y;
			return a;
		}
		
		public static void Lerp(ref Vector2 a, ref Vector2 b, float blend, out Vector2 result) {
			result.X = blend * (b.X - a.X) + a.X;
			result.Y = blend * (b.Y - a.Y) + a.Y;
		}

		public static Vector2 operator + (Vector2 left, Vector2 right) {
			left.X += right.X;
			left.Y += right.Y;
			return left;
		}

		public static Vector2 operator - (Vector2 left, Vector2 right) {
			left.X -= right.X;
			left.Y -= right.Y;
			return left;
		}

		public static Vector2 operator - (Vector2 vec) {
			vec.X = -vec.X;
			vec.Y = -vec.Y;
			return vec;
		}

		public static Vector2 operator * (Vector2 vec, float scale) {
			vec.X *= scale;
			vec.Y *= scale;
			return vec;
		}
		
		public static Vector2 operator * (Vector2 vec, Vector2 scale) {
			vec.X *= scale.X;
			vec.Y *= scale.Y;
			return vec;
		}
		
		public static Vector2 operator / (Vector2 vec, float scale) {
			float mult = 1.0f / scale;
			vec.X *= mult;
			vec.Y *= mult;
			return vec;
		}

		public static bool operator == (Vector2 left, Vector2 right) {
			return left.Equals(right);
		}

		public static bool operator != (Vector2 left, Vector2 right) {
			return !left.Equals(right);
		}

		public override string ToString() {
			return String.Format("({0}, {1})", X, Y);
		}

		public override int GetHashCode() {
			return X.GetHashCode() ^ Y.GetHashCode();
		}
		
		public override bool Equals(object obj) {
			return (obj is Vector2) && Equals((Vector2)obj);
		}
		
		public bool Equals(Vector2 other) {
			return X == other.X && Y == other.Y;
		}
	}
}
