//  Created by Erik Ylvisaker on 3/17/08.
//  Copyright 2008. All rights reserved.

using System;
using System.Runtime.InteropServices;
using AGLContext = System.IntPtr;
using AGLDevice = System.IntPtr;
using AGLDrawable = System.IntPtr;
using AGLPixelFormat = System.IntPtr;

namespace OpenTK.Platform.MacOS {
	
	public unsafe static class Agl {
		const string agl = "/System/Library/Frameworks/AGL.framework/Versions/Current/AGL";
		
		// Attribute names for aglChoosePixelFormat and aglDescribePixelFormat.
		internal enum PixelFormatAttribute {
			AGL_NONE = 0,
			AGL_RGBA = 4,
			AGL_DOUBLEBUFFER = 5,
			AGL_RED_SIZE = 8,
			AGL_GREEN_SIZE = 9,
			AGL_BLUE_SIZE = 10,
			AGL_ALPHA_SIZE = 11,
			AGL_DEPTH_SIZE = 12,
			AGL_STENCIL_SIZE = 13,
			AGL_FULLSCREEN = 54,
		}
		
		internal const int AGL_SWAP_INTERVAL = 222; /* 0 -> Don't sync, n -> Sync every n retrace */
		internal const int AGL_BAD_PIXEL_FORMAT = 0002; /* invalid pixel format */
		
		[DllImport(agl)] 
		internal static extern IntPtr aglChoosePixelFormat(ref IntPtr gdevs, int ndev, int[] attribs);
		[DllImport(agl)] 
		internal static extern IntPtr aglChoosePixelFormat(IntPtr gdevs, int ndev, int[] attribs);
		[DllImport(agl)] 
		internal static extern void aglDestroyPixelFormat(IntPtr pix);
		
		[DllImport(agl)] 
		internal static extern IntPtr aglCreateContext(IntPtr pix, IntPtr share);
		[DllImport(agl)] 
		internal static extern byte aglDestroyContext(IntPtr ctx);
		[DllImport(agl)] 
		internal static extern byte aglUpdateContext(IntPtr ctx);

		[DllImport(agl)] 
		internal static extern byte aglSetCurrentContext(IntPtr ctx);
		[DllImport(agl)] 
		internal static extern IntPtr aglGetCurrentContext();
		
		[DllImport(agl)] 
		internal static extern byte aglSetDrawable(IntPtr ctx, IntPtr draw);
		[DllImport(agl)] 
		internal static extern byte aglSetFullScreen(IntPtr ctx, int width, int height, int freq, int device);
		
		[DllImport(agl)] 
		internal static extern void aglSwapBuffers(IntPtr ctx);	
		[DllImport(agl)]
		internal static extern byte aglSetInteger(IntPtr ctx, int pname, ref int @params);

		[DllImport(agl)] 
		internal static extern int aglGetError();
		[DllImport(agl)] 
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
