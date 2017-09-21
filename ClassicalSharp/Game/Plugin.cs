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
	
	internal class PluginLoader {

		EntryList accepted, denied;
		Game game;
		
		public PluginLoader(Game game) { this.game = game; }
		
		internal List<string> LoadAll() {
			string dir = Path.Combine(Program.AppDirectory, "plugins");
			if (!Directory.Exists(dir))
				Directory.CreateDirectory(dir);
			
			accepted = new EntryList("plugins", "accepted.txt");
			denied = new EntryList("plugins", "denied.txt");
			accepted.Load();
			denied.Load();
			
			return LoadPlugins(dir);
		}
		
		internal void MakeWarning(Game game, string plugin) {
			WarningOverlay warning = new WarningOverlay(game, true, false);
			warning.Metadata = plugin;
			warning.SetHandlers(Accept, Deny);
			
			warning.lines[0] = "&eAre you sure you want to load plugin " + plugin + " ?";
			warning.lines[1] = "Be careful - plugins from strangers may have viruses";
			warning.lines[2] = " or other malicious behaviour.";
			game.Gui.ShowOverlay(warning);
		}
		
		
		void Accept(Overlay overlay, bool always) {
			string plugin = overlay.Metadata;
			if (always && !accepted.HasEntry(plugin)) {
				accepted.AddEntry(plugin);
			}
			
			string dir = Path.Combine(Program.AppDirectory, "plugins");
			Load(Path.Combine(dir, plugin + ".dll"), true);
		}

		void Deny(Overlay overlay, bool always) {
			string plugin = overlay.Metadata;
			if (always && !denied.HasEntry(plugin)) {
				denied.AddEntry(plugin);
			}
		}
		
		List<string> LoadPlugins(string dir) {
			string[] dlls = Directory.GetFiles(dir, "*.dll");
			List<string> nonLoaded = null;
			
			for (int i = 0; i < dlls.Length; i++) {
				string plugin = Path.GetFileNameWithoutExtension(dlls[i]);
				if (denied.HasEntry(plugin)) continue;
				
				if (accepted.HasEntry(plugin)) {
					Load(dlls[i], false);
				} else if (nonLoaded == null) {
					nonLoaded = new List<string>();
					nonLoaded.Add(plugin);
				} else {
					nonLoaded.Add(plugin);
				}
			}
			return nonLoaded;
		}
		
		void Load(string path, bool needsInit) {
			try {
				Assembly lib = Assembly.LoadFile(path);
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
				path = Path.GetFileNameWithoutExtension(path);
				ErrorHandler.LogError("PluginLoader.Load() - " + path, ex);
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
