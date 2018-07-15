#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
#endregion

using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTK.Platform.X11 {
	enum GLXAttribute : int {
		RGBA = 4,
		DOUBLEBUFFER = 5,
		RED_SIZE = 8,
		GREEN_SIZE = 9,
		BLUE_SIZE = 10,
		ALPHA_SIZE = 11,
		DEPTH_SIZE = 12,
		STENCIL_SIZE = 13,
	}

	[SuppressUnmanagedCodeSecurity]
	internal static class Glx {
		
		internal static IntPtr GetAddress(string funcname) {
			return glXGetProcAddress(funcname);
		}
		
		internal static void LoadEntryPoints() {
			IntPtr address = GetAddress("glXSwapIntervalSGI");
			if (address != IntPtr.Zero) {
				glXSwapIntervalSGI = (SwapIntervalSGI)Marshal.GetDelegateForFunctionPointer(
					address, typeof(SwapIntervalSGI));
			}
		}

		const string lib = "libGL.so.1";
		[DllImport(lib)]
		public static extern bool glXIsDirect(IntPtr dpy, IntPtr context);
		[DllImport(lib)]
		public extern static bool glXQueryVersion(IntPtr dpy, ref int major, ref int minor);

		[DllImport(lib)]
		public static extern IntPtr glXCreateContext(IntPtr dpy, ref XVisualInfo vis, IntPtr shareList, bool direct);	
		[DllImport(lib)]
		public static extern void glXDestroyContext(IntPtr dpy, IntPtr context);

		[DllImport(lib)]
		public static extern IntPtr glXGetCurrentContext();
		[DllImport(lib)]
		public static extern bool glXMakeCurrent(IntPtr display, IntPtr drawable, IntPtr context);

		[DllImport(lib)]
		public static extern void glXSwapBuffers(IntPtr display, IntPtr drawable);
		[DllImport(lib)]
		public static extern IntPtr glXGetProcAddress([MarshalAs(UnmanagedType.LPTStr)] string procName);

		[DllImport(lib)]
		public static extern IntPtr glXChooseVisual(IntPtr dpy, int screen, int[] attriblist);
		// Returns an array of GLXFBConfig structures.
		[DllImport(lib)]
		public unsafe extern static IntPtr* glXChooseFBConfig(IntPtr dpy, int screen, int[] attriblist, out int fbount);
		// Returns a pointer to an XVisualInfo structure.
		[DllImport(lib)]
		public unsafe extern static IntPtr glXGetVisualFromFBConfig(IntPtr dpy, IntPtr fbconfig);

		[SuppressUnmanagedCodeSecurity]
		public delegate int SwapIntervalSGI(int interval);
		public static SwapIntervalSGI glXSwapIntervalSGI;
	}
}