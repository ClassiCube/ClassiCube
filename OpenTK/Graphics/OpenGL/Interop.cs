using System;

namespace OpenTK.Graphics.OpenGL {

	internal static unsafe class Interop {
		
		static Exception rewriteEx = new NotImplementedException( "You need to run InteropPatcher on this dll." );
		public static IntPtr Fixed<T>( ref T data ) { throw rewriteEx; }
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
}
