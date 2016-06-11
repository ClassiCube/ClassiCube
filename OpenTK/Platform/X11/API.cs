#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
#endregion

using System;
using System.Runtime.InteropServices;

#pragma warning disable 3019    // CLS-compliance checking
#pragma warning disable 0649    // struct members not explicitly initialized
#pragma warning disable 0169    // field / method is never used.
#pragma warning disable 0414    // field assigned but never used.

namespace OpenTK.Platform.X11 {
	
	using Window = System.IntPtr;
	using VisualID = System.IntPtr;
	using Display = System.IntPtr;
	using Bool = System.Boolean;
	
	[StructLayout(LayoutKind.Sequential)]
	public struct XVisualInfo {
		public IntPtr Visual;
		public VisualID VisualID;
		public int Screen;
		public int Depth;
		public XVisualClass Class;
		public long RedMask;
		public long GreenMask;
		public long blueMask;
		public int ColormapSize;
		public int BitsPerRgb;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XRRScreenSize {
		internal int Width, Height;
		internal int MWidth, MHeight;
	}

	internal enum ErrorCodes : int {
		Success = 0,
		BadRequest = 1,
		BadValue = 2,
		BadWindow = 3,
		BadPixmap = 4,
		BadAtom = 5,
		BadCursor = 6,
		BadFont = 7,
		BadMatch = 8,
		BadDrawable = 9,
		BadAccess = 10,
		BadAlloc = 11,
		BadColor = 12,
		BadGC = 13,
		BadIDChoice = 14,
		BadName = 15,
		BadLength = 16,
		BadImplementation = 17,
	}

	/// <summary> Defines LATIN-1 and miscellaneous keys. </summary>
	internal enum XKey {
		/* TTY function keys, cleverly chosen to map to ASCII, for convenience of
		 * programming, but could have been arbitrary (at the cost of lookup
		 * tables in client code). */

		BackSpace                   = 0xff08,  /* Back space, back char */
		Tab                         = 0xff09,
		Linefeed                    = 0xff0a,  /* Linefeed, LF */
		Clear                       = 0xff0b,
		Return                      = 0xff0d,  /* Return, enter */
		Pause                       = 0xff13,  /* Pause, hold */
		Scroll_Lock                 = 0xff14,
		Sys_Req                     = 0xff15,
		Escape                      = 0xff1b,
		Delete                      = 0xffff,  /* Delete, rubout */

		/* Cursor control & motion */
		Home                        = 0xff50,
		Left                        = 0xff51,  /* Move left, left arrow */
		Up                          = 0xff52,  /* Move up, up arrow */
		Right                       = 0xff53,  /* Move right, right arrow */
		Down                        = 0xff54,  /* Move down, down arrow */
		Page_Up                     = 0xff55,
		Page_Down                   = 0xff56,
		End                         = 0xff57,  /* EOL */

		/* Misc functions */
		Print                       = 0xff61,
		Insert                      = 0xff63,  /* Insert, insert here */
		Menu                        = 0xff67,
		Break                       = 0xff6b,
		Num_Lock                    = 0xff7f,

		/* Keypad functions, keypad numbers cleverly chosen to map to ASCII */
		KP_Space                    = 0xff80,  /* Space */
		KP_Tab                      = 0xff89,
		KP_Enter                    = 0xff8d,  /* Enter */
		KP_F1                       = 0xff91,  /* PF1, KP_A, ... */
		KP_F2                       = 0xff92,
		KP_F3                       = 0xff93,
		KP_F4                       = 0xff94,
		KP_Home                     = 0xff95,
		KP_Left                     = 0xff96,
		KP_Up                       = 0xff97,
		KP_Right                    = 0xff98,
		KP_Down                     = 0xff99,
		KP_Prior                    = 0xff9a,
		KP_Page_Up                  = 0xff9a,
		KP_Next                     = 0xff9b,
		KP_Page_Down                = 0xff9b,
		KP_End                      = 0xff9c,
		KP_Begin                    = 0xff9d,
		KP_Insert                   = 0xff9e,
		KP_Delete                   = 0xff9f,
		KP_Equal                    = 0xffbd,  /* Equals */
		KP_Multiply                 = 0xffaa,
		KP_Add                      = 0xffab,
		KP_Separator                = 0xffac,  /* Separator, often comma */
		KP_Subtract                 = 0xffad,
		KP_Decimal                  = 0xffae,
		KP_Divide                   = 0xffaf,

		KP_0                        = 0xffb0,
		KP_1                        = 0xffb1,
		KP_2                        = 0xffb2,
		KP_3                        = 0xffb3,
		KP_4                        = 0xffb4,
		KP_5                        = 0xffb5,
		KP_6                        = 0xffb6,
		KP_7                        = 0xffb7,
		KP_8                        = 0xffb8,
		KP_9                        = 0xffb9,

		/*
		 * Auxiliary functions; note the duplicate definitions for left and right
		 * function keys;  Sun keyboards and a few other manufacturers have such
		 * function key groups on the left and/or right sides of the keyboard.
		 * We've not found a keyboard with more than 35 function keys total.
		 */
		F1                          = 0xffbe,
		F35                         = 0xffe0,

