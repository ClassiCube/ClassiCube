using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using ClassicalSharp.Commands;
using ClassicalSharp.Model;

namespace ClassicalSharp.Plugins {
	
	public abstract class Plugin {
		
		public abstract string Name { get; }
		
		public abstract string Authors { get; }
		
		public abstract string Description { get; }
		
		public abstract IEnumerable<PluginModule> Modules { get; }
		
		public static void LoadPlugin( string file, Game game ) {
			Assembly assembly = Assembly.LoadFile( file );
			Type[] types = assembly.GetTypes();
			
			foreach( Type t in types ) {
				if( t.IsAbstract || t.IsInterface || t.IsGenericType ) continue;
				if( t.IsSubclassOf( typeof( Plugin ) ) ) {
					Plugin p = (Plugin)Activator.CreateInstance( t );
					Console.WriteLine( "Loading plugin: {0}, written by {1}", p.Name, p.Authors );
					Console.WriteLine( "Plugin description: {0}", p.Description );
					foreach( PluginModule module in p.Modules ) {
						HandleModule( module, game );
					}
				}
			}
		}
		
		static void HandleModule( PluginModule module, Game game ) {
			switch( module.ModuleType ) {
				case PluginModuleType.Command:
					Command cmd = Utils.New<Command>( module.Type, game );
					game.CommandManager.RegisterCommand( cmd );
					break;
					
				case PluginModuleType.EntityModel:
					IModel model = Utils.New<IModel>( module.Type, game );
					game.ModelCache.AddModel( model );
					break;
					
				case PluginModuleType.MapBordersRenderer:
					game.MapBordersRendererTypes.Add( module.Type );
					break;
					
				case PluginModuleType.EnvironmentRenderer:
					game.EnvRendererTypes.Add( module.Type );
					break;
					
				case PluginModuleType.WeatherRenderer:
					game.WeatherRendererTypes.Add( module.Type );
					break;
					
				case PluginModuleType.MapRenderer:
					game.MapRendererTypes.Add( module.Type );
					break;
			}
		}
	}
	
	public enum PluginModuleType {		
		Command,
		EntityModel,
		MapBordersRenderer,
		EnvironmentRenderer,
		WeatherRenderer,
		MapRenderer,
	}
	
	public class PluginModule {
		
		public PluginModuleType ModuleType;
		
		public Type Type;
		
		public PluginModule( PluginModuleType moduleType, Type implementation ) {
			ModuleType = moduleType;
			Type = implementation;
		}
	}
}
