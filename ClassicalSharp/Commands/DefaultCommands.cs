// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
			Help = new string[] {
				"&a/client commands",
				"&ePrints a list of all usable commands"
			};
		}
		
		public override void Execute( string[] args ) {
			game.CommandList.PrintDefinedCommands( game );
		}
	}
	
	/// <summary> Command that displays information about an input client command. </summary>
	public sealed class HelpCommand : Command {
		
		public HelpCommand() {
			Name = "Help";
			Help = new string[] {
				"&a/client help [command name]",
				"&eDisplays the help for the given command.",
			};
		}
		
		public override void Execute( string[] args ) {
			if( args.Length == 1 ) {
				game.Chat.Add( "&eList of client commands:" );
				game.CommandList.PrintDefinedCommands( game );
				game.Chat.Add( "&eTo see a particular command's help, type /client help [cmd name]" );
			} else {
				Command cmd = game.CommandList.GetMatch( args[1] );
				if( cmd == null ) return;
				string[] help = cmd.Help;
				for( int i = 0; i < help.Length; i++ )
					game.Chat.Add( help[i] );
			}
		}
	}
	
	/// <summary> Command that displays information about the user's GPU. </summary>
	public sealed class GpuInfoCommand : Command {
		
		public GpuInfoCommand() {
			Name = "GpuInfo";
			Help = new string[] {
				"&a/client gpuinfo",
				"&eDisplays information about your GPU.",
			};
		}
		
		public override void Execute( string[] args ) {
			string[] lines = game.Graphics.ApiInfo;
			for( int i = 0; i < lines.Length; i++ )
				game.Chat.Add( "&a" + lines[i] );
		}
	}
	
	public sealed class RenderTypeCommand : Command {
		
		public RenderTypeCommand() {
			Name = "RenderType";
			Help = new string[] {
				"&a/client rendertype [normal/legacy/legacyfast]",
				"&bnormal: &eDefault renderer, with all environmental effects enabled.",
				"&blegacy: &eMay be slightly slower than normal, but produces the same environmental effects.",
				"&blegacyfast: &eSacrifices clouds, fog and overhead sky for faster performance.",
				"&bnormalfast: &eSacrifices clouds, fog and overhead sky for faster performance.",
			};
		}
		
		public override void Execute( string[] args ) {
			if( args.Length == 1 ) {
				game.Chat.Add( "&e/client: &cYou didn't specify a new render type." );
			} else if( game.SetRenderType( args[1] ) ) {
				game.Chat.Add( "&e/client: &fRender type is now " + args[1] + "." );
			} else {
				game.Chat.Add( "&e/client: &cUnrecognised render type &f\"" + args[1] + "\"&c." );
			}
		}
	}
}
