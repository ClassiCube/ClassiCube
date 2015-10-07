using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public enum KeyMapping {
		Forward, Back, Left, Right, Jump, Respawn, SetSpawn, OpenChat,
		SendChat, PauseOrExit, OpenInventory, Screenshot, Fullscreen,
		ThirdPersonCamera, ViewDistance, Fly, Speed, NoClip, FlyUp,
		FlyDown, PlayerList, HideGui,
	}
	
	public class KeyMap {
		
		public Key this[KeyMapping key] {
			get { return Keys[(int)key]; }
			set { Keys[(int)key] = value; SaveKeyBindings(); }
		}
		
		Key[] Keys;
		bool IsReservedKey( Key key ) {
			return key == Key.Escape || key == Key.F12 || key == Key.Slash || key == Key.BackSpace ||
				(key >= Key.Insert && key <= Key.End) ||
				(key >= Key.Up && key <= Key.Right) || // chat screen movement
				(key >= Key.Number0 && key <= Key.Number9); // block hotbar
		}
		
		public bool IsKeyOkay( Key oldKey, Key key, out string reason ) {
			if( oldKey == Key.Escape || oldKey == Key.F12 ) {
				reason = "This mapping is locked";
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
			#if !__MonoCS__
			Keys = new Key[] {
				Key.W, Key.S, Key.A, Key.D, Key.Space, Key.R, Key.Y, Key.T,
				Key.Enter, Key.Escape, Key.B, Key.F12, Key.F11, 
				Key.F5, Key.F, Key.Z, Key.ShiftLeft, Key.X, Key.Q,
				Key.E, Key.Tab, Key.F1 };
			#else
			Keys = new Key[22];
			Keys[0] = Key.W; Keys[1] = Key.S; Keys[2] = Key.A; Keys[3] = Key.D;
			Keys[4] = Key.Space; Keys[5] = Key.R; Keys[6] = Key.Y; Keys[7] = Key.T;
			Keys[8] = Key.Enter; Keys[9] = Key.Escape; Keys[10] = Key.B;
			Keys[11] = Key.F12; Keys[12] = Key.F11; Keys[13] = Key.F5; 
			Keys[14] = Key.F; Keys[15] = Key.Z; Keys[16] = Key.ShiftLeft; 
			Keys[17] = Key.X; Keys[18] = Key.Q; Keys[19] = Key.E; 
			Keys[20] = Key.Tab; Keys[21] = Key.F1;
			#endif
			LoadKeyBindings();
		}
		
		void LoadKeyBindings() {
			string[] names = KeyMapping.GetNames( typeof( KeyMapping ) );
			for( int i = 0; i < names.Length; i++ ) {
				string key = "key-" + names[i];
				string value = Options.Get( key );
				if( value == null ) {
					Options.Set( key, Keys[i] );
					continue;
				}
				
				Key mapping;
				try {
					mapping = (Key)Enum.Parse( typeof( Key ), value, true );
				} catch( ArgumentException ) {
					Options.Set( key, Keys[i] );
					continue;
				}
				if( !IsReservedKey( mapping ) )
					Keys[i] = mapping;
			}
		}
		
		void SaveKeyBindings() {
			string[] names = KeyMapping.GetNames( typeof( KeyMapping ) );
			for( int i = 0; i < names.Length; i++ ) {
				Options.Set( "key-" + names[i], Keys[i] );
			}
		}
	}
}