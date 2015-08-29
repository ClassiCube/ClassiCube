using System;
using System.Collections.Generic;
using System.Text;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Commands {
	
	/// <summary> Command that displays the list of all currently registered client commands. </summary>
	public sealed class CommandsCommand : Command {
		
		public CommandsCommand() {
			Name = "Commands";
			Help = new [] {
				"&a/client commands",
				"&ePrints a list of all usable commands"
			};
		}
		
		public override void Execute( CommandReader reader ) {
			List<string> commandNames = new List<string>();
			StringBuilder buffer = new StringBuilder( 64 );
			foreach( Command cmd in Window.CommandManager.RegisteredCommands ) {
				string name = cmd.Name;
				if( buffer.Length + name.Length > 64 ) {
					commandNames.Add( buffer.ToString() );
					buffer.Length = 0;
				}
				buffer.Append( name + ", " );
			}
			if( buffer.Length > 0 ) {
				commandNames.Add( buffer.ToString() );
			}
			foreach( string part in commandNames ) {
				Window.AddChat( part );
			}
		}
	}
	
	/// <summary> Command that modifies various aspects of the environment of the current map. </summary>
	public sealed class EnvCommand : Command {
		
		public EnvCommand() {
			Name = "Env";
			Help = new [] {
				"&a/client env [property] [value]",
				"&bproperties: &eskycol, fogcol, cloudscol, cloudsspeed,",
				"     &esuncol, shadowcol, weather",
				"&bcol properties: &evalue must be a hex colour code (either #RRGGBB or RRGGBB).",
				"&bcloudsspeed: &evalue must be a decimal number. (e.g. 0.5, 1, 4.5).",
				"&eIf no args are given, the current env variables will be displayed.",
			};
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
	
	/// <summary> Command that displays information about an input client command. </summary>
	public sealed class HelpCommand : Command {
		
		public HelpCommand() {
			Name = "Help";
			Help = new [] {
				"&a/client help [command name]",
				"&eDisplays the help for the given command.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string cmdName = reader.Next();
			if( cmdName == null ) {
				Window.AddChat( "&e/client help: No command name specified. See /client commands for a list of commands." );
			} else {
				Command cmd = Window.CommandManager.GetMatchingCommand( cmdName );
				if( cmd != null ) {
					string[] help = cmd.Help;
					for( int i = 0; i < help.Length; i++ ) {
						Window.AddChat( help[i] );
					}
				}
			}
		}
	}
	
	public sealed class InfoCommand : Command {
		
		public InfoCommand() {
			Name = "Info";
			Help = new [] {
				"&a/client info [property]",
				"&bproperties: &epos, target, dimensions, jumpheight",
			};
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
			} else {
				Window.AddChat( "&e/client info: Unrecognised property: \"&f" + property + "&e\"." );
			}
		}
	}
	
	public sealed class RenderTypeCommand : Command {
		
		public RenderTypeCommand() {
			Name = "RenderType";
			Help = new [] {
				"&a/client rendertype [normal/legacy/legacyfast]",
				"&bnormal: &eDefault renderer, with all environmental effects enabled.",
				"&blegacy: &eMay be slightly slower than normal, but produces the same environmental effects.",
				"&blegacyfast: &eSacrifices clouds, fog and overhead sky for faster performance.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string property = reader.Next();
			if( property == null ) {
				Window.AddChat( "&e/client rendertype: &cYou didn't specify a new render type." );
			} else if( Utils.CaselessEquals( property, "legacyfast" ) ) {
				SetNewRenderType( true, true, true );
				Window.AddChat( "&e/client rendertype: &fRender type is now fast legacy." );
			} else if( Utils.CaselessEquals( property, "legacy" ) ) {
				SetNewRenderType( true, false, true );
				Window.AddChat( "&e/client rendertype: &fRender type is now legacy." );
			} else if( Utils.CaselessEquals( property, "normal" ) ) {
				SetNewRenderType( false, false, false );
				Window.AddChat( "&e/client rendertype: &fRender type is now normal." );
			}
		}
		
		void SetNewRenderType( bool legacy, bool minimal, bool legacyEnv ) {
			Game game = Window;
			game.MapEnvRenderer.SetUseLegacyMode( legacy );
			if( minimal ) {
				game.EnvRenderer.Dispose();
				game.EnvRenderer = new MinimalEnvRenderer( game );
				game.EnvRenderer.Init();
			} else {
				if( !( game.EnvRenderer is StandardEnvRenderer ) ) {
					game.EnvRenderer.Dispose();
					game.EnvRenderer = new StandardEnvRenderer( game );
					game.EnvRenderer.Init();
				}
				((StandardEnvRenderer)game.EnvRenderer).SetUseLegacyMode( legacyEnv );
			}
		}
	}
	
	/// <summary> Command that modifies the font size of chat in the normal gui screen. </summary>
	public sealed class ChatFontSizeCommand : Command {
		
		public ChatFontSizeCommand() {
			Name = "ChatSize";
			Help = new [] {
				"&a/client chatsize [fontsize]",
				"&bfontsize: &eWhole number specifying the new font size for chat.",
			};
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
	
	/// <summary> Command that modifies how sensitive the client is to changes in the mouse position. </summary>
	public sealed class MouseSensitivityCommand : Command {
		
		public MouseSensitivityCommand() {
			Name = "Sensitivity";
			Help = new [] {
				"&a/client sensitivity [mouse sensitivity]",
				"&bmouse sensitivity: &eInteger between 1 to 100 specifiying the mouse sensitivity.",
			};
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
	
	/// <summary> Command that modifies how far the client can see. </summary>
	public sealed class ViewDistanceCommand : Command {
		
		public ViewDistanceCommand() {
			Name = "ViewDistance";
			Help = new [] {
				"&a/client viewdistance [range]",
				"&brange: &eInteger specifying how far you can see before fog appears.",
				"&eThe minimum range is 8, the maximum is 4096.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			int newDist;
			if( !reader.NextInt( out newDist ) ) {
				Window.AddChat( "View distance: " + Window.ViewDistance );
			} else if( newDist < 8 ) {
				Window.AddChat( "&e/client viewdistance: &cThat view distance is way too small." );
			} else if( newDist > 4096 ) {
				Window.AddChat( "&e/client viewdistance: &cThat view distance is way too large." );
			} else {
				Window.SetViewDistance( newDist );
			}
		}
	}
}
