#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license information
 */
#endregion

using System;
using System.Collections.Generic;
using OpenTK.Input;

namespace OpenTK.Platform.Windows {
	
	internal class WinKeyMap : Dictionary<int, Key> {
		
		/// <summary> Initializes the map between VirtualKeys and OpenTK.Key </summary>
		internal WinKeyMap() {
			AddKey(VirtualKeys.ESCAPE, Key.Escape);

			// Function keys
			for (int i = 0; i < 24; i++) {
				AddKey((VirtualKeys)((int)VirtualKeys.F1 + i), Key.F1 + i);
			}

			// Number keys (0-9)
			for (int i = 0; i <= 9; i++) {
				AddKey((VirtualKeys)('0' + i), Key.Number0 + i);
			}

			// Letters (A-Z)
			for (int i = 0; i < 26; i++) {
				AddKey((VirtualKeys)('A' + i), Key.A + i);
			}

			AddKey(VirtualKeys.TAB, Key.Tab);
			AddKey(VirtualKeys.CAPITAL, Key.CapsLock);
			AddKey(VirtualKeys.LCONTROL, Key.ControlLeft);
			AddKey(VirtualKeys.LSHIFT, Key.ShiftLeft);
			AddKey(VirtualKeys.LWIN, Key.WinLeft);
			AddKey(VirtualKeys.LMENU, Key.AltLeft);
			AddKey(VirtualKeys.SPACE, Key.Space);
			AddKey(VirtualKeys.RMENU, Key.AltRight);
			AddKey(VirtualKeys.RWIN, Key.WinRight);
			AddKey(VirtualKeys.APPS, Key.Menu);
			AddKey(VirtualKeys.RCONTROL, Key.ControlRight);
			AddKey(VirtualKeys.RSHIFT, Key.ShiftRight);
			AddKey(VirtualKeys.RETURN, Key.Enter);
			AddKey(VirtualKeys.BACK, Key.BackSpace);

			AddKey(VirtualKeys.OEM_1, Key.Semicolon);      // Varies by keyboard, ;: on Win2K/US
			AddKey(VirtualKeys.OEM_2, Key.Slash);          // Varies by keyboard, /? on Win2K/US
			AddKey(VirtualKeys.OEM_3, Key.Tilde);          // Varies by keyboard, `~ on Win2K/US
			AddKey(VirtualKeys.OEM_4, Key.BracketLeft);    // Varies by keyboard, [{ on Win2K/US
			AddKey(VirtualKeys.OEM_5, Key.BackSlash);      // Varies by keyboard, \| on Win2K/US
			AddKey(VirtualKeys.OEM_6, Key.BracketRight);   // Varies by keyboard, ]} on Win2K/US
			AddKey(VirtualKeys.OEM_7, Key.Quote);          // Varies by keyboard, '" on Win2K/US
			AddKey(VirtualKeys.OEM_PLUS, Key.Plus);        // Invariant: +
			AddKey(VirtualKeys.OEM_COMMA, Key.Comma);      // Invariant: ,
			AddKey(VirtualKeys.OEM_MINUS, Key.Minus);      // Invariant: -
			AddKey(VirtualKeys.OEM_PERIOD, Key.Period);    // Invariant: .

			AddKey(VirtualKeys.HOME, Key.Home);
			AddKey(VirtualKeys.END, Key.End);
			AddKey(VirtualKeys.DELETE, Key.Delete);
			AddKey(VirtualKeys.PRIOR, Key.PageUp);
			AddKey(VirtualKeys.NEXT, Key.PageDown);
			AddKey(VirtualKeys.PRINT, Key.PrintScreen);
			AddKey(VirtualKeys.PAUSE, Key.Pause);
			AddKey(VirtualKeys.NUMLOCK, Key.NumLock);

			AddKey(VirtualKeys.SCROLL, Key.ScrollLock);
			AddKey(VirtualKeys.SNAPSHOT, Key.PrintScreen);
			AddKey(VirtualKeys.CLEAR, Key.Clear);
			AddKey(VirtualKeys.INSERT, Key.Insert);

			AddKey(VirtualKeys.SLEEP, Key.Sleep);

			// Keypad
			for (int i = 0; i <= 9; i++) {
				AddKey((VirtualKeys)((int)VirtualKeys.NUMPAD0 + i), Key.Keypad0 + i);
			}
			AddKey(VirtualKeys.DECIMAL, Key.KeypadDecimal);
			AddKey(VirtualKeys.ADD, Key.KeypadAdd);
			AddKey(VirtualKeys.SUBTRACT, Key.KeypadSubtract);
			AddKey(VirtualKeys.DIVIDE, Key.KeypadDivide);
			AddKey(VirtualKeys.MULTIPLY, Key.KeypadMultiply);

			// Navigation
			AddKey(VirtualKeys.UP, Key.Up);
			AddKey(VirtualKeys.DOWN, Key.Down);
			AddKey(VirtualKeys.LEFT, Key.Left);
			AddKey(VirtualKeys.RIGHT, Key.Right);
		}
		
		void AddKey( VirtualKeys vKey, Key key ) {
			Add( (int)vKey, key );
		}
		
		public bool TryGetMappedKey( VirtualKeys vKey, out Key key ) {
			return TryGetValue( (int)vKey, out key );
		}
	}
}
