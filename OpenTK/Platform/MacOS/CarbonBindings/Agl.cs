//  Created by Erik Ylvisaker on 3/17/08.
//  Copyright 2008. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace OpenTK.Platform.MacOS {
	
	internal enum AglAttribute {
		RGBA = 4,
		DOUBLEBUFFER = 5,
		RED_SIZE = 8,
		GREEN_SIZE = 9,
		BLUE_SIZE = 10,
		ALPHA_SIZE = 11,
		DEPTH_SIZE = 12,
		STENCIL_SIZE = 13,
		FULLSCREEN = 54,
	}
	
	public unsafe static class Agl {
		const string lib = "/System/Library/Frameworks/AGL.framework/Versions/Current/AGL";
		
		internal const int AGL_SWAP_INTERVAL = 222; /* 0 -> Don't sync, n -> Sync every n retrace */
		internal const int AGL_BAD_PIXEL_FORMAT = 0002; /* invalid pixel format */
		
		[DllImport(lib)]
		internal static extern IntPtr aglChoosePixelFormat(ref IntPtr gdevs, int ndev, int[] attribs);
		[DllImport(lib)]
		internal static extern IntPtr aglChoosePixelFormat(IntPtr gdevs, int ndev, int[] attribs);
		[DllImport(lib)]
		internal static extern void aglDestroyPixelFormat(IntPtr pix);
		
		[DllImport(lib)]
		internal static extern IntPtr aglCreateContext(IntPtr pix, IntPtr share);
		[DllImport(lib)]
		internal static extern byte aglDestroyContext(IntPtr ctx);
		[DllImport(lib)]
		internal static extern byte aglUpdateContext(IntPtr ctx);

		[DllImport(lib)]
		internal static extern byte aglSetCurrentContext(IntPtr ctx);
		
		[DllImport(lib)]
		internal static extern byte aglSetDrawable(IntPtr ctx, IntPtr draw);
		[DllImport(lib)]
		internal static extern byte aglSetFullScreen(IntPtr ctx, int width, int height, int freq, int device);
		
		[DllImport(lib)]
		internal static extern void aglSwapBuffers(IntPtr ctx);
		[DllImport(lib)]
		internal static extern byte aglSetInteger(IntPtr ctx, int pname, ref int @params);

		[DllImport(lib)]
		internal static extern int aglGetError();
		[DllImport(lib)]
		static extern IntPtr aglErrorString(int code);
		
		internal static void CheckReturnValue(byte code, string function) {
			if (code != 0) return;
			int errCode = aglGetError();
			if (errCode == 0) return;
			
			string error = new String((sbyte*)aglErrorString(errCode));
			throw new MacOSException((OSStatus)errCode, String.Format(
				"AGL Error from function {0}: {1}  {2}",
				function, errCode, error));
		}
	}
}
