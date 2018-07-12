using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace OpenTK.Platform.MacOS {
	
	// Core foundation services
	internal class CF {
		const string appServices = "/System/Library/Frameworks/ApplicationServices.framework/Versions/Current/ApplicationServices";

		[DllImport(appServices)]
		internal static extern int CFArrayGetCount(IntPtr array);

		[DllImport(appServices)]
		internal static extern IntPtr CFArrayGetValueAtIndex(IntPtr array, int idx);

		[DllImport(appServices)]
		internal static extern IntPtr CFDictionaryGetValue(IntPtr dict, IntPtr key);

		// this mirrors the definition in CFString.h.
		// I don't know why, but __CFStringMakeConstantString is marked as "private and should not be used directly"
		// even though the CFSTR macro just calls it.
		[DllImport(appServices)]
		static extern IntPtr __CFStringMakeConstantString(string cStr);
		internal static IntPtr CFSTR(string cStr) {
			return __CFStringMakeConstantString(cStr);
		}
		
		internal static unsafe double DictGetNumber(IntPtr dict, string key) {
			double value;
			IntPtr cfnum = CFDictionaryGetValue(dict, CFSTR(key));
			
			CFNumberGetValue(cfnum, CFNumberType.kCFNumberDoubleType, &value);
			return value;
		}

		[DllImport(appServices)]
		internal unsafe static extern bool CFNumberGetValue(IntPtr number, CFNumberType theType, void* valuePtr);

		internal enum CFNumberType {
			kCFNumberDoubleType = 13,
		};
	}
	
	internal static class CG {
		const string appServices = "/System/Library/Frameworks/ApplicationServices.framework/Versions/Current/ApplicationServices";

		// CGPoint -> HIPoint
		// CGSize -> HISize
		// CGRect -> HIRect

		[DllImport(appServices)]
		internal unsafe static extern int CGGetActiveDisplayList(int maxDisplays, IntPtr* displays, out int count);

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
		internal static extern int CGDisplayCapture(IntPtr display);

		[DllImport(appServices)]
		internal static extern int CGDisplayRelease(IntPtr display);

		[DllImport(appServices)]
		internal static extern IntPtr CGDisplayAvailableModes(IntPtr display);

		[DllImport(appServices)]
		internal static extern IntPtr CGDisplaySwitchToMode(IntPtr display, IntPtr displayMode);
		
		[DllImport(appServices)]
		internal static extern int CGDisplayHideCursor(IntPtr display);
		
		[DllImport(appServices)]
		internal static extern int CGDisplayShowCursor(IntPtr display);

		[DllImport(appServices)]
		internal static extern int CGDisplayMoveCursorToPoint(IntPtr display, HIPoint point);
		
		[DllImport(appServices)]
		internal static extern int CGAssociateMouseAndMouseCursorPosition(int value);
	}
}
