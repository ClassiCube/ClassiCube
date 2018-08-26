﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Textures;

namespace ClassicalSharp {

	public static class OptionsKey {
		#if !LAUNCHER
		public const string UseMusic = "usemusic";
		public const string UseSound = "usesound";
		public const string MusicVolume = "musicvolume";
		public const string SoundsVolume = "soundsvolume";
		public const string ForceOpenAL = "forceopenal";
		public const string ForceOldOpenGL = "force-oldgl";
		
		public const string ViewDist = "viewdist";
		public const string BlockPhysics = "singleplayerphysics";
		public const string NamesMode = "namesmode";
		public const string InvertMouse = "invertmouse";
		public const string Sensitivity = "mousesensitivity";
		public const string FpsLimit = "fpslimit";
		#endif
		public const string DefaultTexturePack = "defaulttexpack";
		public const string AutoCloseLauncher = "autocloselauncher";
		#if !LAUNCHER
		public const string ViewBobbing = "viewbobbing";
		public const string EntityShadow = "entityshadow";
		public const string RenderType = "normal";
		public const string SmoothLighting = "gfx-smoothlighting";
		public const string Mipmaps = "gfx-mipmaps";
		public const string SurvivalMode = "game-survival";
		public const string ChatLogging = "chat-logging";
		public const string WindowWidth = "window-width";
		public const string WindowHeight = "window-height";
		
		public const string HacksOn = "hacks-hacksenabled";
		public const string FieldOfView = "hacks-fov";
		public const string Speed = "hacks-speedmultiplier";
		public const string JumpVelocity = "hacks-jumpvelocity";
		public const string ModifiableLiquids = "hacks-liquidsbreakable";
		public const string PushbackPlacing = "hacks-pushbackplacing";
		public const string NoclipSlide = "hacks-noclipslide";
		public const string CameraClipping = "hacks-cameraclipping";
		public const string WOMStyleHacks = "hacks-womstylehacks";
		public const string FullBlockStep = "hacks-fullblockstep";
		
		public const string TabAutocomplete = "gui-tab-autocomplete";
		public const string ShowBlockInHand = "gui-blockinhand";
		public const string ChatLines = "gui-chatlines";
		public const string ClickableChat = "gui-chatclickable";
		#endif
		public const string UseChatFont = "gui-arialchatfont";
		#if !LAUNCHER
		public const string HotbarScale = "gui-hotbarscale";
		public const string InventoryScale = "gui-inventoryscale";
		public const string ChatScale = "gui-chatscale";
		public const string ShowFPS = "gui-showfps";
		public const string FontName = "gui-fontname";
		public const string BlackText = "gui-blacktextshadows";
		#endif
		
		public const string ClassicMode = "mode-classic";
		public const string CustomBlocks = "nostalgia-customblocks";
		public const string CPE = "nostalgia-usecpe";		
		public const string ServerTextures = "nostalgia-servertextures";
		public const string ClassicGui = "nostalgia-classicgui";
		public const string SimpleArmsAnim = "nostalgia-simplearms";
		public const string ClassicTabList = "nostalgia-classictablist";
		public const string ClassicOptions = "nostalgia-classicoptions";
		public const string ClassicHacks = "nostalgia-hacks";
		#if !LAUNCHER
		public const string ClassicArmModel = "nostalgia-classicarm";
		public const string MaxChunkUpdates = "gfx-maxchunkupdates";
		public const string CameraFriction = "camerafriction";
    }
	public enum FpsLimitMethod {
		LimitVSync, Limit30FPS, Limit60FPS, Limit120FPS, LimitNone,
		#endif
	}

	
	public static class Options {
		
		public static List<string> OptionsKeys = new List<string>();
		public static List<string> OptionsValues = new List<string>();
		static List<string> OptionsChanged = new List<string>();
		const string Filename = "options.txt";
		
		public static bool HasChanged() { return OptionsChanged.Count > 0; }
		
		static bool IsChangedOption(string key) {
			for (int i = 0; i < OptionsChanged.Count; i++) {
				if (Utils.CaselessEq(key, OptionsChanged[i])) return true;
			}
			return false;
		}
		
