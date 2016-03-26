// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public enum KeyBinding {
		Forward, Back, Left, Right, Jump, Respawn, SetSpawn, OpenChat,
		OpenInventory, ViewDistance, SendChat, PauseOrExit, PlayerList, 
		Speed, NoClip, Fly, FlyUp, FlyDown, ExtendedInput, HideFps,
		Screenshot, Fullscreen, ThirdPersonCamera, HideGui, ShowAxisLines,
		ZoomScrolling, HalfSpeed, MouseLeft, MouseMiddle, MouseRight,
	}
	
	public class KeyMap {
		
		public Key this[KeyBinding key] {
			get { return keys[(int)key]; }
			set { keys[(int)key] = value; SaveKeyBindings(); }
		}
		
		public Key GetDefault( KeyBinding key ) {
			return defaultKeys[(int)key];
		}
		
		Key[] keys, defaultKeys;
		bool IsReservedKey( Key key ) {
			return key == Key.Escape || key == Key.Slash || key == Key.BackSpace ||
				(key >= Key.Insert && key <= Key.End) ||
				(key >= Key.Number0 && key <= Key.Number9); // block hotbar
		}
		
		public bool IsKeyOkay( Key oldKey, Key key, out string reason ) {
			if( oldKey == Key.Escape || oldKey == Key.F12 ) {
				reason = "This binding is locked";
				return false;
			}
			
			if( IsReservedKey( key ) ) {
				reason = "New key is reserved";
				return false;
			}
			reason = null;
			return true;
		}
		
		public KeyMap() {
			// See comment in Game() constructor
			keys = new Key[30];
			keys[0] = Key.W; keys[1] = Key.S; keys[2] = Key.A; keys[3] = Key.D;
			keys[4] = Key.Space; keys[5] = Key.R; keys[6] = Key.Enter; keys[7] = Key.T;
			keys[8] = Key.B; keys[9] = Key.F; keys[10] = Key.Enter;
			keys[11] = Key.Escape; keys[12] = Key.Tab; keys[13] = Key.ShiftLeft;
			keys[14] = Key.X; keys[15] = Key.Z; keys[16] = Key.Q;
			keys[17] = Key.E; keys[18] = Key.AltLeft; keys[19] = Key.F3;
			keys[20] = Key.F12; keys[21] = Key.F11; keys[22] = Key.F5;
			keys[23] = Key.F1; keys[24] = Key.F7; keys[25] = Key.C;
			keys[26] = Key.ControlLeft;
			keys[27] = Key.Unknown; keys[28] = Key.Unknown; keys[29] = Key.Unknown;
			
			defaultKeys = new Key[keys.Length];
			for( int i = 0; i < defaultKeys.Length; i++ )
				defaultKeys[i] = keys[i];
			LoadKeyBindings();
		}
		
		void LoadKeyBindings() {
			string[] names = KeyBinding.GetNames( typeof( KeyBinding ) );
			for( int i = 0; i < names.Length; i++ ) {
				string key = "key-" + names[i];
				Key mapping = Options.GetEnum( key, keys[i] );
				if( !IsReservedKey( mapping ) )
					keys[i] = mapping;
			}
		}
		
		void SaveKeyBindings() {
			string[] names = KeyBinding.GetNames( typeof( KeyBinding ) );
			for( int i = 0; i < names.Length; i++ ) {
				Options.Set( "key-" + names[i], keys[i] );
			}
		}
	}
}