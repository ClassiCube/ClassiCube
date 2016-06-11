using System;
using System.Runtime.InteropServices;

namespace OpenTK.Platform.MacOS.Carbon {
	
	// Quartz Display services used here are available in MacOS X 10.3 and later.
	enum CGDisplayErr {

	}

	internal static class CG {
		const string appServices = "/System/Library/Frameworks/ApplicationServices.framework/Versions/Current/ApplicationServices";

		// CGPoint -> HIPoint
		// CGSize -> HISize
		// CGRect -> HIRect

		[DllImport(appServices)]
		internal unsafe static extern CGDisplayErr CGGetActiveDisplayList(int maxDisplays, IntPtr* activeDspys, out int dspyCnt);

		[DllImport(appServices)]
		internal static extern IntPtr CGMainDisplayID();

		[DllImport(appServices)]
		internal unsafe static extern HIRect CGDisplayBounds(IntPtr display);

		[DllImport(appServices)]
		internal static extern int CGDisplayPixelsWide(IntPtr display);

		[DllImport(appServices)]
		internal static extern int CGDisplayPixelsHigh(IntPtr display);

		[DllImport(appServices)]
		internal static extern IntPtr CGDisplayCurrentMode(IntPtr display);

		[DllImport(appServices)]
		internal static extern CGDisplayErr CGDisplayCapture(IntPtr display);

		[DllImport(appServices)]
		internal static extern CGDisplayErr CGDisplayRelease(IntPtr display);

		[DllImport(appServices)]
		internal static extern IntPtr CGDisplayAvailableModes(IntPtr display);

		[DllImport(appServices)]
		internal static extern IntPtr CGDisplaySwitchToMode(IntPtr display, IntPtr displayMode);
		
		[DllImport(appServices)]
		internal static extern CGDisplayErr CGDisplayHideCursor(IntPtr display);
		
		[DllImport(appServices)]
		internal static extern CGDisplayErr CGDisplayShowCursor(IntPtr display);

		[DllImport(appServices)]
		internal static extern CGDisplayErr CGDisplayMoveCursorToPoint(IntPtr display, HIPoint point);
		
		[DllImport(appServices)]
		internal static extern CGDisplayErr CGAssociateMouseAndMouseCursorPosition(int value);
	}
}
