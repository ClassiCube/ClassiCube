#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;

namespace OpenTK.Input {
	
	/// <summary> Represents a keyboard device and provides methods to query its status.  </summary>
	public static class Keyboard {
		static bool[] states = new bool[(int)Key.LastKey];
		static KeyboardKeyEventArgs args = new KeyboardKeyEventArgs();

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

		/// <summary> Gets or sets a System.Boolean indicating key repeat status. </summary>
		/// <remarks> If KeyRepeat is true, multiple KeyDown events will be generated while a key is being held.
		/// Otherwise only one KeyDown event will be reported.
		/// <para> The rate of the generated KeyDown events is controlled by the Operating System. Usually,
		/// one KeyDown event will be reported, followed by a small (250-1000ms) pause and several
		/// more KeyDown events (6-30 events per second). </para>
		/// <para> Set to true to handle text input (where keyboard repeat is desirable), but set to false
		/// for game input. </para> </remarks>
		public static bool KeyRepeat;

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