using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public enum KeyBinding {
		Forward, Back, Left, Right, Jump, Respawn, SetSpawn, OpenChat,
		SendChat, PauseOrExit, OpenInventory, ViewDistance, PlayerList, 
		Speed, NoClip, Fly, FlyUp, FlyDown, HideGui, HideFps,
		Screenshot, Fullscreen, ThirdPersonCamera,
	}
	
	public class KeyMap {
		
		public Key this[KeyBinding key] {
			get { return Keys[(int)key]; }
			set { Keys[(int)key] = value; SaveKeyBindings(); }
		}
		
		Key[] Keys;
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
			Keys = new Key[23];
			Keys[0] = Key.W; Keys[1] = Key.S; Keys[2] = Key.A; Keys[3] = Key.D;
			Keys[4] = Key.Space; Keys[5] = Key.R; Keys[6] = Key.Y; Keys[7] = Key.T;
			Keys[8] = Key.Enter; Keys[9] = Key.Escape; Keys[10] = Key.B;
			Keys[11] = Key.F; Keys[12] = Key.Tab; Keys[13] = Key.ShiftLeft;
			Keys[14] = Key.X; Keys[15] = Key.Z; Keys[16] = Key.Q;
			Keys[17] = Key.E; Keys[18] = Key.F1; Keys[19] = Key.F3;
			Keys[20] = Key.F12; Keys[21] = Key.F11; Keys[22] = Key.F5;
			LoadKeyBindings();
		}
		
		void LoadKeyBindings() {
			string[] names = KeyBinding.GetNames( typeof( KeyBinding ) );
			for( int i = 0; i < names.Length; i++ ) {
				string key = "key-" + names[i];
				Key mapping = Options.GetEnum( key, Keys[i] );
				if( !IsReservedKey( mapping ) )
					Keys[i] = mapping;
			}
		}
		
		void SaveKeyBindings() {
			string[] names = KeyBinding.GetNames( typeof( KeyBinding ) );
			for( int i = 0; i < names.Length; i++ ) {
				Options.Set( "key-" + names[i], Keys[i] );
			}
		}
	}
}