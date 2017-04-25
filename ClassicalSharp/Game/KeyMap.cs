﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK.Input;

namespace ClassicalSharp {
	
	/// <summary> Enumeration of all custom key bindings. </summary>
	public enum KeyBind {
#pragma warning disable 1591
		Forward, Back, Left, Right, Jump, Respawn, SetSpawn, Chat,
		Inventory, ToggleFog, SendChat, PauseOrExit, PlayerList,
		Speed, NoClip, Fly, FlyUp, FlyDown, ExtInput, HideFps,
		Screenshot, Fullscreen, ThirdPerson, HideGui, AxisLines,
		ZoomScrolling, HalfSpeed, MouseLeft, MouseMiddle, MouseRight, 
		Autorotate, HotbarSwitching
#pragma warning restore 1591
	}
	
	/// <summary> Maps a key binding to its actual key on the keyboard. </summary>
	public class KeyMap {
		
		/// <summary> Gets the actual key on the keyboard that maps to the given key binding. </summary>
		public Key this[KeyBind key] {
			get { return keys[(int)key]; }
			set { keys[(int)key] = value; SaveKeyBindings(); }
		}
		
		/// <summary> Gets the default key on the keyboard that maps to the given key binding. </summary>
		public Key GetDefault(KeyBind key) {
			return defaultKeys[(int)key];
		}
		
		Key[] keys, defaultKeys;
		
		public KeyMap() {
			// We can't use enum array initaliser because this causes problems when building with mono
			// and running on default .NET (https://bugzilla.xamarin.com/show_bug.cgi?id=572)
			keys = new Key[32];
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
			keys[30] = Key.F6; keys[31] = Key.AltLeft;
			
			defaultKeys = new Key[keys.Length];
			for (int i = 0; i < defaultKeys.Length; i++)
				defaultKeys[i] = keys[i];
			LoadKeyBindings();
		}
		
		
		void LoadKeyBindings() {
			string[] names = KeyBind.GetNames(typeof(KeyBind));
			for (int i = 0; i < names.Length; i++) {
				string key = "key-" + names[i];
				Key mapping = Options.GetEnum(key, keys[i]);
				if (mapping != Key.Escape)
					keys[i] = mapping;
			}
		}
		
		void SaveKeyBindings() {
			string[] names = KeyBind.GetNames(typeof(KeyBind));
			for (int i = 0; i < names.Length; i++) {
				Options.Set("key-" + names[i], keys[i]);
			}
		}
	}
}