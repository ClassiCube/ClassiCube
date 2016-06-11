using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTK.Platform.Windows {

	internal partial class Wgl : BindingsBase {

		protected override IntPtr GetAddress( string funcname ) {
			return Wgl.wglGetProcAddress( funcname );
		}
		
		internal void LoadEntryPoints() {
			LoadDelegate( "wglGetSwapIntervalEXT", out wglGetSwapIntervalEXT );
			LoadDelegate( "wglSwapIntervalEXT", out wglSwapIntervalEXT );
		}

		[SuppressUnmanagedCodeSecurity]
		internal delegate Boolean SwapIntervalEXT(int interval);
		internal static SwapIntervalEXT wglSwapIntervalEXT;
		
		[SuppressUnmanagedCodeSecurity]
		internal delegate int GetSwapIntervalEXT();
		internal static GetSwapIntervalEXT wglGetSwapIntervalEXT;
		
		[SuppressUnmanagedCodeSecurity,DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglCreateContext(IntPtr hDc);
		
		[SuppressUnmanagedCodeSecurity, DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static Boolean wglDeleteContext(IntPtr oldContext);
		
		[SuppressUnmanagedCodeSecurity, DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetCurrentContext();
		
		[SuppressUnmanagedCodeSecurity, DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static Boolean wglMakeCurrent(IntPtr hDc, IntPtr newContext);
		
		[SuppressUnmanagedCodeSecurity, DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetCurrentDC();
		
		[SuppressUnmanagedCodeSecurity, DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetProcAddress(String lpszProc);
	}
}
