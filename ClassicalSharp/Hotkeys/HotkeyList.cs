using System;
using System.Collections.Generic;
using OpenTK.Input;

namespace ClassicalSharp.Hotkeys {

	/// <summary> Maintains the list of hotkeys defined by the client and by SetTextHotkey packets. </summary>
	public sealed class HotkeyList {
		
		public List<Hotkey> Hotkeys = new List<Hotkey>();
		
		/// <summary> Creates or updates an existing hotkey with the given baseKey and modifier flags. </summary>
		public void AddHotkey( Key baseKey, byte flags, string text, bool more ) {
			if( !UpdateExistingHotkey( baseKey, flags, text, more ) )
				AddNewHotkey( baseKey, flags, text, more );
		}
		
		/// <summary> Removes an existing hotkey with the given baseKey and modifier flags. </summary>
		/// <returns> Whether a hotkey with the given baseKey and modifier flags was found
		/// and subsequently removed. </returns>
		public bool RemoveHotkey( Key baseKey, byte flags ) {
			for( int i = 0; i < Hotkeys.Count; i++ ) {
				Hotkey hKey = Hotkeys[i];
				if( hKey.BaseKey == baseKey && hKey.Flags == flags ) {
					Hotkeys.RemoveAt( i );
					return true;
				}
			}
			return false;
		}
		
		bool UpdateExistingHotkey( Key baseKey, byte flags, string text, bool more ) {
			for( int i = 0; i < Hotkeys.Count; i++ ) {
				Hotkey hKey = Hotkeys[i];
				if( hKey.BaseKey == baseKey && hKey.Flags == flags ) {
					hKey.Text = text;
					hKey.MoreInput = more;
					Hotkeys[i] = hKey;
					return true;
				}
			}
			return false;
		}
		
		void AddNewHotkey( Key baseKey, byte flags, string text, bool more ) {
			Hotkey hotkey;
			hotkey.BaseKey = baseKey;
			hotkey.Flags = flags;
			hotkey.Text = text;
			hotkey.MoreInput = more;
			
			Hotkeys.Add( hotkey );
			// sort so that hotkeys with largest modifiers are first
			Hotkeys.Sort( (a, b) => b.Flags.CompareTo( a.Flags ) );
		}
		
		/// <summary> Determines whether a hotkey is active based on the given key,
		/// and the currently active control, alt, and shift modifiers </summary>
		public bool IsHotkey( Key key, KeyboardDevice keyboard,
		                     out string text, out bool moreInput ) {
			byte flags = 0;
			if( keyboard[Key.ControlLeft] || keyboard[Key.ControlRight] ) flags |= 1;
			if( keyboard[Key.ShiftLeft] || keyboard[Key.ShiftRight] ) flags |= 2;
			if( keyboard[Key.AltLeft] || keyboard[Key.AltRight] ) flags |= 4;
			
			foreach( Hotkey hKey in Hotkeys ) {
				if( (hKey.Flags & flags) == hKey.Flags && hKey.BaseKey == key ) {
					text = hKey.Text;
					moreInput = hKey.MoreInput;
					return true;
				}
			}
			
			text = null;
			moreInput = false;
			return false;
		}
	}	
	
	public struct Hotkey {
		public Key BaseKey;
		public byte Flags; // ctrl 1, shift 2, alt 4
		public string Text; // contents to copy directly into the input bar
		public bool MoreInput; // whether the user is able to enter further input
	}
}
