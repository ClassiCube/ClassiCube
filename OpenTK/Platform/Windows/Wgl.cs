using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTK.Platform.Windows {

	internal partial class Wgl : BindingsBase {

		const string Library = "OPENGL32.DLL";
		static readonly object sync_root = new object();

		protected override IntPtr GetAddress( string funcname ) {
			return Wgl.wglGetProcAddress( funcname );
		}
		
		internal void LoadEntryPoints() {
			lock( sync_root ) {
				LoadDelegate( "wglGetSwapIntervalEXT", out wglGetSwapIntervalEXT );
				LoadDelegate( "wglSwapIntervalEXT", out wglSwapIntervalEXT );
			}
		}

		[SuppressUnmanagedCodeSecurity]
		internal delegate Boolean SwapIntervalEXT(int interval);
		internal static SwapIntervalEXT wglSwapIntervalEXT;
		
		[SuppressUnmanagedCodeSecurity]
		internal delegate int GetSwapIntervalEXT();
		internal static GetSwapIntervalEXT wglGetSwapIntervalEXT;
		
		[SuppressUnmanagedCodeSecurity,DllImport(Library, SetLastError = true)]
		internal extern static IntPtr wglCreateContext(IntPtr hDc);
		
		[SuppressUnmanagedCodeSecurity, DllImport(Library, SetLastError = true)]
		internal extern static Boolean wglDeleteContext(IntPtr oldContext);
		
		[SuppressUnmanagedCodeSecurity, DllImport(Library, SetLastError = true)]
		internal extern static IntPtr wglGetCurrentContext();
		
		[SuppressUnmanagedCodeSecurity, DllImport(Library, SetLastError = true)]
		internal extern static Boolean wglMakeCurrent(IntPtr hDc, IntPtr newContext);
		
		[SuppressUnmanagedCodeSecurity, DllImport(Library, SetLastError = true)]
		internal extern static IntPtr wglGetCurrentDC();
		
		[SuppressUnmanagedCodeSecurity, DllImport(Library, SetLastError = true)]
		internal extern static IntPtr wglGetProcAddress(String lpszProc);
	}
}
