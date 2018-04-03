// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if !ANDROID
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {	
	// TODO: Hotkey added event for CPE
	public sealed class HotkeyListScreen : ListScreen {
		
		public HotkeyListScreen(Game game) : base(game) {
			titleText = "Modify hotkeys";
			HotkeyList hotkeys = game.Input.Hotkeys;
			int count = hotkeys.Hotkeys.Count;
			entries = new string[hotkeys.Hotkeys.Count + items];
			
			for (int i = 0; i < count; i++) {
				Hotkey hKey = hotkeys.Hotkeys[i];
				entries[i] = hKey.BaseKey + " |" + MakeFlagsString(hKey.Flags);
			}
			for (int i = 0; i < items; i++)
				entries[count + i] = "-----";
		}
		
		internal static string MakeFlagsString(byte flags) {
			if (flags == 0) return " None";
			
			return ((flags & 1) == 0 ? "" : " Ctrl") +
				((flags & 2) == 0 ? "" : " Shift") +
				((flags & 4) == 0 ? "" : " Alt");
		}
		
		protected override void TextButtonClick(Game game, Widget widget) {
			string text = ((ButtonWidget)widget).Text;
			Hotkey original = default(Hotkey);
			if (text != "-----") 
				original = Parse(text);
			game.Gui.SetNewScreen(new EditHotkeyScreen(game, original));
		}
		
		Hotkey Parse(string text) {			
			int sepIndex = text.IndexOf('|');
			string key = text.Substring(0, sepIndex - 1);
			string value = text.Substring(sepIndex + 1);
			
			byte flags = 0;
			if (value.Contains("Ctrl")) flags |= 1;
			if (value.Contains("Shift")) flags |= 2;
			if (value.Contains("Alt")) flags |= 4;
			
			Key hKey = (Key)Enum.Parse(typeof(Key), key);
			HotkeyList hotkeys = game.Input.Hotkeys;
			
			for (int i = 0; i < hotkeys.Hotkeys.Count; i++) {
				Hotkey h = hotkeys.Hotkeys[i];
				if (h.BaseKey == hKey && h.Flags == flags) return h;
			}
			return default(Hotkey);
		}
	}
}
#endif