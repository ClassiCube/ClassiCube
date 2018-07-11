#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK team.
 * This notice may not be removed.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Collections.Generic;
using OpenTK.Input;

namespace OpenTK.Platform.X11
{
	/// <summary> Defines LATIN-1 and miscellaneous keys. </summary>
	internal static class XKey {
		/* TTY function keys, cleverly chosen to map to ASCII, for convenience of
		 * programming, but could have been arbitrary (at the cost of lookup
		 * tables in client code). */

		public const int BackSpace   = 0xff08;  /* Back space, back char */
		public const int Tab         = 0xff09;
		public const int Linefeed    = 0xff0a;  /* Linefeed, LF */
		public const int Clear       = 0xff0b;
		public const int Return      = 0xff0d;  /* Return, enter */
		public const int Pause       = 0xff13;  /* Pause, hold */
		public const int Scroll_Lock = 0xff14;
		public const int Sys_Req     = 0xff15;
		public const int Escape      = 0xff1b;
		public const int Delete      = 0xffff;  /* Delete, rubout */

		/* Cursor control & motion */
		public const int Home      = 0xff50; 
		public const int Left      = 0xff51;  /* Move left, left arrow */
		public const int Up        = 0xff52;  /* Move up, up arrow */
		public const int Right     = 0xff53;  /* Move right, right arrow */
		public const int Down      = 0xff54;  /* Move down, down arrow */
		public const int Page_Up   = 0xff55;
		public const int Page_Down = 0xff56;
		public const int End       = 0xff57;  /* EOL */

		/* Misc functions */
		public const int Print    = 0xff61;
		public const int Insert   = 0xff63;  /* Insert, insert here */
		public const int Menu     = 0xff67;
		public const int Break    = 0xff6b;
		public const int Num_Lock = 0xff7f;

		/* Keypad functions, keypad numbers cleverly chosen to map to ASCII */
		public const int KP_Space     = 0xff80;  /* Space */
		public const int KP_Tab       = 0xff89;
		public const int KP_Enter     = 0xff8d;  /* Enter */
		public const int KP_F1        = 0xff91;  /* PF1, KP_A, ... */
		public const int KP_F2        = 0xff92;
		public const int KP_F3        = 0xff93;
		public const int KP_F4        = 0xff94;
		public const int KP_Home      = 0xff95;
		public const int KP_Left      = 0xff96;
		public const int KP_Up        = 0xff97;
		public const int KP_Right     = 0xff98;
		public const int KP_Down      = 0xff99;
		public const int KP_Page_Up   = 0xff9a;
		public const int KP_Page_Down = 0xff9b;
		public const int KP_End       = 0xff9c;
		public const int KP_Begin     = 0xff9d;
		public const int KP_Insert    = 0xff9e;
		public const int KP_Delete    = 0xff9f;
		
		public const int KP_Equal     = 0xffbd;  /* Equals */
		public const int KP_Multiply  = 0xffaa;
		public const int KP_Add       = 0xffab;
		public const int KP_Separator = 0xffac;  /* Separator, often comma */
		public const int KP_Subtract  = 0xffad;
		public const int KP_Decimal   = 0xffae;
		public const int KP_Divide    = 0xffaf;

		public const int KP_0 = 0xffb0;
		public const int KP_9 = 0xffb9;

		/*
		 * Auxiliary functions; note the duplicate definitions for left and right
		 * function keys;  Sun keyboards and a few other manufacturers have such
		 * function key groups on the left and/or right sides of the keyboard.
		 * We've not found a keyboard with more than 35 function keys total.
		 */
		public const int F1  = 0xffbe;
		public const int F35 = 0xffe0;

		/* Modifiers */

		public const int Shift_L    = 0xffe1;  /* Left shift */
		public const int Shift_R    = 0xffe2;  /* Right shift */
		public const int Control_L  = 0xffe3;  /* Left control */
		public const int Control_R  = 0xffe4;  /* Right control */
		public const int Caps_Lock  = 0xffe5;  /* Caps lock */
		public const int Shift_Lock = 0xffe6;  /* Shift lock */

		public const int Meta_L  = 0xffe7;  /* Left meta */
		public const int Meta_R  = 0xffe8;  /* Right meta */
		public const int Alt_L   = 0xffe9;  /* Left alt */
		public const int Alt_R   = 0xffea;  /* Right alt */
		public const int Super_L = 0xffeb;  /* Left super */
		public const int Super_R = 0xffec;  /* Right super */

