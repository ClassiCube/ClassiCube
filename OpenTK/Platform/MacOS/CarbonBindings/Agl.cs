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
		
		// Integer parameter names
		internal enum ParameterNames {
			AGL_SWAP_INTERVAL = 222,  /* 0 -> Don't sync, n -> Sync every n retrace    */
		}

		// Error return values from aglGetError.
		internal enum AglError {
			NoError =                 0, /* no error                        */
			
			BadAttribute =        10000, /* invalid pixel format attribute  */
			BadProperty =         10001, /* invalid renderer property       */
			BadPixelFormat =      10002, /* invalid pixel format            */
			BadRendererInfo =     10003, /* invalid renderer info           */
			BadContext =          10004, /* invalid context                 */
			BadDrawable =         10005, /* invalid drawable                */
			BadGraphicsDevice =   10006, /* invalid graphics device         */
			BadState =            10007, /* invalid context state           */
			BadValue =            10008, /* invalid numerical value         */
			BadMatch =            10009, /* invalid share context           */
			BadEnum =             10010, /* invalid enumerant               */
			BadOffscreen =        10011, /* invalid offscreen drawable      */
			BadFullscreen =       10012, /* invalid offscreen drawable      */
			BadWindow =           10013, /* invalid window                  */
			BadPointer =          10014, /* invalid pointer                 */
			BadModule =           10015, /* invalid code module             */
			BadAlloc =            10016, /* memory allocation failure       */
			BadConnection =       10017, /* invalid CoreGraphics connection */
		}
		
		// Pixel format functions
		[DllImport(agl)] internal static extern AGLPixelFormat aglChoosePixelFormat(ref AGLDevice gdevs, int ndev, int[] attribs);
		[DllImport(agl)] internal static extern AGLPixelFormat aglChoosePixelFormat(IntPtr gdevs, int ndev, int[] attribs);
		[DllImport(agl)] internal static extern void aglDestroyPixelFormat(AGLPixelFormat pix);
		
		// Context functions
		[DllImport(agl)] internal static extern AGLContext aglCreateContext(AGLPixelFormat pix, AGLContext share);
		[DllImport(agl)] internal static extern byte aglDestroyContext(AGLContext ctx);
		[DllImport(agl)] internal static extern byte aglUpdateContext(AGLContext ctx);

		// Current state functions
		[DllImport(agl)] internal static extern byte aglSetCurrentContext(AGLContext ctx);
		[DllImport(agl)] internal static extern AGLContext aglGetCurrentContext();
		
		// Drawable Functions
		[DllImport(agl)] internal static extern byte aglSetDrawable(AGLContext ctx, AGLDrawable draw);
		[DllImport(agl)] internal static extern byte aglSetFullScreen(AGLContext ctx, int width, int height, int freq, int device);
		
		// Virtual screen functions
		[DllImport(agl)] static extern byte aglSetVirtualScreen(AGLContext ctx, int screen);
		[DllImport(agl)] static extern int aglGetVirtualScreen(AGLContext ctx);
		
		// Swap functions
		[DllImport(agl)] internal static extern void aglSwapBuffers(AGLContext ctx);
		
		// Per context options
		[DllImport(agl)] internal static extern byte aglSetInteger(AGLContext ctx, ParameterNames pname, ref int @params);

		// Error functions
		[DllImport(agl)] internal static extern AglError aglGetError();
		[DllImport(agl)] static extern IntPtr aglErrorString(AglError code);
		
		internal static void CheckReturnValue( byte code, string function ) {
			if( code != 0 ) return;
			AglError errCode = aglGetError();
			if( errCode == AglError.NoError ) return;
			
			string error = new String( (sbyte*)aglErrorString( errCode ) );
			throw new MacOSException( (OSStatus)errCode, String.Format(
				"AGL Error from function {0}: {1}  {2}",
				function, errCode, error) );
		}
	}
}
