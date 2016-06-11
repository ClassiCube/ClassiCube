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

	partial class Glx : BindingsBase
	{
		const string Library = "libGL.so.1";

		// Disable BeforeFieldInit optimization.
		static Glx() { }

		protected override IntPtr GetAddress(string funcname) {
			return glXGetProcAddress(funcname);
		}
		
		internal void LoadEntryPoints() {
			LoadDelegate( "glXSwapIntervalSGI", out glXSwapIntervalSGI );
		}

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern bool glXIsDirect(IntPtr dpy, IntPtr context);

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern IntPtr glXCreateContext(IntPtr dpy, ref XVisualInfo vis, IntPtr shareList, bool direct);
		
		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern void glXDestroyContext(IntPtr dpy, IntPtr context);

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern IntPtr glXGetCurrentContext();

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern bool glXMakeCurrent(IntPtr display, IntPtr drawable, IntPtr context);

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern void glXSwapBuffers(IntPtr display, IntPtr drawable);

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern IntPtr glXGetProcAddress([MarshalAs(UnmanagedType.LPTStr)] string procName);

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern int glXGetConfig(IntPtr dpy, ref XVisualInfo vis, GLXAttribute attrib, out int value);

		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public static extern IntPtr glXChooseVisual(IntPtr dpy, int screen, int[] attriblist);

		// Returns an array of GLXFBConfig structures.
		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public unsafe extern static IntPtr* glXChooseFBConfig(IntPtr dpy, int screen, int[] attriblist, out int fbount);

		// Returns a pointer to an XVisualInfo structure.
		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public unsafe extern static IntPtr glXGetVisualFromFBConfig(IntPtr dpy, IntPtr fbconfig);
		
		[SuppressUnmanagedCodeSecurity, DllImport( Library )]
		public extern static bool glXQueryVersion(IntPtr dpy, ref int major, ref int minor);

		[SuppressUnmanagedCodeSecurity]
		public delegate GLXErrorCode SwapIntervalSGI(int interval);
		public static SwapIntervalSGI glXSwapIntervalSGI;
	}
}

#pragma warning restore 1591