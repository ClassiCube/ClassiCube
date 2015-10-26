using System;
using System.Collections.Generic;
using System.Text;
using ClassicalSharp.Renderers;
using OpenTK.Input;

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
				game.Chat.Add( part );
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
				game.Chat.Add( "&e/client help: No command name specified. See /client commands for a list of commands." );
			} else {
				Command cmd = game.CommandManager.GetMatchingCommand( cmdName );
				if( cmd != null ) {
					string[] help = cmd.Help;
					for( int i = 0; i < help.Length; i++ ) {
						game.Chat.Add( help[i] );
					}
				}
			}
		}
	}
	
	/// <summary> Command that displays information about the user's GPU. </summary>
	public sealed class GpuInfoCommand : Command {
		
		public GpuInfoCommand() {
			Name = "GpuInfo";
			Help = new [] {
				"&a/client gpuinfo",
				"&eDisplays information about your GPU.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			foreach( string line in game.Graphics.ApiInfo ) {
				game.Chat.Add( "&a" + line );
			}
			throw new InvalidCastException();
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
				game.Chat.Add( "&e/client info: &cYou didn't specify a property." );
			} else if( Utils.CaselessEquals( property, "pos" ) ) {
				game.Chat.Add( "Feet: " + game.LocalPlayer.Position );
				game.Chat.Add( "Eye: " + game.LocalPlayer.EyePosition );
			} else if( Utils.CaselessEquals( property, "target" ) ) {
				PickedPos pos = game.SelectedPos;
				if( !pos.Valid ) {
					game.Chat.Add( "Currently not targeting a block" );
				} else {
					game.Chat.Add( "Currently targeting at: " + pos.BlockPos );
				}
			} else if( Utils.CaselessEquals( property, "dimensions" ) ) {
				game.Chat.Add( "map width: " + game.Map.Width );
				game.Chat.Add( "map height: " + game.Map.Height );
				game.Chat.Add( "map length: " + game.Map.Length );
			} else if( Utils.CaselessEquals( property, "jumpheight" ) ) {
				float jumpHeight = game.LocalPlayer.JumpHeight;
				game.Chat.Add( jumpHeight.ToString( "F2" ) + " blocks" );
			} else {
				game.Chat.Add( "&e/client info: Unrecognised property: \"&f" + property + "&e\"." );
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
				"&bnormalfast: &eSacrifices clouds, fog and overhead sky for faster performance.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string property = reader.Next();
			if( property == null ) {
				game.Chat.Add( "&e/client rendertype: &cYou didn't specify a new render type." );
			} else if( Utils.CaselessEquals( property, "legacyfast" ) ) {
				SetNewRenderType( true, true );
				game.Chat.Add( "&e/client rendertype: &fRender type is now fast legacy." );
			} else if( Utils.CaselessEquals( property, "legacy" ) ) {
				SetNewRenderType( true, false );
				game.Chat.Add( "&e/client rendertype: &fRender type is now legacy." );
			} else if( Utils.CaselessEquals( property, "normal" ) ) {
				SetNewRenderType( false, false );
				game.Chat.Add( "&e/client rendertype: &fRender type is now normal." );
			} else if( Utils.CaselessEquals( property, "normalfast" ) ) {
				SetNewRenderType( false, true );
				game.Chat.Add( "&e/client rendertype: &fRender type is now normalfast." );
			}
		}
		
		void SetNewRenderType( bool legacy, bool minimal ) {
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
				((StandardEnvRenderer)game.EnvRenderer).SetUseLegacyMode( legacy );
			}
		}
	}
}
