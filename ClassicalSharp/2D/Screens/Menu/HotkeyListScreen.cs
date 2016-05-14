// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	// TODO: Hotkey added event for CPE
	public sealed class HotkeyListScreen : FilesScreen {
		
		public HotkeyListScreen( Game game ) : base( game ) {
			titleText = "Modify hotkeys";
			HotkeyList hotkeys = game.InputHandler.Hotkeys;
			entries = new string[hotkeys.Hotkeys.Count];
			for( int i = 0; i < entries.Length; i++ ) {
				Hotkey hKey = hotkeys.Hotkeys[i];
				entries[i] = hKey.BaseKey + " |" + MakeFlagsString( hKey.Flags );
			}
		}
		
		internal static string MakeFlagsString( byte flags ) {
			if( flags == 0 ) return " None";
			
			return ((flags & 1) == 0 ? "" : " Ctrl") +
				((flags & 2) == 0 ? "" : " Shift") +
				((flags & 4) == 0 ? "" : " Alt");
		}
		
		protected override void TextButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string text = ((ButtonWidget)widget).Text;
			Hotkey original = default(Hotkey);
			if( text != "-----" ) 
				original = Parse( text );
			game.SetNewScreen( new EditHotkeyScreen( game, original ) );
		}
		
		Hotkey Parse( string text ) {			
			int sepIndex = text.IndexOf( '|' );
			string key = text.Substring( 0, sepIndex - 1 );
			string value = text.Substring( sepIndex + 1 );
			
			byte flags = 0;
			if( value.Contains( "Ctrl" ) ) flags |= 1;
			if( value.Contains( "Shift" ) ) flags |= 2;
			if( value.Contains( "Alt" ) ) flags |= 4;
			
			Key hKey = (Key)Enum.Parse( typeof( Key ), key );
			HotkeyList hotkeys = game.InputHandler.Hotkeys;
			return hotkeys.Hotkeys.Find( h => h.BaseKey == hKey && h.Flags == flags );
		}
	}
}