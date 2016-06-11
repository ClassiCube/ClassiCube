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

namespace OpenTK {
	
	/// <summary> The implementation of this class is filled by InteropBuilder post-building-event. </summary>
	public static unsafe class Interop {
		
		static Exception rewriteEx = new NotImplementedException( "You need to run InteropPatcher on this dll." );
		public static IntPtr Fixed<T>( ref T data ) { throw rewriteEx; }
		// Direct3D9 interop definitions
		public static int Calli(IntPtr comPtr, IntPtr methodPtr) { throw rewriteEx; }
		public static void Calli_V(IntPtr comPtr, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, IntPtr methodPtr) { throw rewriteEx; }
		public static IntPtr Calli_IntPtr(IntPtr comPtr, int arg0, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, IntPtr arg0, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, IntPtr arg0, IntPtr arg1, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, IntPtr arg1, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, IntPtr arg2, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, IntPtr arg0, IntPtr arg1, int arg2, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, IntPtr arg3, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, IntPtr arg1, int arg2, int arg3, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, IntPtr arg2, int arg3, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, IntPtr arg1, IntPtr arg2, int arg3, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, IntPtr arg0, IntPtr arg1, IntPtr arg2, IntPtr arg3, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, int arg3, int arg4, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, int arg3, IntPtr arg4, IntPtr arg5, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, IntPtr arg1, int arg2, int arg3, float arg4, int arg5, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, IntPtr arg2, int arg3, IntPtr arg4, IntPtr arg5, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, int arg3, IntPtr arg4, int arg5, IntPtr arg6, int arg7, IntPtr methodPtr) { throw rewriteEx; }
		public static int Calli(IntPtr comPtr, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, IntPtr arg6, IntPtr arg7, IntPtr methodPtr) { throw rewriteEx; }
		// OpenGL interop definitions
		public static void Calli( IntPtr address ) { throw rewriteEx; }
		public static int Calli_Int32( IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, IntPtr address ) { throw rewriteEx; }
		public static IntPtr Calli_IntPtr( int arg0, IntPtr address ) { throw rewriteEx; }
		public static void Calli( float* arg0, IntPtr address ) { throw rewriteEx; }
		public static byte Calli_UInt8( int arg0, IntPtr address ) { throw rewriteEx; }
		public static void Calli( byte arg0, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, float arg1, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int* arg1, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, float* arg1, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, int arg2, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, int arg2, int* arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, int arg2, IntPtr arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, int arg2, int arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( float arg0, float arg1, float arg2, float arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( byte arg0, byte arg1, byte arg2, byte arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, IntPtr arg1, IntPtr arg2, IntPtr arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, IntPtr arg1, IntPtr arg2, int arg3, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, IntPtr arg6, IntPtr address ) { throw rewriteEx; }
		public static void Calli( int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, IntPtr arg8, IntPtr address ) { throw rewriteEx; }
	}
	
	/// <summary> Indicates that the specified type needs to have interop patching performed on it. </summary>
	[AttributeUsage( AttributeTargets.Struct | AttributeTargets.Class )]
	public sealed class InteropPatchAttribute : Attribute {
		
	}
}