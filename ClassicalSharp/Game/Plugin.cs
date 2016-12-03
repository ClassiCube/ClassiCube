// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.Reflection;
using ClassicalSharp.Gui.Screens;

namespace ClassicalSharp {
	
	/// <summary> Allows external functionality to be added to the client. </summary>
	public interface Plugin : IGameComponent {
		
		/// <summary> Client version this plugin is compatible with. </summary>
		string ClientVersion { get; }
	}
	
	internal static class PluginLoader {

		internal static void Load(Game game) {
			string dllPath = Path.Combine(Program.AppDirectory, "plugins");
			if (!Directory.Exists(dllPath))
				Directory.CreateDirectory(dllPath);
			
			string[] dlls = Directory.GetFiles(dllPath, "*.dll");
			foreach (string dll in dlls) {
			}
		}
		
		internal static void MakeWarning(Game game, string plugin) {
			WarningScreen warning = new WarningScreen(game, false, false);
			warning.Metadata = plugin;
			warning.SetHandlers(w => Load(game, w), w => {}, null);
			
			warning.SetTextData(
				"&eAre you sure you want to load " + plugin + " plugin?",
				"Be careful - plugins from strangers may have viruses",
				" or other malicious behaviour.");
			game.Gui.ShowWarning(warning);
		}
		
		static void Load(Game game, WarningScreen warning) {
			string plugin = (string)warning.Metadata;
			string dllPath = Path.Combine(Program.AppDirectory, "plugins");
			Load(game, Path.Combine(dllPath, plugin));
		}
		
		static void Load(Game game, string path) {
			try {
				Assembly lib = Assembly.LoadFile(path);
				Type[] types = lib.GetTypes();
				
				foreach (Type t in lib.GetTypes()) {
					if (t.IsAbstract || t.IsInterface || !t.IsSubclassOf(typeof(Plugin))) continue;
					object instance = Activator.CreateInstance(t);
					
					if (instance == null)
						throw new InvalidOperationException("Type " + t.Name + " returned null instance");
					game.AddComponent((IGameComponent)instance);
				}
			} catch (Exception ex) {
				path = Path.GetFileNameWithoutExtension(path);
				game.Chat.Add("&cError loading plugin " + path + ", see client.log for details");
				ErrorHandler.LogError("PluginLoader.Load()", ex);
			}
		}
	}
}
