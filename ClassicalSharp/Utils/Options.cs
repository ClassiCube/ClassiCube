﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Textures;

namespace ClassicalSharp {
	
	public static class OptionsKey {
		
		public const string ViewDist = "viewdist";
		public const string DefaultTexturePack = "defaulttexpack";
		public const string SingleplayerPhysics = "singleplayerphysics";
		public const string UseMusic = "usemusic";
		public const string UseSound = "usesound";
		public const string ForceOpenAL = "forceopenal";
		public const string NamesMode = "namesmode";
		public const string InvertMouse = "invertmouse";
		public const string Sensitivity = "mousesensitivity";
		public const string FpsLimit = "fpslimit";
		public const string AutoCloseLauncher = "autocloselauncher";
		public const string ViewBobbing = "viewbobbing";
		public const string EntityShadow = "entityshadow";
		public const string RenderType = "normal";
		public const string SmoothLighting = "gfx-smoothlighting";
		public const string SurvivalMode = "game-survivalmode";
		public const string ChatLogging = "chat-logging";
		
		public const string HacksEnabled = "hacks-hacksenabled";
		public const string FieldOfView = "hacks-fov";
		public const string Speed = "hacks-speedmultiplier";
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
		public const string ArialChatFont = "gui-arialchatfont";
		public const string HotbarScale = "gui-hotbarscale";
		public const string InventoryScale = "gui-inventoryscale";
		public const string ChatScale = "gui-chatscale";
		public const string ShowFPS = "gui-showfps";
		public const string FontName = "gui-fontname";
		public const string BlackTextShadows = "gui-blacktextshadows";
		
		public const string AllowCustomBlocks = "nostalgia-customblocks";
		public const string UseCPE = "nostalgia-usecpe";
		public const string AllowServerTextures = "nostalgia-servertextures";
		public const string UseClassicGui = "nostalgia-classicgui";
		public const string SimpleArmsAnim = "nostalgia-simplearms";
		public const string UseClassicTabList = "nostalgia-classictablist";
		public const string UseClassicOptions = "nostalgia-classicoptions";
		public const string AllowClassicHacks = "nostalgia-hacks";
	}
	
	public enum FpsLimitMethod {
		LimitVSync, Limit30FPS, Limit60FPS, Limit120FPS, LimitNone,
	}
	
	public static class Options {
		
		public static Dictionary<string, string> OptionsSet = new Dictionary<string, string>();
		public static List<string> OptionsChanged = new List<string>();
		const string Filename = "options.txt";
		
		static bool TryGetValue(string key, out string value) {
			if (OptionsSet.TryGetValue(key, out value)) return true;
			int index = key.IndexOf('-');
			
			if (index == -1) return false;
			return OptionsSet.TryGetValue(key.Substring(index + 1), out value);
		}
		
		public static string Get(string key) {
			string value;
			return TryGetValue(key, out value) ? value : null;
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
		
		public static float GetFloat(string key, float min, float max, float defValue) {
			string value;
			float valueFloat = 0;
			if (!TryGetValue(key, out value) || !Utils.TryParseDecimal(value, out valueFloat))
				return defValue;
			Utils.Clamp(ref valueFloat, min, max);
			return valueFloat;
		}
		
		public static T GetEnum<T>(string key, T defValue) {
			string value = Get(key.ToLower());
			if (value == null) {
				Set(key, defValue);
				return defValue;
			}
			
			T mapping;
			if (!Utils.TryParseEnum(value, defValue, out mapping))
				Set(key, defValue);
			return mapping;
		}
		
		public static void Set<T>(string key, T value) {
			key = key.ToLower();
			if (value != null) {
				OptionsSet[key] = value.ToString();
			} else {
				OptionsSet.Remove(key);
			}
			
			if (!OptionsChanged.Contains(key))
				OptionsChanged.Add(key);
		}
		
		public static bool Load() {
			// Both of these are from when running from the launcher
			if (Program.AppDirectory == null)
				Program.AppDirectory = AppDomain.CurrentDomain.BaseDirectory;
			string defZip = Path.Combine(Program.AppDirectory, "default.zip");
			string texDir = Path.Combine(Program.AppDirectory, TexturePack.Dir);
			if (File.Exists(defZip) || !Directory.Exists(texDir))
				Program.CleanupMainDirectory();
			
			try {
				string path = Path.Combine(Program.AppDirectory, Filename);
				using (Stream fs = File.OpenRead(path))
					using (StreamReader reader = new StreamReader(fs, false))
						LoadFrom(reader);
				return true;
			} catch (FileNotFoundException) {
				return true;
			} catch (IOException ex) {
				ErrorHandler.LogError("loading options", ex);
				return false;
			}
		}
		
		static void LoadFrom(StreamReader reader) {
			string line;
			// remove all the unchanged options
			List<string> toRemove = new List<string>();
			foreach (KeyValuePair<string, string> kvp in OptionsSet) {
				if (!OptionsChanged.Contains(kvp.Key))
					toRemove.Add(kvp.Key);
			}
			for (int i = 0; i < toRemove.Count; i++)
				OptionsSet.Remove(toRemove[i]);
			
			while ((line = reader.ReadLine()) != null) {
				if (line.Length == 0 || line[0] == '#') continue;
				
				int sepIndex = line.IndexOf('=');
				if (sepIndex <= 0) continue;
				string key = Utils.ToLower(line.Substring(0, sepIndex));
				
				sepIndex++;
				if (sepIndex == line.Length) continue;
				string value = line.Substring(sepIndex, line.Length - sepIndex);
				if (!OptionsChanged.Contains(key))
					OptionsSet[key] = value;
			}
		}
		
		public static bool Save() {
			try {
				string path = Path.Combine(Program.AppDirectory, Filename);
				using (Stream fs = File.Create(path))
					using (StreamWriter writer = new StreamWriter(fs))
				{
					SaveTo(writer);
				}
				
				OptionsChanged.Clear();
				return true;
			} catch (IOException ex) {
				ErrorHandler.LogError("saving options", ex);
				return false;
			}
		}
		
		static void SaveTo(StreamWriter writer) {
			foreach (KeyValuePair<string, string> pair in OptionsSet) {
				writer.Write(pair.Key);
				writer.Write('=');
				writer.Write(pair.Value);
				writer.WriteLine();
			}
		}
	}
}
