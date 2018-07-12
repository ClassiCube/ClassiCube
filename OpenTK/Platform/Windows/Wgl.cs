using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTK.Platform.Windows {

	[SuppressUnmanagedCodeSecurity]
	internal class Wgl : BindingsBase {

		protected override IntPtr GetAddress(string funcname) {
			return Wgl.wglGetProcAddress(funcname);
		}
		
		internal void LoadEntryPoints() {
			LoadDelegate("wglGetSwapIntervalEXT", out wglGetSwapIntervalEXT);
			LoadDelegate("wglSwapIntervalEXT", out wglSwapIntervalEXT);
		}

		[SuppressUnmanagedCodeSecurity]
		internal delegate Boolean SwapIntervalEXT(int interval);
		internal static SwapIntervalEXT wglSwapIntervalEXT;
		
		[SuppressUnmanagedCodeSecurity]
		internal delegate int GetSwapIntervalEXT();
		internal static GetSwapIntervalEXT wglGetSwapIntervalEXT;
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglCreateContext(IntPtr hDc);
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static Boolean wglDeleteContext(IntPtr oldContext);
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetCurrentContext();
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static Boolean wglMakeCurrent(IntPtr hDc, IntPtr newContext);
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetCurrentDC();
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetProcAddress(String lpszProc);
	}
}
