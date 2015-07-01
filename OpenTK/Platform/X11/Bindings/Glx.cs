#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
#endregion

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Security;

#pragma warning disable 1591

namespace OpenTK.Platform.X11
{
	#region Enums

	enum GLXAttribute : int
	{
		USE_GL = 1,
		RGBA = 4,
		DOUBLEBUFFER = 5,
		RED_SIZE = 8,
		GREEN_SIZE = 9,
		BLUE_SIZE = 10,
		ALPHA_SIZE = 11,
		DEPTH_SIZE = 12,
		STENCIL_SIZE = 13,
	}

	enum GLXSyncType : int
	{
		SYNC_SWAP_SGIX = 0x00000001,
		SYNC_FRAME_SGIX = 0x00000000,
	}

	enum GLXErrorCode : int
	{
		NO_ERROR       = 0,
		BAD_SCREEN     = 1,   /* screen # is bad */
		BAD_ATTRIBUTE  = 2,   /* attribute to get is bad */
		NO_EXTENSION   = 3,   /* no glx extension on server */
		BAD_VISUAL     = 4,   /* visual # not known by GLX */
		BAD_CONTEXT    = 5,   /* returned only by import_context EXT? */
		BAD_VALUE      = 6,   /* returned only by glXSwapIntervalSGI? */
		BAD_ENUM       = 7,   /* unused? */
	}

	#endregion

	partial class Glx
	{

		[DllImport(Library, EntryPoint = "glXIsDirect")]
		public static extern bool IsDirect(IntPtr dpy, IntPtr context);

		[DllImport(Library, EntryPoint = "glXCreateContext")]
		public static extern IntPtr CreateContext(IntPtr dpy, IntPtr vis, IntPtr shareList, bool direct);

		[DllImport(Library, EntryPoint = "glXCreateContext")]
		public static extern IntPtr CreateContext(IntPtr dpy, ref XVisualInfo vis, IntPtr shareList, bool direct);
		
		[DllImport(Library, EntryPoint = "glXDestroyContext")]
		public static extern void DestroyContext(IntPtr dpy, IntPtr context);

		[DllImport(Library, EntryPoint = "glXGetCurrentContext")]
		public static extern IntPtr GetCurrentContext();

		[DllImport(Library, EntryPoint = "glXMakeCurrent")]
		public static extern bool MakeCurrent(IntPtr display, IntPtr drawable, IntPtr context);

		[DllImport(Library, EntryPoint = "glXSwapBuffers")]
		public static extern void SwapBuffers(IntPtr display, IntPtr drawable);

		[DllImport(Library, EntryPoint = "glXGetProcAddress")]
		public static extern IntPtr GetProcAddress([MarshalAs(UnmanagedType.LPTStr)] string procName);

		[DllImport(Library, EntryPoint = "glXGetConfig")]
		public static extern int GetConfig(IntPtr dpy, ref XVisualInfo vis, GLXAttribute attrib, out int value);

		[DllImport(Library, EntryPoint = "glXChooseVisual")]
		public static extern IntPtr ChooseVisual(IntPtr dpy, int screen, int[] attriblist);

		// Returns an array of GLXFBConfig structures.
		[DllImport(Library, EntryPoint = "glXChooseFBConfig")]
		public unsafe extern static IntPtr* ChooseFBConfig(IntPtr dpy, int screen, int[] attriblist, out int fbount);

		// Returns a pointer to an XVisualInfo structure.
		[DllImport(Library, EntryPoint = "glXGetVisualFromFBConfig")]
		public unsafe extern static IntPtr GetVisualFromFBConfig(IntPtr dpy, IntPtr fbconfig);

		public static partial class Imports {
			public static GLXErrorCode SwapInterval(int interval) {
				return (GLXErrorCode)Delegates.glXSwapIntervalSGI(interval);
			}
		}

		internal static partial class Delegates
		{
			[SuppressUnmanagedCodeSecurity]
			public delegate int SwapIntervalSGI(int interval);
			public static SwapIntervalSGI glXSwapIntervalSGI = null;
		}
	}
}

#pragma warning restore 1591