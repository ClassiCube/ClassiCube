using System;
using System.Collections.Generic;
using System.Text;
using ClassicalSharp;
using ClassicalSharp.Commands;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {
	
	public sealed class CommandsCommand : Command {
		
		public CommandsCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "Commands"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client commands",
					"Prints a list of all usable commands",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			IEnumerable<string> parts = Utils.JoinItemsForChat( Window.CommandManager.RegisteredCommands,
			                                                   cmd => cmd.Name );
			foreach( string part in parts ) {
				Window.AddChat( part );
			}
		}
	}
	
	public sealed class HelpCommand : Command {
		
		public HelpCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "Help"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client help [command name]",
					"&eDisplays the help (if available) for the given command.",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			string commandName = reader.Next();
			if( commandName == null ) {
				Window.AddChat( "&e/client: &cYou didn't specify a command to get help with." );
			} else {
				Command cmd = Window.CommandManager.GetMatchingCommand( commandName );
				if( cmd != null ) {
					string[] help = cmd.Help;
					for( int i = 0; i < help.Length; i++ ) {
						Window.AddChat( help[i] );
					}
				}
			}
		}
	}
	
	public sealed class EnvCommand : Command {
		
		public EnvCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "Env"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client env [property] [value]",
					"&bproperties: &eskycol, fogcol, cloudscol, cloudsspeed,",
					"     &esuncol, shadowcol, weather",
					"&bcol properties: &evalue must be a hex colour code (either #RRGGBB or RRGGBB).",
					"&bcloudsspeed: &evalue must be a decimal number. (e.g. 0.5, 1, 4.5).",
					"&eIf no args are given, the current env variables will be displayed.",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			string property = reader.Next();
			if( property == null ) {
				Window.AddChat( "Fog colour: " + Window.Map.FogCol.ToRGBHexString() );
				Window.AddChat( "Clouds colour: " + Window.Map.CloudsCol.ToRGBHexString() );
				Window.AddChat( "Sky colour: " + Window.Map.SkyCol.ToRGBHexString() );
			} else if( Utils.CaselessEquals( property, "skycol" ) ) {
				ReadHexColourAnd( reader, c => Window.Map.SetSkyColour( c ) );
			} else if( Utils.CaselessEquals( property, "fogcol" ) ) {
				ReadHexColourAnd( reader, c => Window.Map.SetFogColour( c ) );
			} else if( Utils.CaselessEquals( property, "cloudscol" )
			          || Utils.CaselessEquals( property, "cloudcol" ) ) {
				ReadHexColourAnd( reader, c => Window.Map.SetCloudsColour( c ) );
			} else if( Utils.CaselessEquals( property, "suncol" ) ) {
				ReadHexColourAnd( reader, c => Window.Map.SetSunlight( c ) );
			} else if( Utils.CaselessEquals( property, "shadowcol" ) ) {
				ReadHexColourAnd( reader, c => Window.Map.SetShadowlight( c ) );
			} else if( Utils.CaselessEquals( property, "cloudsspeed" )
			          || Utils.CaselessEquals( property, "cloudspeed" ) ) {
				float speed;
				if( !reader.NextFloat( out speed ) ) {
					Window.AddChat( "&e/client env: &cInvalid clouds speed." );
				} else {
					StandardEnvRenderer env = Window.EnvRenderer as StandardEnvRenderer;
					if( env != null ) {
						env.CloudsSpeed = speed;
					}
				}
			} else if( Utils.CaselessEquals( property, "weather" ) ) {
				int weather;
				if( !reader.NextInt( out weather ) || weather < 0 || weather > 2 ) {
					Window.AddChat( "&e/client env: &cInvalid weather." );
				} else {
					Window.Map.SetWeather( (Weather)weather );
				}
			}
		}
		
		void ReadHexColourAnd( CommandReader reader, Action<FastColour> action ) {
			FastColour colour;
			if( !reader.NextHexColour( out colour ) ) {
				Window.AddChat( "&e/client env: &cInvalid hex colour." );
			} else {
				action( colour );
			}
		}
	}
	
	public sealed class InfoCommand : Command {
		
		public InfoCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "Info"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client info [property]",
					"&bproperties: &epos, target, dimensions, jumpheight, viewdist",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			string property = reader.Next();
			if( property == null ) {
				Window.AddChat( "&e/client info: &cYou didn't specify a property." );
			} else if( Utils.CaselessEquals( property, "pos" ) ) {
				Window.AddChat( "Feet: " + Window.LocalPlayer.Position );
				Window.AddChat( "Eye: " + Window.LocalPlayer.EyePosition );
			} else if( Utils.CaselessEquals( property, "target" ) ) {
				PickedPos pos = Window.SelectedPos;
				if( !pos.Valid ) {
					Window.AddChat( "Currently not targeting a block" );
				} else {
					Window.AddChat( "Currently targeting at: " + pos.BlockPos );
				}
			} else if( Utils.CaselessEquals( property, "dimensions" ) ) {
				Window.AddChat( "map width: " + Window.Map.Width );
				Window.AddChat( "map height: " + Window.Map.Height );
				Window.AddChat( "map length: " + Window.Map.Length );
			} else if( Utils.CaselessEquals( property, "jumpheight" ) ) {
				float jumpHeight = Window.LocalPlayer.JumpHeight;
				Window.AddChat( jumpHeight.ToString( "F2" ) + " blocks" );
			} else if( Utils.CaselessEquals( property, "viewdist" ) ) {
				int newDist;
				if( !reader.NextInt( out newDist ) ) {
					Window.AddChat( "View distance: " + Window.ViewDistance );
				} else if( newDist < 8 ) {
					Window.AddChat( "&e/client info: &cThat view distance is way too small." );
				} else if( newDist > 8192 ) {
					Window.AddChat( "&e/client info: &cThat view distance is way too large." );
				} else {
					Window.SetViewDistance( newDist );
				}
			} else {
				Window.AddChat( "&e/client info: Unrecognised property: \"&f" + property + "&e\"." );
			}
		}
	}
	
	public sealed class RenderTypeCommand : Command {
		
		public RenderTypeCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "RenderType"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client rendertype [normal/legacy/legacyfast]",
					"&bnormal: &eDefault renderer, with all environmental effects enabled.",
					"&bfast: &eMay be slightly slower than normal, but produces the same environmental effects.",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			string property = reader.Next();
			if( property == null ) {
				Window.AddChat( "&e/client rendertype: &cYou didn't specify a new render type." );
			} else if( Utils.CaselessEquals( property, "fast" ) ) {
				SetNewRenderType( true );
				Window.AddChat( "&e/client rendertype: &fRender type is now fast." );
			} else if( Utils.CaselessEquals( property, "normal" ) ) {
				SetNewRenderType( false );
				Window.AddChat( "&e/client rendertype: &fRender type is now normal." );
			}
		}
		
		void SetNewRenderType( bool minimal ) {
			Game game = Window;
			game.EnvRenderer.Dispose();
			if( minimal ) {
				game.EnvRenderer = new MinimalEnvRenderer( game );
			} else {
				game.EnvRenderer = new StandardEnvRenderer( game );
			}
			game.EnvRenderer.Init();
		}
	}
	
	public sealed class ChatFontSizeCommand : Command {
		
		public ChatFontSizeCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "ChatSize"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client chatsize [fontsize]",
					"&bfontsize: &eWhole number specifying the new font size for chat.",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			int fontSize;
			if( !reader.NextInt( out fontSize ) ) {
				Window.AddChat( "&e/client chatsize: &cInvalid font size." );
			} else {
				if( fontSize < 6 ) {
					Window.AddChat( "&e/client chatsize: &cFont size too small." );
					return;
				} else if( fontSize > 30 ) {
					Window.AddChat( "&e/client chatsize: &cFont size too big." );
					return;
				}
				Window.ChatFontSize = fontSize;
				Window.SetNewScreen( null );
				Window.chatInInputBuffer = null;
				Window.SetNewScreen( new NormalScreen( Window ) );
			}
		}
	}
	
	public sealed class MouseSensitivityCommand : Command {
		
		public MouseSensitivityCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "Sensitivity"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client sensitivity [mouse sensitivity]",
					"&bmouse sensitivity: &eInteger between 1 to 100 specifiying the mouse sensitivity.",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			int sensitivity;
			if( !reader.NextInt( out sensitivity ) ) {
				Window.AddChat( "&e/client sensitivity: Current sensitivity is: " + Window.MouseSensitivity );
			} else if( sensitivity < 1 || sensitivity > 100 ) {
				Window.AddChat( "&e/client sensitivity: &cMouse sensitivity must be between 1 to 100." );
			} else {
				Window.MouseSensitivity = sensitivity;
			}
		}
	}
	
	public sealed class PostProcessorCommand : Command {
		
		public PostProcessorCommand( Game window ) : base( window ) {
		}
		
		public override string Name {
			get { return "PostProcessor"; }
		}
		
		public override string[] Help {
			get {
				return new [] {
					"&a/client postprocessor [set/off/list/affect2d] <value>",
					"&blist: &ePrints a list of all potential post processing filters that can be used.",
					"&boff: &eTurns off the currently enabled post processing filter.",
					"&bset: &eSets the post processing filter to the specified name.",
					"&baffect2d: &eSets whether the post processing filter affects the gui. (true or false)",
				};
			}
		}
		
		public override void Execute( CommandReader reader ) {
			string param = reader.Next();
			if( String.IsNullOrEmpty( param ) ) {
				Window.AddChat( "&e/client postprocessor: &cYou didn't specify any parameter." );
			} else if( Utils.CaselessEquals( param, "off" ) ) {
				if( Window.PostProcessor != null ) {
					Window.PostProcessor.Dispose();
					Window.PostProcessor = null;
				}
			} else if( Utils.CaselessEquals( param, "list" ) ) {
				IEnumerable<string> parts = Utils.JoinItemsForChat( Window.PostProcessingShaders,
				                                                   type => type.Name );
				foreach( string part in parts ) {
					Window.AddChat( part );
				}
			} else if( Utils.CaselessEquals( param, "set" ) ) {
				string type = reader.Next();
				if( !String.IsNullOrEmpty( type ) ) {
					foreach( Type t in Window.PostProcessingShaders ) {
						if( Utils.CaselessEquals( type, t.Name ) ) {
							SetPostProcessor( t );
							return;
						}
					}
				}
				Window.AddChat( "&e/client postprocessor set: No post processor with name: \"&f" + type + "&e\"." );
			} else if( Utils.CaselessEquals( param, "affect2d" ) || Utils.CaselessEquals( param, "effect2d" ) ) {
				bool affectGui;
				if( !reader.NextOf( out affectGui, Boolean.TryParse ) ) {
					Window.AddChat( "&e/client postprocessor affect2d: Unrecognised value." );
				} else if( Window.PostProcessor == null ) {
					Window.AddChat( "&e/client postprocessor affect2d: &cNo post processor is active." );
				} else {
					Window.PostProcessor.ApplyToGui = affectGui;
				}
			} else {
				Window.AddChat( "&e/client postprocessor: Unrecognised parameter: \"&f" + param + "&e\"." );
			}
		}
		
		void SetPostProcessor( Type type ) {
			if( Window.PostProcessor == null ) {
				Window.PostProcessor = new PostProcessor( Window );
				Window.PostProcessor.Init();
			}
			Window.PostProcessor.SetShader( Utils.New<PostProcessingShader>( type ) );
		}
	}
}
