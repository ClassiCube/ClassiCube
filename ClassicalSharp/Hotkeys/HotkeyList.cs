// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
					hKey.StaysOpen = more;
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
			hotkey.StaysOpen = more;
			
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
					moreInput = hKey.StaysOpen;
					return true;
				}
			}
			
			text = null;
			moreInput = false;
			return false;
		}
		
		public void LoadSavedHotkeys() {
			foreach( var pair in Options.OptionsSet ) {
				if( Utils.CaselessStarts( pair.Key, "hotkey-" ) ) {
					// First retrieve the parts from 
					const int startIndex = 7;
					int keySplit = pair.Key.IndexOf( '&', startIndex );
					if( keySplit < 0 ) {
						Utils.LogDebug( "Hotkey {0} has an invalid key", pair.Key );
						continue;
					}
					string strKey = pair.Key.Substring( startIndex, keySplit - startIndex );
					string strFlags = pair.Key.Substring( keySplit + 1, pair.Key.Length - keySplit - 1 );
					
					int valueSplit = pair.Value.IndexOf( '&', 0 );
					if( valueSplit < 0 ) {
						Utils.LogDebug( "Hotkey {0} has an invalid value", pair.Key );
						continue;
					}
					string strMoreInput = pair.Value.Substring( 0, valueSplit - 0 );
					string strText = pair.Value.Substring( valueSplit + 1, pair.Value.Length - valueSplit - 1 );
					
					// Then try to parse the key and value
					Key key; byte flags; bool moreInput;
					if( !Utils.TryParseEnum( strKey, Key.Unknown, out key ) ||
					   !Byte.TryParse( strFlags, out flags ) ||
					   !Boolean.TryParse( strMoreInput, out moreInput ) ||
					   strText.Length == 0 ) {
						Utils.LogDebug( "Hotkey {0} has invalid arguments", pair.Key );
						continue;
					}
					AddHotkey( key, flags, strText, moreInput );
				}
			}
		}
		
		public void UserRemovedHotkey( Key baseKey, byte flags ) {
			string key = "hotkey-" + baseKey + "&" + flags;
			Options.Set( key, null );
		}
		
		public void UserAddedHotkey( Key baseKey, byte flags, bool moreInput, string text ) {
			string key = "hotkey-" + baseKey + "&" + flags;
			string value = moreInput + "&" + text;
			Options.Set( key, value );
		}
	}
	
	public struct Hotkey {
		public Key BaseKey;
		public byte Flags; // ctrl 1, shift 2, alt 4
		public bool StaysOpen; // whether the user is able to enter further input
		public string Text; // contents to copy directly into the input bar
	}
}
