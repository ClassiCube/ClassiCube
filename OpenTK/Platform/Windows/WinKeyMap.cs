#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license information
 */
#endregion

using System;
using System.Collections.Generic;
using OpenTK.Input;

namespace OpenTK.Platform.Windows {
	
	internal static class VirtualKeys {
		// Virtual Key, Standard Set
		public const int BACK   = 0x08;
		public const int TAB    = 0x09;
		public const int CLEAR  = 0x0C;
		public const int RETURN = 0x0D;

		public const int SHIFT   = 0x10;
		public const int CONTROL = 0x11;
		public const int MENU    = 0x12;
		public const int PAUSE   = 0x13;
		public const int CAPITAL = 0x14;
		public const int ESCAPE  = 0x1B;
		
		public const int SPACE    = 0x20;
		public const int PRIOR    = 0x21;
		public const int NEXT     = 0x22;
		public const int END      = 0x23;
		public const int HOME     = 0x24;
		public const int LEFT     = 0x25;
		public const int UP       = 0x26;
		public const int RIGHT    = 0x27;
		public const int DOWN     = 0x28;
		public const int PRINT    = 0x2A;
		public const int SNAPSHOT = 0x2C;
		public const int INSERT   = 0x2D;
		public const int DELETE   = 0x2E;
		public const int HELP     = 0x2F;
		
		// 0 - 9 are the same as ASCII '0' - '9' (0x30 - 0x39)
		// A - Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)

		public const int LWIN      = 0x5B;
		public const int RWIN      = 0x5C;
		public const int APPS      = 0x5D;
		public const int SLEEP     = 0x5F;
		public const int NUMPAD0   = 0x60;
		public const int NUMPAD9   = 0x69;
		public const int MULTIPLY  = 0x6A;
		public const int ADD       = 0x6B;
		public const int SEPARATOR = 0x6C;
		public const int SUBTRACT  = 0x6D;
		public const int DECIMAL   = 0x6E;
		public const int DIVIDE    = 0x6F;
		
		public const int F1      = 0x70;
		public const int F24     = 0x87;
		public const int NUMLOCK = 0x90;
		public const int SCROLL  = 0x91;

		/* L* & R* - left and right Alt, Ctrl and Shift virtual keys.
		 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
		 * No other API or message will distinguish left and right keys in this way. */
		public const int LSHIFT   = 0xA0;
		public const int RSHIFT   = 0xA1;
		public const int LCONTROL = 0xA2;
		public const int RCONTROL = 0xA3;
		public const int LMENU    = 0xA4;
		public const int RMENU    = 0xA5;

		public const int OEM_1      = 0xBA;   // ';:' for US
		public const int OEM_PLUS   = 0xBB;   // '+' any country
		public const int OEM_COMMA  = 0xBC;   // ';' any country
		public const int OEM_MINUS  = 0xBD;   // '-' any country
		public const int OEM_PERIOD = 0xBE;   // '.' any country
		public const int OEM_2      = 0xBF;   // '/?' for US
		public const int OEM_3      = 0xC0;   // '`~' for US

		public const int OEM_4 = 0xDB;  //  '[{' for US
		public const int OEM_5 = 0xDC;  //  '\|' for US
		public const int OEM_6 = 0xDD;  //  ']}' for US
		public const int OEM_7 = 0xDE;  //  ''"' for US
		public const int OEM_8 = 0xDF;
	}
	
	internal class WinKeyMap : Dictionary<int, Key> {
		internal WinKeyMap() {
			Add(VirtualKeys.ESCAPE, Key.Escape);

			for (int i = 0; i < 24; i++) {
				Add(VirtualKeys.F1 + i, Key.F1 + i);
			}
			
			for (int i = 0; i < 26; i++) {
				Add('A' + i, Key.A + i);
			}
			
			for (int i = 0; i <= 9; i++) {
				Add('0' + i,                 Key.Number0 + i);
				Add(VirtualKeys.NUMPAD0 + i, Key.Keypad0 + i);
			}

			Add(VirtualKeys.TAB, Key.Tab);
			Add(VirtualKeys.CAPITAL, Key.CapsLock);
			Add(VirtualKeys.LCONTROL, Key.ControlLeft);
			Add(VirtualKeys.LSHIFT, Key.ShiftLeft);
			Add(VirtualKeys.LWIN, Key.WinLeft);
			Add(VirtualKeys.LMENU, Key.AltLeft);
			Add(VirtualKeys.SPACE, Key.Space);
			Add(VirtualKeys.RMENU, Key.AltRight);
			Add(VirtualKeys.RWIN, Key.WinRight);
			Add(VirtualKeys.APPS, Key.Menu);
			Add(VirtualKeys.RCONTROL, Key.ControlRight);
			Add(VirtualKeys.RSHIFT, Key.ShiftRight);
			Add(VirtualKeys.RETURN, Key.Enter);
			Add(VirtualKeys.BACK, Key.BackSpace);

			Add(VirtualKeys.OEM_1, Key.Semicolon);      // Varies by keyboard, ;: on Win2K/US
			Add(VirtualKeys.OEM_2, Key.Slash);          // Varies by keyboard, /? on Win2K/US
			Add(VirtualKeys.OEM_3, Key.Tilde);          // Varies by keyboard, `~ on Win2K/US
			Add(VirtualKeys.OEM_4, Key.BracketLeft);    // Varies by keyboard, [{ on Win2K/US
			Add(VirtualKeys.OEM_5, Key.BackSlash);      // Varies by keyboard, \| on Win2K/US
			Add(VirtualKeys.OEM_6, Key.BracketRight);   // Varies by keyboard, ]} on Win2K/US
			Add(VirtualKeys.OEM_7, Key.Quote);          // Varies by keyboard, '" on Win2K/US
			Add(VirtualKeys.OEM_PLUS, Key.Plus);        // Invariant: +
			Add(VirtualKeys.OEM_COMMA, Key.Comma);      // Invariant: ,
			Add(VirtualKeys.OEM_MINUS, Key.Minus);      // Invariant: -
			Add(VirtualKeys.OEM_PERIOD, Key.Period);    // Invariant: .

			Add(VirtualKeys.HOME, Key.Home);
			Add(VirtualKeys.END, Key.End);
			Add(VirtualKeys.DELETE, Key.Delete);
			Add(VirtualKeys.PRIOR, Key.PageUp);
			Add(VirtualKeys.NEXT, Key.PageDown);
			Add(VirtualKeys.PRINT, Key.PrintScreen);
			Add(VirtualKeys.PAUSE, Key.Pause);
			Add(VirtualKeys.NUMLOCK, Key.NumLock);

			Add(VirtualKeys.SCROLL, Key.ScrollLock);
			Add(VirtualKeys.SNAPSHOT, Key.PrintScreen);
			Add(VirtualKeys.INSERT, Key.Insert);

			// Keypad
			Add(VirtualKeys.DECIMAL, Key.KeypadDecimal);
			Add(VirtualKeys.ADD, Key.KeypadAdd);
			Add(VirtualKeys.SUBTRACT, Key.KeypadSubtract);
			Add(VirtualKeys.DIVIDE, Key.KeypadDivide);
			Add(VirtualKeys.MULTIPLY, Key.KeypadMultiply);

			// Navigation
			Add(VirtualKeys.UP, Key.Up);
			Add(VirtualKeys.DOWN, Key.Down);
			Add(VirtualKeys.LEFT, Key.Left);
			Add(VirtualKeys.RIGHT, Key.Right);
		}
	}
}