		/*
		 * Latin 1
		 * (ISO/IEC 8859-1 = Unicode U+0020..U+00FF)
		 * Byte 3 = 0
		 */

		public const int space        = 0x0020;  /* U+0020 SPACE */
		public const int exclam       = 0x0021;  /* U+0021 EXCLAMATION MARK */
		public const int quotedbl     = 0x0022;  /* U+0022 QUOTATION MARK */
		public const int numbersign   = 0x0023;  /* U+0023 NUMBER SIGN */
		public const int dollar       = 0x0024;  /* U+0024 DOLLAR SIGN */
		public const int percent      = 0x0025;  /* U+0025 PERCENT SIGN */
		public const int ampersand    = 0x0026;  /* U+0026 AMPERSAND */
		public const int apostrophe   = 0x0027;  /* U+0027 APOSTROPHE */
		public const int quoteright   = 0x0027;  /* deprecated */
		public const int parenleft    = 0x0028;  /* U+0028 LEFT PARENTHESIS */
		public const int parenright   = 0x0029;  /* U+0029 RIGHT PARENTHESIS */
		public const int asterisk     = 0x002a;  /* U+002A ASTERISK */
		public const int plus         = 0x002b;  /* U+002B PLUS SIGN */
		public const int comma        = 0x002c;  /* U+002C COMMA */
		public const int minus        = 0x002d;  /* U+002D HYPHEN-MINUS */
		public const int period       = 0x002e;  /* U+002E FULL STOP */
		public const int slash        = 0x002f;  /* U+002F SOLIDUS */
		public const int Number0      = 0x0030;  /* U+0030 DIGIT ZERO */
		public const int Number9      = 0x0039;  /* U+0039 DIGIT NINE */
		public const int colon        = 0x003a;  /* U+003A COLON */
		public const int semicolon    = 0x003b;  /* U+003B SEMICOLON */
		public const int less         = 0x003c;  /* U+003C LESS-THAN SIGN */
		public const int equal        = 0x003d;  /* U+003D EQUALS SIGN */
		public const int greater      = 0x003e;  /* U+003E GREATER-THAN SIGN */
		public const int question     = 0x003f;  /* U+003F QUESTION MARK */
		public const int at           = 0x0040;  /* U+0040 COMMERCIAL AT */
		public const int A            = 0x0041;  /* U+0041 LATIN CAPITAL LETTER A */
		public const int Z            = 0x005a;  /* U+005A LATIN CAPITAL LETTER Z */
		public const int bracketleft  = 0x005b;  /* U+005B LEFT SQUARE BRACKET */
		public const int backslash    = 0x005c;  /* U+005C REVERSE SOLIDUS */
		public const int bracketright = 0x005d;  /* U+005D RIGHT SQUARE BRACKET */
		public const int asciicircum  = 0x005e;  /* U+005E CIRCUMFLEX ACCENT */
		public const int underscore   = 0x005f;  /* U+005F LOW LINE */
		public const int grave        = 0x0060;  /* U+0060 GRAVE ACCENT */
		public const int quoteleft    = 0x0060;  /* deprecated */
		public const int a            = 0x0061;  /* U+0061 LATIN SMALL LETTER A */
		public const int z            = 0x007a;  /* U+007A LATIN SMALL LETTER Z */
		public const int braceleft    = 0x007b;  /* U+007B LEFT CURLY BRACKET */
		public const int bar          = 0x007c;  /* U+007C VERTICAL LINE */
		public const int braceright   = 0x007d;  /* U+007D RIGHT CURLY BRACKET */
		public const int asciitilde   = 0x007e;  /* U+007E TILDE */
	}
	
