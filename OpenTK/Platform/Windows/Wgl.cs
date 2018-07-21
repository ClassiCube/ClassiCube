#if !USE_DX
using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTK.Platform.Windows {

	[SuppressUnmanagedCodeSecurity]
	internal static class Wgl {

		static bool IsInvalidAddress(IntPtr address) {
			return address == IntPtr.Zero || address == new IntPtr(1) ||
				address == new IntPtr(2) || address == new IntPtr(-1);
		}
		
		internal static IntPtr GetAddress(string funcname) {
			IntPtr address = wglGetProcAddress(funcname);
			return IsInvalidAddress(address) ? IntPtr.Zero : address;
		}
		
		internal static void LoadEntryPoints() {
			IntPtr address = GetAddress("wglSwapIntervalEXT");
			if (address == IntPtr.Zero) return;
			
			wglSwapIntervalEXT = (SwapIntervalEXT)Marshal.GetDelegateForFunctionPointer(
				address, typeof(SwapIntervalEXT));
		}

		[SuppressUnmanagedCodeSecurity]
		internal delegate Boolean SwapIntervalEXT(int interval);
		internal static SwapIntervalEXT wglSwapIntervalEXT;
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglCreateContext(IntPtr hDc);		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static Boolean wglDeleteContext(IntPtr oldContext);
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static Boolean wglMakeCurrent(IntPtr hDc, IntPtr newContext);		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetCurrentDC();
		
		[DllImport("OPENGL32.DLL", SetLastError = true)]
		internal extern static IntPtr wglGetProcAddress(String lpszProc);
	}
}
#endif
