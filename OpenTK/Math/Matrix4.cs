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
	public struct Matrix4 : IEquatable<Matrix4> {
		
		/// <summary> Top row of the matrix </summary>
		public Vector4 Row0;
		/// <summary> 2nd row of the matrix </summary>
		public Vector4 Row1;
		/// <summary> 3rd row of the matrix </summary>
		public Vector4 Row2;
		/// <summary> Bottom row of the matrix </summary>
		public Vector4 Row3;
		
		/// <summary> The identity matrix </summary>
		public static Matrix4 Identity = new Matrix4(Vector4.UnitX, Vector4.UnitY, Vector4.UnitZ, Vector4.UnitW);

		public Matrix4(Vector4 row0, Vector4 row1, Vector4 row2, Vector4 row3) {
			Row0 = row0;
			Row1 = row1;
			Row2 = row2;
			Row3 = row3;
		}
		
		public Matrix4(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33) {
			Row0 = new Vector4(m00, m01, m02, m03);
			Row1 = new Vector4(m10, m11, m12, m13);
			Row2 = new Vector4(m20, m21, m22, m23);
			Row3 = new Vector4(m30, m31, m32, m33);
		}

		/// <summary> Gets or sets the value at row 1, column 1 of this instance. </summary>
		public float M11 { get { return Row0.X; } set { Row0.X = value; } }

		/// <summary> Gets or sets the value at row 1, column 2 of this instance. </summary>
		public float M12 { get { return Row0.Y; } set { Row0.Y = value; } }

		/// <summary> Gets or sets the value at row 1, column 3 of this instance. </summary>
		public float M13 { get { return Row0.Z; } set { Row0.Z = value; } }

		/// <summary> Gets or sets the value at row 1, column 4 of this instance. </summary>
		public float M14 { get { return Row0.W; } set { Row0.W = value; } }

		/// <summary> Gets or sets the value at row 2, column 1 of this instance. </summary>
		public float M21 { get { return Row1.X; } set { Row1.X = value; } }

		/// <summary> Gets or sets the value at row 2, column 2 of this instance. </summary>
		public float M22 { get { return Row1.Y; } set { Row1.Y = value; } }

		/// <summary> Gets or sets the value at row 2, column 3 of this instance. </summary>
		public float M23 { get { return Row1.Z; } set { Row1.Z = value; } }

		/// <summary> Gets or sets the value at row 2, column 4 of this instance. </summary>
		public float M24 { get { return Row1.W; } set { Row1.W = value; } }

		/// <summary> Gets or sets the value at row 3, column 1 of this instance. </summary>
		public float M31 { get { return Row2.X; } set { Row2.X = value; } }

		/// <summary> Gets or sets the value at row 3, column 2 of this instance. </summary>
		public float M32 { get { return Row2.Y; } set { Row2.Y = value; } }

		/// <summary> Gets or sets the value at row 3, column 3 of this instance. </summary>
		public float M33 { get { return Row2.Z; } set { Row2.Z = value; } }

		/// <summary> Gets or sets the value at row 3, column 4 of this instance. </summary>
		public float M34 { get { return Row2.W; } set { Row2.W = value; } }

		/// <summary> Gets or sets the value at row 4, column 1 of this instance. </summary>
		public float M41 { get { return Row3.X; } set { Row3.X = value; } }

		/// <summary> Gets or sets the value at row 4, column 2 of this instance. </summary>
		public float M42 { get { return Row3.Y; } set { Row3.Y = value; } }

		/// <summary> Gets or sets the value at row 4, column 3 of this instance. </summary>
		public float M43 { get { return Row3.Z; } set { Row3.Z = value; } }

		/// <summary> Gets or sets the value at row 4, column 4 of this instance. </summary>
		public float M44 { get { return Row3.W; } set { Row3.W = value; } }

		public static void RotateX(float angle, out Matrix4 result) {
			float cos = (float)Math.Cos(angle);
			float sin = (float)Math.Sin(angle);

			result.Row0 = Vector4.UnitX;
			result.Row1 = new Vector4(0.0f, cos, sin, 0.0f);
			result.Row2 = new Vector4(0.0f, -sin, cos, 0.0f);
			result.Row3 = Vector4.UnitW;
		}

		public static Matrix4 RotateX(float angle) {
			Matrix4 result;
			RotateX(angle, out result);
			return result;
		}

		public static void RotateY(float angle, out Matrix4 result) {
			float cos = (float)Math.Cos(angle);
			float sin = (float)Math.Sin(angle);

			result.Row0 = new Vector4(cos, 0.0f, -sin, 0.0f);
			result.Row1 = Vector4.UnitY;
			result.Row2 = new Vector4(sin, 0.0f, cos, 0.0f);
			result.Row3 = Vector4.UnitW;
		}

		public static Matrix4 RotateY(float angle) {
			Matrix4 result;
			RotateY(angle, out result);
			return result;
		}

		public static void RotateZ(float angle, out Matrix4 result) {
			float cos = (float)System.Math.Cos(angle);
			float sin = (float)System.Math.Sin(angle);

			result.Row0 = new Vector4(cos, sin, 0.0f, 0.0f);
			result.Row1 = new Vector4(-sin, cos, 0.0f, 0.0f);
			result.Row2 = Vector4.UnitZ;
			result.Row3 = Vector4.UnitW;
		}

		public static Matrix4 RotateZ(float angle) {
			Matrix4 result;
			RotateZ(angle, out result);
			return result;
		}

		public static void Translate(float x, float y, float z, out Matrix4 result) {
			result = Identity;
			result.Row3 = new Vector4(x, y, z, 1);
		}

		public static void Translate(ref Vector3 vector, out Matrix4 result) {
			result = Identity;
			result.Row3 = new Vector4(vector.X, vector.Y, vector.Z, 1);
		}

		public static Matrix4 Translate(float x, float y, float z) {
			Matrix4 result = Identity;
			result.Row3 = new Vector4(x, y, z, 1);
			return result;
		}

		public static Matrix4 Translate(Vector3 vector) {
			Matrix4 result = Identity;
			result.Row3 = new Vector4(vector.X, vector.Y, vector.Z, 1);
			return result;
		}

		public static void CreateOrthographic(float width, float height, float zNear, float zFar, out Matrix4 result) {
			CreateOrthographicOffCenter(-width / 2, width / 2, -height / 2, height / 2, zNear, zFar, out result);
		}
		
		public static Matrix4 CreateOrthographic(float width, float height, float zNear, float zFar) {
			Matrix4 result;
			CreateOrthographicOffCenter(-width / 2, width / 2, -height / 2, height / 2, zNear, zFar, out result);
			return result;
		}

		public static void CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNear, float zFar, out Matrix4 result) {
			result = new Matrix4();

			float invRL = 1 / (right - left);
			float invTB = 1 / (top - bottom);
			float invFN = 1 / (zFar - zNear);

			result.M11 = 2 * invRL;
			result.M22 = 2 * invTB;
			result.M33 = -2 * invFN;

			result.M41 = -(right + left) * invRL;
			result.M42 = -(top + bottom) * invTB;
			result.M43 = -(zFar + zNear) * invFN;
			result.M44 = 1;
		}

		public static Matrix4 CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNear, float zFar) {
			Matrix4 result;
			CreateOrthographicOffCenter(left, right, bottom, top, zNear, zFar, out result);
			return result;
		}
		
		public static void CreatePerspectiveFieldOfView(float fovy, float aspect, float zNear, float zFar, out Matrix4 result) {
			if (fovy <= 0 || fovy > Math.PI)
				throw new ArgumentOutOfRangeException("fovy");
			if (aspect <= 0)
				throw new ArgumentOutOfRangeException("aspect");
			if (zNear <= 0)
				throw new ArgumentOutOfRangeException("zNear");
			if (zFar <= 0)
				throw new ArgumentOutOfRangeException("zFar");
			if (zNear >= zFar)
				throw new ArgumentOutOfRangeException("zNear");
			
			float yMax = zNear * (float)System.Math.Tan(0.5f * fovy);
			float yMin = -yMax;
			float xMin = yMin * aspect;
			float xMax = yMax * aspect;

			CreatePerspectiveOffCenter(xMin, xMax, yMin, yMax, zNear, zFar, out result);
		}

		public static Matrix4 CreatePerspectiveFieldOfView(float fovy, float aspect, float zNear, float zFar) {
			Matrix4 result;
			CreatePerspectiveFieldOfView(fovy, aspect, zNear, zFar, out result);
			return result;
		}
		
		public static void CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float zNear, float zFar, out Matrix4 result) {
			if (zNear <= 0)
				throw new ArgumentOutOfRangeException("zNear");
			if (zFar <= 0)
				throw new ArgumentOutOfRangeException("zFar");
			if (zNear >= zFar)
				throw new ArgumentOutOfRangeException("zNear");
			
			float x = (2.0f * zNear) / (right - left);
			float y = (2.0f * zNear) / (top - bottom);
			float a = (right + left) / (right - left);
			float b = (top + bottom) / (top - bottom);
			float c = -(zFar + zNear) / (zFar - zNear);
			float d = -(2.0f * zFar * zNear) / (zFar - zNear);
			
			result = new Matrix4(x, 0, 0,  0,
			                     0, y, 0,  0,
			                     a, b, c, -1,
			                     0, 0, d,  0);
		}
		
		public static Matrix4 CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float zNear, float zFar) {
			Matrix4 result;
			CreatePerspectiveOffCenter(left, right, bottom, top, zNear, zFar, out result);
			return result;
		}

		public static Matrix4 Scale(float scale) {
			return Scale(scale, scale, scale);
		}

		public static Matrix4 Scale(Vector3 scale) {
			return Scale(scale.X, scale.Y, scale.Z);
		}

		public static Matrix4 Scale(float x, float y, float z) {
			Matrix4 result;
			result.Row0 = Vector4.UnitX * x;
			result.Row1 = Vector4.UnitY * y;
			result.Row2 = Vector4.UnitZ * z;
			result.Row3 = Vector4.UnitW;
			return result;
		}

		public static Matrix4 LookAt(Vector3 eye, Vector3 target, Vector3 up) {
			Vector3 z = Vector3.Normalize(eye - target);
			Vector3 x = Vector3.Normalize(Vector3.Cross(up, z));
			Vector3 y = Vector3.Normalize(Vector3.Cross(z, x));

			Matrix4 rot = new Matrix4(new Vector4(x.X, y.X, z.X, 0.0f),
			                          new Vector4(x.Y, y.Y, z.Y, 0.0f),
			                          new Vector4(x.Z, y.Z, z.Z, 0.0f),
			                          Vector4.UnitW);

			Matrix4 trans = Matrix4.Translate(-eye);

			return trans * rot;
		}
		
		public static Matrix4 Mult(Matrix4 left, Matrix4 right) {
			Matrix4 result;
			Mult(ref left, ref right, out result);
			return result;
		}

		public static void Mult(ref Matrix4 left, ref Matrix4 right, out Matrix4 result) {
			// Originally from http://www.edais.co.uk/blog/?p=27
			float lM11 = left.Row0.X, lM12 = left.Row0.Y, lM13 = left.Row0.Z, lM14 = left.Row0.W,
			lM21 = left.Row1.X, lM22 = left.Row1.Y, lM23 = left.Row1.Z, lM24 = left.Row1.W,
			lM31 = left.Row2.X, lM32 = left.Row2.Y, lM33 = left.Row2.Z, lM34 = left.Row2.W,
			lM41 = left.Row3.X, lM42 = left.Row3.Y, lM43 = left.Row3.Z, lM44 = left.Row3.W,
			
			rM11 = right.Row0.X, rM12 = right.Row0.Y, rM13 = right.Row0.Z, rM14 = right.Row0.W,
			rM21 = right.Row1.X, rM22 = right.Row1.Y, rM23 = right.Row1.Z, rM24 = right.Row1.W,
			rM31 = right.Row2.X, rM32 = right.Row2.Y, rM33 = right.Row2.Z, rM34 = right.Row2.W,
			rM41 = right.Row3.X, rM42 = right.Row3.Y, rM43 = right.Row3.Z, rM44 = right.Row3.W;
			
			result.Row0.X = (((lM11 * rM11) + (lM12 * rM21)) + (lM13 * rM31)) + (lM14 * rM41);
			result.Row0.Y = (((lM11 * rM12) + (lM12 * rM22)) + (lM13 * rM32)) + (lM14 * rM42);
			result.Row0.Z = (((lM11 * rM13) + (lM12 * rM23)) + (lM13 * rM33)) + (lM14 * rM43);
			result.Row0.W = (((lM11 * rM14) + (lM12 * rM24)) + (lM13 * rM34)) + (lM14 * rM44);
			
			result.Row1.X = (((lM21 * rM11) + (lM22 * rM21)) + (lM23 * rM31)) + (lM24 * rM41);
			result.Row1.Y = (((lM21 * rM12) + (lM22 * rM22)) + (lM23 * rM32)) + (lM24 * rM42);
			result.Row1.Z = (((lM21 * rM13) + (lM22 * rM23)) + (lM23 * rM33)) + (lM24 * rM43);
			result.Row1.W = (((lM21 * rM14) + (lM22 * rM24)) + (lM23 * rM34)) + (lM24 * rM44);
			
			result.Row2.X = (((lM31 * rM11) + (lM32 * rM21)) + (lM33 * rM31)) + (lM34 * rM41);
			result.Row2.Y = (((lM31 * rM12) + (lM32 * rM22)) + (lM33 * rM32)) + (lM34 * rM42);
			result.Row2.Z = (((lM31 * rM13) + (lM32 * rM23)) + (lM33 * rM33)) + (lM34 * rM43);
			result.Row2.W = (((lM31 * rM14) + (lM32 * rM24)) + (lM33 * rM34)) + (lM34 * rM44);
			
			result.Row3.X = (((lM41 * rM11) + (lM42 * rM21)) + (lM43 * rM31)) + (lM44 * rM41);
			result.Row3.Y = (((lM41 * rM12) + (lM42 * rM22)) + (lM43 * rM32)) + (lM44 * rM42);
			result.Row3.Z = (((lM41 * rM13) + (lM42 * rM23)) + (lM43 * rM33)) + (lM44 * rM43);
			result.Row3.W = (((lM41 * rM14) + (lM42 * rM24)) + (lM43 * rM34)) + (lM44 * rM44);
		}

		public static Matrix4 operator * (Matrix4 left, Matrix4 right) {
			return Matrix4.Mult(left, right);
		}

		public static bool operator == (Matrix4 left, Matrix4 right) {
			return left.Equals(right);
		}
		
		public static bool operator != (Matrix4 left, Matrix4 right) {
			return !left.Equals(right);
		}

		public override string ToString() {
			return String.Format("{0}\n{1}\n{2}\n{3}", Row0, Row1, Row2, Row3);
		}

		public override int GetHashCode() {
			return Row0.GetHashCode() ^ Row1.GetHashCode() ^ Row2.GetHashCode() ^ Row3.GetHashCode();
		}
		
		public override bool Equals(object obj) {
			return (obj is Matrix4) && Equals((Matrix4)obj);
		}

		public bool Equals(Matrix4 other) {
			return Row0 == other.Row0 && Row1 == other.Row1 &&
				Row2 == other.Row2 && Row3 == other.Row3;
		}
	}
}
