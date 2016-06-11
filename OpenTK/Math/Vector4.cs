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
	public struct Vector4 : IEquatable<Vector4> {

		public float X;

		public float Y;

		public float Z;

		public float W;

		public static Vector4 UnitX = new Vector4(1, 0, 0, 0);

		public static Vector4 UnitY = new Vector4(0, 1, 0, 0);

		public static Vector4 UnitZ = new Vector4(0, 0, 1, 0);

		public static Vector4 UnitW = new Vector4(0, 0, 0, 1);

		public static Vector4 Zero = new Vector4(0, 0, 0, 0);

		public static readonly Vector4 One = new Vector4(1, 1, 1, 1);

		public static readonly int SizeInBytes = 4 * sizeof( float );


		public Vector4(float x, float y, float z, float w) {
			X = x;
			Y = y;
			Z = z;
			W = w;
		}

		public Vector4(Vector2 v) {
			X = v.X;
			Y = v.Y;
			Z = 0.0f;
			W = 0.0f;
		}

		public Vector4(Vector3 v) {
			X = v.X;
			Y = v.Y;
			Z = v.Z;
			W = 0.0f;
		}

		public Vector4(Vector3 v, float w) {
			X = v.X;
			Y = v.Y;
			Z = v.Z;
			W = w;
		}

		public Vector4(Vector4 v) {
			X = v.X;
			Y = v.Y;
			Z = v.Z;
			W = v.W;
		}

		public float Length {
			get { return (float)Math.Sqrt(X * X + Y * Y + Z * Z + W * W); }
		}

		public float LengthSquared {
			get { return X * X + Y * Y + Z * Z + W * W; }
		}

		public static void Add(ref Vector4 a, ref Vector4 b, out Vector4 result) {
			result = new Vector4(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W);
		}

		public static void Subtract(ref Vector4 a, ref Vector4 b, out Vector4 result) {
			result = new Vector4(a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W);
		}

		public static void Multiply(ref Vector4 vector, float scale, out Vector4 result) {
			result = new Vector4(vector.X * scale, vector.Y * scale, vector.Z * scale, vector.W * scale);
		}

		public static void Multiply(ref Vector4 vector, ref Vector4 scale, out Vector4 result) {
			result = new Vector4(vector.X * scale.X, vector.Y * scale.Y, vector.Z * scale.Z, vector.W * scale.W);
		}

		public static void Divide(ref Vector4 vector, float scale, out Vector4 result) {
			Multiply(ref vector, 1 / scale, out result);
		}

		public static void Divide(ref Vector4 vector, ref Vector4 scale, out Vector4 result) {
			result = new Vector4(vector.X / scale.X, vector.Y / scale.Y, vector.Z / scale.Z, vector.W / scale.W);
		}
		
        public static Vector4 Transform(Vector4 vec, Matrix4 mat) {
            Vector4 result;
            Transform(ref vec, ref mat, out result);
            return result;
        }

        public static void Transform(ref Vector4 vec, ref Matrix4 mat, out Vector4 result) {
            result = new Vector4(
                vec.X * mat.Row0.X + vec.Y * mat.Row1.X + vec.Z * mat.Row2.X + vec.W * mat.Row3.X,
                vec.X * mat.Row0.Y + vec.Y * mat.Row1.Y + vec.Z * mat.Row2.Y + vec.W * mat.Row3.Y,
                vec.X * mat.Row0.Z + vec.Y * mat.Row1.Z + vec.Z * mat.Row2.Z + vec.W * mat.Row3.Z,
                vec.X * mat.Row0.W + vec.Y * mat.Row1.W + vec.Z * mat.Row2.W + vec.W * mat.Row3.W);
        }

		public static Vector4 operator + (Vector4 left, Vector4 right) {
			left.X += right.X;
			left.Y += right.Y;
			left.Z += right.Z;
			left.W += right.W;
			return left;
		}

		public static Vector4 operator - (Vector4 left, Vector4 right) {
			left.X -= right.X;
			left.Y -= right.Y;
			left.Z -= right.Z;
			left.W -= right.W;
			return left;
		}

		public static Vector4 operator - (Vector4 vec) {
			vec.X = -vec.X;
			vec.Y = -vec.Y;
			vec.Z = -vec.Z;
			vec.W = -vec.W;
			return vec;
		}

		public static Vector4 operator * (Vector4 vec, float scale) {
			vec.X *= scale;
			vec.Y *= scale;
			vec.Z *= scale;
			vec.W *= scale;
			return vec;
		}
		
		public static Vector4 operator * (Vector4 vec, Vector4 scale) {
			vec.X *= scale.X;
			vec.Y *= scale.Y;
			vec.Z *= scale.Z;
			vec.W *= scale.W;
			return vec;
		}

		public static Vector4 operator / (Vector4 vec, float scale) {
			float mult = 1.0f / scale;
			vec.X *= mult;
			vec.Y *= mult;
			vec.Z *= mult;
			vec.W *= mult;
			return vec;
		}

		public static bool operator == (Vector4 left, Vector4 right) {
			return left.Equals(right);
		}

		public static bool operator != (Vector4 left, Vector4 right) {
			return !left.Equals(right);
		}

		public override string ToString() {
			return String.Format("({0}, {1}, {2}, {3})", X, Y, Z, W);
		}
		
		public override int GetHashCode() {
			return X.GetHashCode() ^ Y.GetHashCode() ^ Z.GetHashCode() ^ W.GetHashCode();
		}
		
		public override bool Equals(object obj) {
			return (obj is Vector4) && Equals((Vector4)obj);
		}

		public bool Equals(Vector4 other) {
			return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
		}
	}
}