	internal class X11KeyMap : Dictionary<int, Key> {
		internal X11KeyMap() {
			Add(XKey.Escape, Key.Escape);
			Add(XKey.Return, Key.Enter);
			Add(XKey.space, Key.Space);
			Add(XKey.BackSpace, Key.BackSpace);

			Add(XKey.Shift_L, Key.ShiftLeft);
			Add(XKey.Shift_R, Key.ShiftRight);
			Add(XKey.Alt_L, Key.AltLeft);
			Add(XKey.Alt_R, Key.AltRight);
			Add(XKey.Control_L, Key.ControlLeft);
			Add(XKey.Control_R, Key.ControlRight);
			Add(XKey.Super_L, Key.WinLeft);
			Add(XKey.Super_R, Key.WinRight);
			Add(XKey.Meta_L, Key.WinLeft);
			Add(XKey.Meta_R, Key.WinRight);

			Add(XKey.Menu, Key.Menu);
			Add(XKey.Tab, Key.Tab);
			Add(XKey.minus, Key.Minus);
			Add(XKey.plus, Key.Plus);
			Add(XKey.equal, Key.Plus);

			Add(XKey.Caps_Lock, Key.CapsLock);
			Add(XKey.Num_Lock, Key.NumLock);

			for (int i = 0; i < 35; i++) {
				Add(XKey.F1 + i, Key.F1 + i);
			}

			for (int i = 0; i < 26; i++) {
				Add(XKey.a + i, Key.A + i);
				Add(XKey.A + i, Key.A + i);
			}

			for (int i = 0; i <= 9; i++) {
				Add(XKey.Number0 + i, Key.Number0 + i);
				Add(XKey.KP_0,        Key.Keypad0 + i);
			}

			Add(XKey.Pause, Key.Pause);
			Add(XKey.Break, Key.Pause);
			Add(XKey.Scroll_Lock, Key.Pause);
			Add(XKey.Insert, Key.PrintScreen);
			Add(XKey.Print, Key.PrintScreen);
			Add(XKey.Sys_Req, Key.PrintScreen);

			Add(XKey.backslash, Key.BackSlash);
			Add(XKey.bar, Key.BackSlash);
			Add(XKey.braceleft, Key.BracketLeft);
			Add(XKey.bracketleft, Key.BracketLeft);
			Add(XKey.braceright, Key.BracketRight);
			Add(XKey.bracketright, Key.BracketRight);
			Add(XKey.colon, Key.Semicolon);
			Add(XKey.semicolon, Key.Semicolon);
			Add(XKey.quoteright, Key.Quote);
			Add(XKey.quotedbl, Key.Quote);
			Add(XKey.quoteleft, Key.Tilde);
			Add(XKey.asciitilde, Key.Tilde);

			Add(XKey.comma, Key.Comma);
			Add(XKey.less, Key.Comma);
			Add(XKey.period, Key.Period);
			Add(XKey.greater, Key.Period);
			Add(XKey.slash, Key.Slash);
			Add(XKey.question, Key.Slash);

			Add(XKey.Left, Key.Left);
			Add(XKey.Down, Key.Down);
			Add(XKey.Right, Key.Right);
			Add(XKey.Up, Key.Up);

			Add(XKey.Delete, Key.Delete);
			Add(XKey.Home, Key.Home);
			Add(XKey.End, Key.End);
			//Add(XKey.Prior, Key.PageUp);   // XKey.Prior == XKey.Page_Up
			Add(XKey.Page_Up, Key.PageUp);
			Add(XKey.Page_Down, Key.PageDown);
			//Add(XKey.Next, Key.PageDown);  // XKey.Next == XKey.Page_Down

			Add(XKey.KP_Add, Key.KeypadAdd);
			Add(XKey.KP_Subtract, Key.KeypadSubtract);
			Add(XKey.KP_Multiply, Key.KeypadMultiply);
			Add(XKey.KP_Divide, Key.KeypadDivide);
			Add(XKey.KP_Decimal, Key.KeypadDecimal);
			Add(XKey.KP_Insert, Key.Keypad0);
			Add(XKey.KP_End, Key.Keypad1);
			Add(XKey.KP_Down, Key.Keypad2);
			Add(XKey.KP_Page_Down, Key.Keypad3);
			Add(XKey.KP_Left, Key.Keypad4);
			Add(XKey.KP_Right, Key.Keypad6);
			Add(XKey.KP_Home, Key.Keypad7);
			Add(XKey.KP_Up, Key.Keypad8);
			Add(XKey.KP_Page_Up, Key.Keypad9);
			Add(XKey.KP_Delete, Key.KeypadDecimal);
			Add(XKey.KP_Enter, Key.KeypadEnter);
		}
	}
}
