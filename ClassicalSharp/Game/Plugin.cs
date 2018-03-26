// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using ClassicalSharp.Textures;
using ClassicalSharp.Gui.Screens;

namespace ClassicalSharp {
	
	/// <summary> Allows external functionality to be added to the client. </summary>
	public interface Plugin : IGameComponent {
		
		/// <summary> Client version this plugin is compatible with. </summary>
		string ClientVersion { get; }
	}
	
	internal static class PluginLoader {

		public static EntryList Accepted, Denied;
		internal static Game game;
		
		internal static List<string> LoadAll() {
			string dir = Path.Combine(Program.AppDirectory, "plugins");
			if (!Directory.Exists(dir))
				Directory.CreateDirectory(dir);
			
			Accepted = new EntryList("plugins", "accepted.txt");
			Denied = new EntryList("plugins", "denied.txt");
			Accepted.Load();
			Denied.Load();
			
			return LoadPlugins(dir);
		}
		
		static List<string> LoadPlugins(string dir) {
			string[] dlls = Directory.GetFiles(dir, "*.dll");
			List<string> nonLoaded = null;
			
			for (int i = 0; i < dlls.Length; i++) {
				string plugin = Path.GetFileNameWithoutExtension(dlls[i]);
				if (Denied.Has(plugin)) continue;
				
				if (Accepted.Has(plugin)) {
					Load(plugin, false);
				} else if (nonLoaded == null) {
					nonLoaded = new List<string>();
					nonLoaded.Add(plugin);
				} else {
					nonLoaded.Add(plugin);
				}
			}
			return nonLoaded;
		}
		
		public static void Load(string pluginName, bool needsInit) {
			try {
				string dir = Path.Combine(Program.AppDirectory, "plugins");
				string path = Path.Combine(dir, pluginName + ".dll");
				Assembly lib = Assembly.LoadFrom(path);
				Type[] types = lib.GetTypes();
				
				for (int i = 0; i < types.Length; i++) {
					if (!IsPlugin(types[i])) continue;
					
					IGameComponent plugin = (IGameComponent)Activator.CreateInstance(types[i]);
					if (needsInit) {
						plugin.Init(game);
						plugin.Ready(game);
					}
					
					if (plugin == null)
						throw new InvalidOperationException("Type " + types[i].Name + " returned null instance");
					game.Components.Add(plugin);
				}
			} catch (Exception ex) {
				ErrorHandler.LogError("PluginLoader.Load() - " + pluginName, ex);
			}
		}
		
		static bool IsPlugin(Type t) {
			if (t.IsAbstract || t.IsInterface) return false;
			
			Type[] interfaces = t.GetInterfaces();
			for (int i = 0; i < interfaces.Length; i++) {
				if (interfaces[i] == typeof(Plugin)) return true;
			}
			return false;
		}
	}
}
