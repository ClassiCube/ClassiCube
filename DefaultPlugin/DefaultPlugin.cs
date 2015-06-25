using System;
using System.Collections.Generic;
using ClassicalSharp.Plugins;
using DefaultPlugin.Network;
using DefaultPlugin.Shaders;

namespace DefaultPlugin {
	
	public class DefaultPlugin : Plugin {
		
		public override string Name {
			get { return "ClassicalSharp default plugin"; }
		}
		
		public override string Authors {
			get { return "UnknownShadow200"; }
		}
		
		public override string Description {
			get { return "Contains the default set of commands and entity models for ClassicalSharp."; }
		}
		
		public override IEnumerable<PluginModule> Modules {
			get {
				return new List<PluginModule>() {
					new PluginModule( PluginModuleType.Command, typeof( CommandsCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( HelpCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( EnvCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( InfoCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( RenderTypeCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( ChatFontSizeCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( MouseSensitivityCommand ) ),
					new PluginModule( PluginModuleType.Command, typeof( PostProcessorCommand ) ),
					
					new PluginModule( PluginModuleType.EntityModel, typeof( BlockModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( ChickenModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( CreeperModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( PigModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( SheepModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( SkeletonModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( SpiderModel ) ),
					new PluginModule( PluginModuleType.EntityModel, typeof( ZombieModel ) ),
					
					new PluginModule( PluginModuleType.MapBordersRenderer, typeof( StandardMapBordersRenderer ) ),
					new PluginModule( PluginModuleType.EnvironmentRenderer, typeof( StandardEnvRenderer ) ),
					new PluginModule( PluginModuleType.EnvironmentRenderer, typeof( MinimalEnvRenderer ) ),
					new PluginModule( PluginModuleType.WeatherRenderer, typeof( StandardWeatherRenderer ) ),
					new PluginModule( PluginModuleType.MapRenderer, typeof( StandardMapRenderer ) ),
					
					new PluginModule( PluginModuleType.PostProcessingShader, typeof( GrayscaleFilter ) ),
					
					new PluginModule( PluginModuleType.NetworkProcessor, typeof( ClassicNetworkProcessor ) ),
				};
			}
		}
	}
}