		static bool TryGetValue(string key, out string value) {
			value = null;
			int i = FindOption(key);
			if (i >= 0) { value = OptionsValues[i]; return true; }
			
			int sepIndex = key.IndexOf('-');
			if (sepIndex == -1) return false;
			
			key = key.Substring(sepIndex + 1);
			i = FindOption(key);
			if (i >= 0) { value = OptionsValues[i]; return true; }
			return false;
		}
		
		public static string Get(string key, string defValue) {
			string value;
			return TryGetValue(key, out value) ? value : defValue;
		}
		
		public static int GetInt(string key, int min, int max, int defValue) {
			string value;
			int valueInt = 0;
			if (!TryGetValue(key, out value) || !Int32.TryParse(value, out valueInt))
				return defValue;

			Utils.Clamp(ref valueInt, min, max);
			return valueInt;
		}
		
		public static bool GetBool(string key, bool defValue) {
			string value;
			bool valueBool = false;
			if (!TryGetValue(key, out value) || !Boolean.TryParse(value, out valueBool))
				return defValue;
			return valueBool;
		}

		#if !LAUNCHER
		public static float GetFloat(string key, float min, float max, float defValue) {
			string value;
			float valueFloat = 0;
			if (!TryGetValue(key, out value) || !Utils.TryParseDecimal(value, out valueFloat))
				return defValue;
			Utils.Clamp(ref valueFloat, min, max);
			return valueFloat;
		}
		
		public static T GetEnum<T>(string key, T defValue) {
			string value = Get(key, null);
			if (value == null) return defValue;
			
			T mapping;
			if (!Utils.TryParseEnum(value, defValue, out mapping)) {
				Set(key, defValue.ToString());
				mapping = defValue;
			}
			return mapping;
		}
		#endif
		
		static int FindOption(string key) {
			for (int i = 0; i < OptionsKeys.Count; i++) {
				if (Utils.CaselessEq(OptionsKeys[i], key)) return i;
			}
			return -1;
		}
		
		public static void Set(string key, int value)  { Set(key, value.ToString()); }
		public static void Set(string key, bool value) { Set(key, value.ToString()); }
		public static void Set(string key, string value) {
			if (value == null) {
				int i = FindOption(key);
				if (i >= 0) RemoveOption(i);
			} else {
				SetOption(key, value);
			}
			
			if (!IsChangedOption(key)) {
				OptionsChanged.Add(key);
			}
		}
		
		static void SetOption(string key, string value) {
			int i = FindOption(key);
			if (i >= 0) {
				OptionsValues[i] = value;
			} else {
				OptionsKeys.Add(key);
				OptionsValues.Add(value);
			}
		}
		
		static void RemoveOption(int i) {
			OptionsKeys.RemoveAt(i);
			OptionsValues.RemoveAt(i);
		}
		
		
		public static bool Load() {
			try {
				using (Stream fs = Platform.FileOpen(Filename))
					using (StreamReader reader = new StreamReader(fs, false))
				{
					LoadFrom(reader);
				}
				return true;
			} catch (FileNotFoundException) {
				return true;
			} catch (IOException ex) {
				ErrorHandler.LogError("loading options", ex);
				return false;
			}
		}
		
		static void LoadFrom(StreamReader reader) {
			// remove all the unchanged options
			for (int i = OptionsKeys.Count - 1; i >= 0; i--) {
				string key = OptionsKeys[i];
				if (IsChangedOption(key)) continue;
				RemoveOption(i);
			}

			string line;
			while ((line = reader.ReadLine()) != null) {
				if (line.Length == 0 || line[0] == '#') continue;
				
				int sepIndex = line.IndexOf('=');
				if (sepIndex <= 0) continue;
				string key = line.Substring(0, sepIndex);
				
				sepIndex++;
				if (sepIndex == line.Length) continue;
				string value = line.Substring(sepIndex, line.Length - sepIndex);
				
				if (!IsChangedOption(key)) {
					SetOption(key, value);
				}
			}
		}
		
		public static bool Save() {
			try {
				using (Stream fs = Platform.FileCreate(Filename))
					using (StreamWriter writer = new StreamWriter(fs))
				{
					for (int i = 0; i < OptionsKeys.Count; i++) {
						writer.Write(OptionsKeys[i]);
						writer.Write('=');
						writer.Write(OptionsValues[i]);
						writer.WriteLine();
					}
				}
				
				OptionsChanged.Clear();
				return true;
			} catch (IOException ex) {
				ErrorHandler.LogError("saving options", ex);
				return false;
			}
		}
	}
}
