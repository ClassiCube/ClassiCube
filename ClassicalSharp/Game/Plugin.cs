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
		
		/// <summary> API version this plugin is compatible with. </summary>
		int APIVersion { get; }
	}
	
	internal static class PluginLoader {
		public static EntryList Accepted, Denied;
		
		internal static List<string> LoadAll(Game game) {
			Utils.EnsureDirectory("plugins");
			
			Accepted = new EntryList("plugins", "accepted.txt");
			Denied = new EntryList("plugins", "denied.txt");
			Accepted.Load();
			Denied.Load();
			
			return LoadPlugins(game);
		}
		
		static List<string> LoadPlugins(Game game) {
			string[] dlls = Platform.DirectoryFiles("plugins", "*.dll");
			List<string> nonLoaded = null;
			
			for (int i = 0; i < dlls.Length; i++) {
				string plugin = Path.GetFileNameWithoutExtension(dlls[i]);
				if (Denied.Has(plugin)) continue;
				
				if (Accepted.Has(plugin)) {
					Load(plugin, game, false);
				} else if (nonLoaded == null) {
					nonLoaded = new List<string>();
					nonLoaded.Add(plugin);
				} else {
					nonLoaded.Add(plugin);
				}
			}
			return nonLoaded;
		}
		
		public static void Load(string pluginName, Game game, bool needsInit) {
			try {
				string path = Path.Combine("plugins", pluginName + ".dll");
				Assembly lib = Assembly.LoadFrom(path);
				Type[] types = lib.GetTypes();
				
				for (int i = 0; i < types.Length; i++) {
					if (!IsPlugin(types[i])) continue;					
					Plugin plugin = (Plugin)Activator.CreateInstance(types[i]);
					
					if (plugin.APIVersion < Program.APIVersion) {
						game.Chat.Add("&c" + pluginName + " plugin is outdated! Try getting a more recent version.");
						continue;
					}
					if (plugin.APIVersion > Program.APIVersion) {
						game.Chat.Add("&cYour game is too outdated to use " + pluginName + " plugin! Try updating it.");
						continue;
					}
					
					if (needsInit) {
						plugin.Init(game);
						plugin.Ready(game);
					}
					
					if (plugin == null)
						throw new InvalidOperationException("Type " + types[i].Name + " returned null instance");
					game.Components.Add(plugin);
				}
			} catch (Exception ex) {
				game.Chat.Add("&cError loading " + pluginName + " plugin, see client.log");
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
