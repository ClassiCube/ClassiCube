using System;
using System.Collections.Generic;
using System.Reflection;

namespace ClassicalSharp.Plugin {
	
	public abstract class Plugin {
		
		public abstract string Name { get; }
		
		public abstract string Description { get; }
		
		public abstract List<PluginModule> Modules { get; }
		
		public static void LoadPlugin( string file, Game game ) {
			Assembly assembly = Assembly.LoadFile( file );
			Type[] types = assembly.GetTypes();
			foreach( Type t in types ) {
				if( t.IsAbstract || t.IsInterface || t.IsGenericType ) continue;
				if( t.IsAssignableFrom( typeof( Plugin ) ) ) {
					Plugin p = (Plugin)Activator.CreateInstance( t );
					Console.WriteLine( p.Name );
					Console.WriteLine( p.Description );
				}
			}
		}
	}
	
	public enum PluginModuleType {
		EnvironmentRenderer,
		WeatherRenderer,
		Command,
	}
	
	public class PluginModule {
		
		public PluginModuleType ModuleType;
		
		public Type ImplementationType;
		
		public PluginModule( PluginModuleType moduleType, Type implementation ) {
			ModuleType = moduleType;
			ImplementationType = implementation;
		}
	}
}
