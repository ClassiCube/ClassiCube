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
	public struct Vector3 : IEquatable<Vector3>
	{
		public float X;

		public float Y;

		public float Z;

		public Vector3(float x, float y, float z) {
			X = x; Y = y; Z = z;
		}
		
		public Vector3(float value) {
			X = value; Y = value; Z = value;
		}

		public Vector3(Vector2 v) {
			X = v.X; Y = v.Y; Z = 0.0f;
		}

		public Vector3(Vector3 v) {
			X = v.X; Y = v.Y; Z = v.Z;
		}

		public Vector3(Vector4 v) {
			X = v.X; Y = v.Y; Z = v.Z;
		}

		public float Length {
			get { return (float)Math.Sqrt(X * X + Y * Y + Z * Z); }
		}

		public float LengthSquared {
			get { return X * X + Y * Y + Z * Z; }
		}

		public static readonly Vector3 UnitX = new Vector3(1, 0, 0);

		public static readonly Vector3 UnitY = new Vector3(0, 1, 0);

		public static readonly Vector3 UnitZ = new Vector3(0, 0, 1);

		public static readonly Vector3 Zero = new Vector3(0, 0, 0);

		public static readonly Vector3 One = new Vector3(1, 1, 1);

		public static readonly int SizeInBytes = 3 * sizeof( float );

		public static void Add(ref Vector3 a, ref Vector3 b, out Vector3 result) {
			result = new Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
		}

		public static void Subtract(ref Vector3 a, ref Vector3 b, out Vector3 result) {
			result = new Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
		}

		public static void Multiply(ref Vector3 vector, float scale, out Vector3 result) {
			result = new Vector3(vector.X * scale, vector.Y * scale, vector.Z * scale);
		}

		public static void Multiply(ref Vector3 vector, ref Vector3 scale, out Vector3 result) {
			result = new Vector3(vector.X * scale.X, vector.Y * scale.Y, vector.Z * scale.Z);
		}

		public static void Divide(ref Vector3 vector, float scale, out Vector3 result) {
			float invScale = 1 / scale;
			result = new Vector3(vector.X * invScale, vector.Y * invScale, vector.Z * invScale);
		}

		public static void Divide(ref Vector3 vector, ref Vector3 scale, out Vector3 result) {
			result = new Vector3(vector.X / scale.X, vector.Y / scale.Y, vector.Z / scale.Z);
		}
		
		public static Vector3 Lerp(Vector3 a, Vector3 b, float blend) {
			a.X = blend * (b.X - a.X) + a.X;
			a.Y = blend * (b.Y - a.Y) + a.Y;
			a.Z = blend * (b.Z - a.Z) + a.Z;
			return a;
		}

		public static void Lerp(ref Vector3 a, ref Vector3 b, float blend, out Vector3 result) {
			result.X = blend * (b.X - a.X) + a.X;
			result.Y = blend * (b.Y - a.Y) + a.Y;
			result.Z = blend * (b.Z - a.Z) + a.Z;
		}

		public static float Dot(Vector3 left, Vector3 right) {
			return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
		}

		public static void Dot(ref Vector3 left, ref Vector3 right, out float result) {
			result = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
		}
		
		public static Vector3 Cross(Vector3 left, Vector3 right) {
			return new Vector3(left.Y * right.Z - left.Z * right.Y,
			                   left.Z * right.X - left.X * right.Z,
			                   left.X * right.Y - left.Y * right.X);
		}

		public static void Cross(ref Vector3 left, ref Vector3 right, out Vector3 result) {
			result = new Vector3(left.Y * right.Z - left.Z * right.Y,
			                     left.Z * right.X - left.X * right.Z,
			                     left.X * right.Y - left.Y * right.X);
		}
		
		public static Vector3 Normalize(Vector3 vec) {
			float scale = 1f / vec.Length;
			vec.X *= scale;
			vec.Y *= scale;
			vec.Z *= scale;
			return vec;
		}
		
		public static Vector3 Normalize(float x, float y, float z) {
			float scale = 1f / (float)Math.Sqrt( x * x + y * y + z * z );
			return new Vector3( x * scale, y * scale, z * scale );
		}

		public static void Normalize(ref Vector3 vec, out Vector3 result) {
			float scale = 1f / vec.Length;
			result.X = vec.X * scale;
			result.Y = vec.Y * scale;
			result.Z = vec.Z * scale;
		}
		
		public static void Transform(ref Vector3 vec, ref Matrix4 mat, out Vector3 result) {
			result.X = vec.X * mat.Row0.X + vec.Y * mat.Row1.X + vec.Z * mat.Row2.X + mat.Row3.X;
			result.Y = vec.X * mat.Row0.Y + vec.Y * mat.Row1.Y + vec.Z * mat.Row2.Y + mat.Row3.Y;
			result.Z = vec.X * mat.Row0.Z + vec.Y * mat.Row1.Z + vec.Z * mat.Row2.Z + mat.Row3.Z;
		}
		
		public static void TransformX(float x, ref Matrix4 mat, out Vector3 result) {
			result.X = x * mat.Row0.X + mat.Row3.X;
			result.Y = x * mat.Row0.Y + mat.Row3.Y;
			result.Z = x * mat.Row0.Z + mat.Row3.Z;
		}
		
		public static void TransformY(float y, ref Matrix4 mat, out Vector3 result) {
			result.X = y * mat.Row1.X + mat.Row3.X;
			result.Y = y * mat.Row1.Y + mat.Row3.Y;
			result.Z = y * mat.Row1.Z + mat.Row3.Z;
		}
		
		public static void TransformZ(float z, ref Matrix4 mat, out Vector3 result) {
			result.X = z * mat.Row2.X + mat.Row3.X;
			result.Y = z * mat.Row2.Y + mat.Row3.Y;
			result.Z = z * mat.Row2.Z + mat.Row3.Z;
		}

		public static Vector3 operator + (Vector3 left, Vector3 right) {
			left.X += right.X;
			left.Y += right.Y;
			left.Z += right.Z;
			return left;
		}

		public static Vector3 operator - (Vector3 left, Vector3 right) {
			left.X -= right.X;
			left.Y -= right.Y;
			left.Z -= right.Z;
			return left;
		}

		public static Vector3 operator - (Vector3 vec) {
			vec.X = -vec.X;
			vec.Y = -vec.Y;
			vec.Z = -vec.Z;
			return vec;
		}

		public static Vector3 operator * (Vector3 vec, float scale) {
			vec.X *= scale;
			vec.Y *= scale;
			vec.Z *= scale;
			return vec;
		}
		
		public static Vector3 operator * (Vector3 vec, Vector3 scale) {
			vec.X *= scale.X;
			vec.Y *= scale.Y;
			vec.Z *= scale.Z;
			return vec;
		}

		public static Vector3 operator / (Vector3 vec, float scale) {
			float mult = 1f / scale;
			vec.X *= mult;
			vec.Y *= mult;
			vec.Z *= mult;
			return vec;
		}

		public static bool operator ==(Vector3 left, Vector3 right) {
			return left.Equals(right);
		}

		public static bool operator !=(Vector3 left, Vector3 right) {
			return !left.Equals(right);
		}

		public override int GetHashCode() {
			return X.GetHashCode() ^ Y.GetHashCode() ^ Z.GetHashCode();
		}

		public override bool Equals(object obj) {
			return (obj is Vector3) && Equals((Vector3)obj);
		}

		public bool Equals(Vector3 other) {
			return X == other.X && Y == other.Y && Z == other.Z;
		}
	}
}
