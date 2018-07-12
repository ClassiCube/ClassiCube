#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;

namespace OpenTK.Input {
	
	public enum Key : int {
		None = 0,
		
		// Modifiers
		ShiftLeft, ShiftRight, ControlLeft, ControlRight,
		AltLeft, AltRight, WinLeft, WinRight, Menu,
		
		// Function keys (hopefully enough for most keyboards - mine has 26)
		// <keysymdef.h> on X11 reports up to 35 function keys.
		F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10,
		F11, F12, F13, F14, F15, F16, F17, F18, F19, F20,
		F21, F22, F23, F24, F25, F26, F27, F28, F29, F30,
		F31, F32, F33, F34, F35,
		
		// Direction arrows
		Up, Down, Left, Right,
		
		// Action keys
		Enter, Escape, Space, Tab, BackSpace, Insert,
		Delete, PageUp, PageDown, Home, End, CapsLock,
		ScrollLock, PrintScreen, Pause, NumLock,
		
		// Keypad keys
		Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
		Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
		KeypadDivide, KeypadMultiply, KeypadSubtract,
		KeypadAdd, KeypadDecimal, KeypadEnter,
		
		// Letters
		A, B, C, D, E, F, G, H, I, J,
		K, L, M, N, O, P, Q, R, S, T,
		U, V, W, X, Y, Z,
		
		// Numbers
		Number0, Number1, Number2, Number3, Number4,
		Number5, Number6, Number7, Number8, Number9,
		
		// Symbols
		Tilde, Minus, Plus, BracketLeft, BracketRight,
		Semicolon, Quote, Comma, Period, Slash, BackSlash,
		
		// Extended mouse buttons
		XButton1, XButton2,
		// Last available keyboard key
		LastKey
	}
	
	public static class Keyboard {
		static bool[] states = new bool[(int)Key.LastKey];
		static KeyboardKeyEventArgs args = new KeyboardKeyEventArgs();
		public static bool KeyRepeat;

		public static bool Get(Key key) { return states[(int)key]; }
		internal static void Set(Key key, bool value) {
			if (states[(int)key] != value || KeyRepeat) {
				states[(int)key] = value;
				args.Key = key;
				
				if (value && KeyDown != null) {
					KeyDown(null, args);
				} else if (!value && KeyUp != null) {
					KeyUp(null, args);
				}
			}
		}

		public static event EventHandler<KeyboardKeyEventArgs> KeyDown;
		public static event EventHandler<KeyboardKeyEventArgs> KeyUp;

		internal static void ClearKeys() {
			for (int i = 0; i < states.Length; i++) {
				// Make sure KeyUp events are *not* raised for keys that are up, even if key repeat is on.
				if (states[i]) { Set((Key)i, false); }
			}
		}
	}

	public class KeyboardKeyEventArgs : EventArgs { public Key Key; }
}