// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

using System;
using System.Globalization;
using System.Runtime.InteropServices;

namespace SharpDX
{
	/// <summary>
	/// Represents a 4x4 mathematical matrix.
	/// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	public struct Matrix
	{
		/// <summary>
		/// Value at row 1 column 1 of the matrix.
		/// </summary>
		public float M11;

		/// <summary>
		/// Value at row 1 column 2 of the matrix.
		/// </summary>
		public float M12;

		/// <summary>
		/// Value at row 1 column 3 of the matrix.
		/// </summary>
		public float M13;

		/// <summary>
		/// Value at row 1 column 4 of the matrix.
		/// </summary>
		public float M14;

		/// <summary>
		/// Value at row 2 column 1 of the matrix.
		/// </summary>
		public float M21;

		/// <summary>
		/// Value at row 2 column 2 of the matrix.
		/// </summary>
		public float M22;

		/// <summary>
		/// Value at row 2 column 3 of the matrix.
		/// </summary>
		public float M23;

		/// <summary>
		/// Value at row 2 column 4 of the matrix.
		/// </summary>
		public float M24;

		/// <summary>
		/// Value at row 3 column 1 of the matrix.
		/// </summary>
		public float M31;

		/// <summary>
		/// Value at row 3 column 2 of the matrix.
		/// </summary>
		public float M32;

		/// <summary>
		/// Value at row 3 column 3 of the matrix.
		/// </summary>
		public float M33;

		/// <summary>
		/// Value at row 3 column 4 of the matrix.
		/// </summary>
		public float M34;

		/// <summary>
		/// Value at row 4 column 1 of the matrix.
		/// </summary>
		public float M41;

		/// <summary>
		/// Value at row 4 column 2 of the matrix.
		/// </summary>
		public float M42;

		/// <summary>
		/// Value at row 4 column 3 of the matrix.
		/// </summary>
		public float M43;

		/// <summary>
		/// Value at row 4 column 4 of the matrix.
		/// </summary>
		public float M44;
		
		public Matrix(float value) {
			M11 = M12 = M13 = M14 =
				M21 = M22 = M23 = M24 =
				M31 = M32 = M33 = M34 =
				M41 = M42 = M43 = M44 = value;
		}
		
		public static Matrix Identity = new Matrix( 1, 0, 0, 0,
		                                           0, 1, 0, 0,
		                                           0, 0, 1, 0,
		                                           0, 0, 0, 1 );

		public Matrix(float m11, float m12, float m13, float m14,
		              float m21, float m22, float m23, float m24,
		              float m31, float m32, float m33, float m34,
		              float m41, float m42, float m43, float m44) {
			M11 = m11; M12 = m12; M13 = m13; M14 = m14;
			M21 = m21; M22 = m22; M23 = m23; M24 = m24;
			M31 = m31; M32 = m32; M33 = m33; M34 = m34;
			M41 = m41; M42 = m42; M43 = m43; M44 = m44;
		}

		public static void Multiply(ref Matrix left, ref Matrix right, out Matrix result) {
			Matrix temp = new Matrix();
			temp.M11 = (left.M11 * right.M11) + (left.M12 * right.M21) + (left.M13 * right.M31) + (left.M14 * right.M41);
			temp.M12 = (left.M11 * right.M12) + (left.M12 * right.M22) + (left.M13 * right.M32) + (left.M14 * right.M42);
			temp.M13 = (left.M11 * right.M13) + (left.M12 * right.M23) + (left.M13 * right.M33) + (left.M14 * right.M43);
			temp.M14 = (left.M11 * right.M14) + (left.M12 * right.M24) + (left.M13 * right.M34) + (left.M14 * right.M44);
			temp.M21 = (left.M21 * right.M11) + (left.M22 * right.M21) + (left.M23 * right.M31) + (left.M24 * right.M41);
			temp.M22 = (left.M21 * right.M12) + (left.M22 * right.M22) + (left.M23 * right.M32) + (left.M24 * right.M42);
			temp.M23 = (left.M21 * right.M13) + (left.M22 * right.M23) + (left.M23 * right.M33) + (left.M24 * right.M43);
			temp.M24 = (left.M21 * right.M14) + (left.M22 * right.M24) + (left.M23 * right.M34) + (left.M24 * right.M44);
			temp.M31 = (left.M31 * right.M11) + (left.M32 * right.M21) + (left.M33 * right.M31) + (left.M34 * right.M41);
			temp.M32 = (left.M31 * right.M12) + (left.M32 * right.M22) + (left.M33 * right.M32) + (left.M34 * right.M42);
			temp.M33 = (left.M31 * right.M13) + (left.M32 * right.M23) + (left.M33 * right.M33) + (left.M34 * right.M43);
			temp.M34 = (left.M31 * right.M14) + (left.M32 * right.M24) + (left.M33 * right.M34) + (left.M34 * right.M44);
			temp.M41 = (left.M41 * right.M11) + (left.M42 * right.M21) + (left.M43 * right.M31) + (left.M44 * right.M41);
			temp.M42 = (left.M41 * right.M12) + (left.M42 * right.M22) + (left.M43 * right.M32) + (left.M44 * right.M42);
			temp.M43 = (left.M41 * right.M13) + (left.M42 * right.M23) + (left.M43 * right.M33) + (left.M44 * right.M43);
			temp.M44 = (left.M41 * right.M14) + (left.M42 * right.M24) + (left.M43 * right.M34) + (left.M44 * right.M44);
			result = temp;
		}

		public static Matrix Multiply(Matrix left, Matrix right) {
			Matrix result;
			Multiply(ref left, ref right, out result);
			return result;
		}

		public static Matrix operator * (Matrix left, Matrix right) {
			Matrix result;
			Multiply(ref left, ref right, out result);
			return result;
		}

		public override string ToString() {
			return string.Format(CultureInfo.CurrentCulture, "[{0} {1} {2} {3}] [{4} {5} {6} {7}] [{8} {9} {10} {11}] [{12} {13} {14} {15}]",
			                     M11, M12, M13, M14, M21, M22, M23, M24, M31, M32, M33, M34, M41, M42, M43, M44);
		}
	}
}
