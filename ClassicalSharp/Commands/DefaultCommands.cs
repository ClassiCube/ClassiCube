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
			foreach( Command cmd in game.CommandManager.RegisteredCommands ) {
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
				game.AddChat( part );
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
				game.AddChat( "Fog colour: " + game.Map.FogCol.ToRGBHexString() );
				game.AddChat( "Clouds colour: " + game.Map.CloudsCol.ToRGBHexString() );
				game.AddChat( "Sky colour: " + game.Map.SkyCol.ToRGBHexString() );
			} else if( Utils.CaselessEquals( property, "skycol" ) ) {
				ReadHexColourAnd( reader, c => game.Map.SetSkyColour( c ) );
			} else if( Utils.CaselessEquals( property, "fogcol" ) ) {
				ReadHexColourAnd( reader, c => game.Map.SetFogColour( c ) );
			} else if( Utils.CaselessEquals( property, "cloudscol" )
			          || Utils.CaselessEquals( property, "cloudcol" ) ) {
				ReadHexColourAnd( reader, c => game.Map.SetCloudsColour( c ) );
			} else if( Utils.CaselessEquals( property, "suncol" ) ) {
				ReadHexColourAnd( reader, c => game.Map.SetSunlight( c ) );
			} else if( Utils.CaselessEquals( property, "shadowcol" ) ) {
				ReadHexColourAnd( reader, c => game.Map.SetShadowlight( c ) );
			} else if( Utils.CaselessEquals( property, "cloudsspeed" )
			          || Utils.CaselessEquals( property, "cloudspeed" ) ) {
				float speed;
				if( !reader.NextFloat( out speed ) ) {
					game.AddChat( "&e/client env: &cInvalid clouds speed." );
				} else {
					StandardEnvRenderer env = game.EnvRenderer as StandardEnvRenderer;
					if( env != null ) {
						env.CloudsSpeed = speed;
					}
				}
			} else if( Utils.CaselessEquals( property, "weather" ) ) {
				int weather;
				if( !reader.NextInt( out weather ) || weather < 0 || weather > 2 ) {
					game.AddChat( "&e/client env: &cInvalid weather." );
				} else {
					game.Map.SetWeather( (Weather)weather );
				}
			}
		}
		
		void ReadHexColourAnd( CommandReader reader, Action<FastColour> action ) {
			FastColour colour;
			if( !reader.NextHexColour( out colour ) ) {
				game.AddChat( "&e/client env: &cInvalid hex colour." );
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
				game.AddChat( "&e/client help: No command name specified. See /client commands for a list of commands." );
			} else {
				Command cmd = game.CommandManager.GetMatchingCommand( cmdName );
				if( cmd != null ) {
					string[] help = cmd.Help;
					for( int i = 0; i < help.Length; i++ ) {
						game.AddChat( help[i] );
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
				game.AddChat( "&e/client info: &cYou didn't specify a property." );
			} else if( Utils.CaselessEquals( property, "pos" ) ) {
				game.AddChat( "Feet: " + game.LocalPlayer.Position );
				game.AddChat( "Eye: " + game.LocalPlayer.EyePosition );
			} else if( Utils.CaselessEquals( property, "target" ) ) {
				PickedPos pos = game.SelectedPos;
				if( !pos.Valid ) {
					game.AddChat( "Currently not targeting a block" );
				} else {
					game.AddChat( "Currently targeting at: " + pos.BlockPos );
				}
			} else if( Utils.CaselessEquals( property, "dimensions" ) ) {
				game.AddChat( "map width: " + game.Map.Width );
				game.AddChat( "map height: " + game.Map.Height );
				game.AddChat( "map length: " + game.Map.Length );
			} else if( Utils.CaselessEquals( property, "jumpheight" ) ) {
				float jumpHeight = game.LocalPlayer.JumpHeight;
				game.AddChat( jumpHeight.ToString( "F2" ) + " blocks" );
			} else {
				game.AddChat( "&e/client info: Unrecognised property: \"&f" + property + "&e\"." );
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
				game.AddChat( "&e/client rendertype: &cYou didn't specify a new render type." );
			} else if( Utils.CaselessEquals( property, "legacyfast" ) ) {
				SetNewRenderType( true, true, true );
				game.AddChat( "&e/client rendertype: &fRender type is now fast legacy." );
			} else if( Utils.CaselessEquals( property, "legacy" ) ) {
				SetNewRenderType( true, false, true );
				game.AddChat( "&e/client rendertype: &fRender type is now legacy." );
			} else if( Utils.CaselessEquals( property, "normal" ) ) {
				SetNewRenderType( false, false, false );
				game.AddChat( "&e/client rendertype: &fRender type is now normal." );
			}
		}
		
		void SetNewRenderType( bool legacy, bool minimal, bool legacyEnv ) {
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
				game.AddChat( "&e/client chatsize: &cInvalid font size." );
			} else {
				if( fontSize < 6 ) {
					game.AddChat( "&e/client chatsize: &cFont size too small." );
					return;
				} else if( fontSize > 30 ) {
					game.AddChat( "&e/client chatsize: &cFont size too big." );
					return;
				}
				game.ChatFontSize = fontSize;
				game.SetNewScreen( null );
				game.chatInInputBuffer = null;
				game.SetNewScreen( new NormalScreen( game ) );
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
				game.AddChat( "&e/client sensitivity: Current sensitivity is: " + game.MouseSensitivity );
			} else if( sensitivity < 1 || sensitivity > 100 ) {
				game.AddChat( "&e/client sensitivity: &cMouse sensitivity must be between 1 to 100." );
			} else {
				game.MouseSensitivity = sensitivity;
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
				game.AddChat( "View distance: " + game.ViewDistance );
			} else if( newDist < 8 ) {
				game.AddChat( "&e/client viewdistance: &cThat view distance is way too small." );
			} else if( newDist > 4096 ) {
				game.AddChat( "&e/client viewdistance: &cThat view distance is way too large." );
			} else {
				game.SetViewDistance( newDist );
			}
		}
	}
}
