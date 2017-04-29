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
