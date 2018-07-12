using System;
using System.Runtime.InteropServices;

namespace OpenTK.Platform.MacOS {
	
	// Core foundation services
	internal class CF {
		const string lib = "/System/Library/Frameworks/ApplicationServices.framework/Versions/Current/ApplicationServices";

		[DllImport(lib)]
		internal static extern int CFArrayGetCount(IntPtr array);
		[DllImport(lib)]
		internal static extern IntPtr CFArrayGetValueAtIndex(IntPtr array, int idx);

		// this mirrors the definition in CFString.h.
		// I don't know why, but __CFStringMakeConstantString is marked as "private and should not be used directly"
		// even though the CFSTR macro just calls it.
		[DllImport(lib)]
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
		[DllImport(lib)]
		internal static extern IntPtr CFDictionaryGetValue(IntPtr dict, IntPtr key);

		[DllImport(lib)]
		internal unsafe static extern bool CFNumberGetValue(IntPtr number, CFNumberType type, void* valuePtr);
		internal enum CFNumberType { kCFNumberDoubleType = 13 };
	}
	
	internal static class CG {
		const string lib = "/System/Library/Frameworks/ApplicationServices.framework/Versions/Current/ApplicationServices";

		// CGPoint -> HIPoint
		// CGSize -> HISize
		// CGRect -> HIRect

		[DllImport(lib)]
		internal unsafe static extern int CGGetActiveDisplayList(int maxDisplays, IntPtr* displays, out int count);
		[DllImport(lib)]
		internal static extern IntPtr CGMainDisplayID();

		[DllImport(lib)]
		internal unsafe static extern HIRect CGDisplayBounds(IntPtr display);
		[DllImport(lib)]
		internal static extern int CGDisplayPixelsWide(IntPtr display);
		[DllImport(lib)]
		internal static extern int CGDisplayPixelsHigh(IntPtr display);

		[DllImport(lib)]
		internal static extern int CGDisplayCapture(IntPtr display);
		[DllImport(lib)]
		internal static extern int CGDisplayRelease(IntPtr display);

		[DllImport(lib)]
		internal static extern IntPtr CGDisplayAvailableModes(IntPtr display);
		[DllImport(lib)]
		internal static extern IntPtr CGDisplayCurrentMode(IntPtr display);
		
		[DllImport(lib)]
		internal static extern int CGDisplayHideCursor(IntPtr display);	
		[DllImport(lib)]
		internal static extern int CGDisplayShowCursor(IntPtr display);

		[DllImport(lib)]
		internal static extern int CGDisplayMoveCursorToPoint(IntPtr display, HIPoint point);	
		[DllImport(lib)]
		internal static extern int CGAssociateMouseAndMouseCursorPosition(int value);
	}
}