		/* Modifiers */

		Shift_L                     = 0xffe1,  /* Left shift */
		Shift_R                     = 0xffe2,  /* Right shift */
		Control_L                   = 0xffe3,  /* Left control */
		Control_R                   = 0xffe4,  /* Right control */
		Caps_Lock                   = 0xffe5,  /* Caps lock */
		Shift_Lock                  = 0xffe6,  /* Shift lock */

		Meta_L                      = 0xffe7,  /* Left meta */
		Meta_R                      = 0xffe8,  /* Right meta */
		Alt_L                       = 0xffe9,  /* Left alt */
		Alt_R                       = 0xffea,  /* Right alt */
		Super_L                     = 0xffeb,  /* Left super */
		Super_R                     = 0xffec,  /* Right super */

		/*
		 * Latin 1
		 * (ISO/IEC 8859-1 = Unicode U+0020..U+00FF)
		 * Byte 3 = 0
		 */

		space                       = 0x0020,  /* U+0020 SPACE */
		exclam                      = 0x0021,  /* U+0021 EXCLAMATION MARK */
		quotedbl                    = 0x0022,  /* U+0022 QUOTATION MARK */
		numbersign                  = 0x0023,  /* U+0023 NUMBER SIGN */
		dollar                      = 0x0024,  /* U+0024 DOLLAR SIGN */
		percent                     = 0x0025,  /* U+0025 PERCENT SIGN */
		ampersand                   = 0x0026,  /* U+0026 AMPERSAND */
		apostrophe                  = 0x0027,  /* U+0027 APOSTROPHE */
		quoteright                  = 0x0027,  /* deprecated */
		parenleft                   = 0x0028,  /* U+0028 LEFT PARENTHESIS */
		parenright                  = 0x0029,  /* U+0029 RIGHT PARENTHESIS */
		asterisk                    = 0x002a,  /* U+002A ASTERISK */
		plus                        = 0x002b,  /* U+002B PLUS SIGN */
		comma                       = 0x002c,  /* U+002C COMMA */
		minus                       = 0x002d,  /* U+002D HYPHEN-MINUS */
		period                      = 0x002e,  /* U+002E FULL STOP */
		slash                       = 0x002f,  /* U+002F SOLIDUS */
		Number0                     = 0x0030,  /* U+0030 DIGIT ZERO */
		Number1                     = 0x0031,  /* U+0031 DIGIT ONE */
		Number2                     = 0x0032,  /* U+0032 DIGIT TWO */
		Number3                     = 0x0033,  /* U+0033 DIGIT THREE */
		Number4                     = 0x0034,  /* U+0034 DIGIT FOUR */
		Number5                     = 0x0035,  /* U+0035 DIGIT FIVE */
		Number6                     = 0x0036,  /* U+0036 DIGIT SIX */
		Number7                     = 0x0037,  /* U+0037 DIGIT SEVEN */
		Number8                     = 0x0038,  /* U+0038 DIGIT EIGHT */
		Number9                     = 0x0039,  /* U+0039 DIGIT NINE */
		colon                       = 0x003a,  /* U+003A COLON */
		semicolon                   = 0x003b,  /* U+003B SEMICOLON */
		less                        = 0x003c,  /* U+003C LESS-THAN SIGN */
		equal                       = 0x003d,  /* U+003D EQUALS SIGN */
		greater                     = 0x003e,  /* U+003E GREATER-THAN SIGN */
		question                    = 0x003f,  /* U+003F QUESTION MARK */
		at                          = 0x0040,  /* U+0040 COMMERCIAL AT */
		A                           = 0x0041,  /* U+0041 LATIN CAPITAL LETTER A */
		Z                           = 0x005a,  /* U+005A LATIN CAPITAL LETTER Z */
		bracketleft                 = 0x005b,  /* U+005B LEFT SQUARE BRACKET */
		backslash                   = 0x005c,  /* U+005C REVERSE SOLIDUS */
		bracketright                = 0x005d,  /* U+005D RIGHT SQUARE BRACKET */
		asciicircum                 = 0x005e,  /* U+005E CIRCUMFLEX ACCENT */
		underscore                  = 0x005f,  /* U+005F LOW LINE */
		grave                       = 0x0060,  /* U+0060 GRAVE ACCENT */
		quoteleft                   = 0x0060,  /* deprecated */
		a                           = 0x0061,  /* U+0061 LATIN SMALL LETTER A */
		z                           = 0x007a,  /* U+007A LATIN SMALL LETTER Z */
		braceleft                   = 0x007b,  /* U+007B LEFT CURLY BRACKET */
		bar                         = 0x007c,  /* U+007C VERTICAL LINE */
		braceright                  = 0x007d,  /* U+007D RIGHT CURLY BRACKET */
		asciitilde                  = 0x007e,  /* U+007E TILDE */
	}

	public enum XVisualClass : int {
		StaticGray = 0,
		GrayScale = 1,
		StaticColor = 2,
		PseudoColor = 3,
		TrueColor = 4,
		DirectColor = 5,
	}
}

#pragma warning restore 3019
#pragma warning restore 0649
#pragma warning restore 0169
#pragma warning restore 0414