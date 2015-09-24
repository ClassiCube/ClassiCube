using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using ClassicalSharp.TexturePack;
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
	
	/// <summary> Command that changes the client's texture pack. </summary>
	public sealed class TexturePackCommand : Command {
		
		public TexturePackCommand() {
			Name = "TexturePack";
			Help = new [] {
				"&a/client texturepack [path]",
				"&bpath: &eLoads a texture pack from the specified path.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string path = reader.NextAll();
			if( String.IsNullOrEmpty( path ) ) return;
			
			try {
				TexturePackExtractor extractor = new TexturePackExtractor();
				extractor.Extract( path, game );
			} catch( FileNotFoundException ) {
				game.AddChat( "&e/client texturepack: Couldn't find file \"" + path + "\"" );
			}
		}
	}
}
