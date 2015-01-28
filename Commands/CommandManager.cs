using System;
using System.Collections.Generic;

namespace ClassicalSharp.Commands {
	
	public class CommandManager {
		
		public static bool IsCommandPrefix( string input ) {
			return input.StartsWith( "/client", StringComparison.OrdinalIgnoreCase );
		}
		
		public Game Window;
		public List<Command> RegisteredCommands = new List<Command>();
		public void Init( Game game ) {
			Window = game;			
			RegisterCommand( new CommandsCommand() );
			RegisterCommand( new HelpCommand() );
			RegisterCommand( new EnvCommand() );
			RegisterCommand( new InfoCommand() );
			RegisterCommand( new RenderTypeCommand() );
			RegisterCommand( new ChatFontSizeCommand() );
		}
		
		void RegisterCommand( Command command ) {
			command.Window = Window;
			foreach( Command cmd in RegisteredCommands ) {
				if( command.Name.Equals( cmd.Name, StringComparison.OrdinalIgnoreCase ) ) {
					throw new InvalidOperationException( "Another command already has name : " + command.Name );
				}
			}
			RegisteredCommands.Add( command );
		}
		
		public Command GetMatchingCommand( string commandName ) {
			bool matchFound = false;
			Command matchingCommand = null;
			foreach( Command cmd in RegisteredCommands ) {
				if( cmd.Name.StartsWith( commandName, StringComparison.OrdinalIgnoreCase ) ) {
					if( matchFound ) {
						Window.AddChat( "§e/client: Multiple commands found that start with: \"§f" + commandName + "§e\"." );
						return null;
					}
					matchFound = true;
					matchingCommand = cmd;
				}
			}
			
			if( matchingCommand == null ) {
				Window.AddChat( "§e/client: Unrecognised command: \"§f" + commandName + "§e\"." );
			}
			return matchingCommand;
		}
		
		public void Execute( string text ) {
			CommandReader reader = new CommandReader( text );
			if( reader.TotalArgs == 0 ) {
				Window.AddChat( "§e/client: No command name specified. §fSee /client commands for a list of commands." );
				return;
			}
			string commandName = reader.Next();
			Command cmd = GetMatchingCommand( commandName );
			if( cmd != null ) {
				cmd.Execute( reader );
			}
		}
	}
}
