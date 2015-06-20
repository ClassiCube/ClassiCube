using System;
using System.Collections.Generic;
using ClassicalSharp.Plugin;

namespace DefaultPlugin {
	
	public class DefaultPlugin : Plugin {
		
		public override string Name {
			get { return "ClassicalSharp default plugin"; }
		}
		
		public override string Authors {
			get { return "UnknownShadow200"; }
		}
		
		public override string Description {
			get { return "Contains the default set of commands for ClassicalSharp."; }
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
				};
			}
		}
	}
}