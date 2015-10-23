using System;
using System.Collections.Generic;

namespace ClassicalSharp.Commands {
	
	public class CommandManager {
		
		public static bool IsCommandPrefix( string input ) {
			return Utils.CaselessStarts( input, "/client" );
		}
		
		protected Game game;
		public List<Command> RegisteredCommands = new List<Command>();
		public void Init( Game game ) {
			this.game = game;			
			RegisterCommand( new CommandsCommand() );
			RegisterCommand( new HelpCommand() );
			RegisterCommand( new InfoCommand() );
			RegisterCommand( new RenderTypeCommand() );
		}
		
		public void RegisterCommand( Command command ) {
			command.game = game;
			foreach( Command cmd in RegisteredCommands ) {
				if( Utils.CaselessEquals( cmd.Name, command.Name ) ) {
					throw new InvalidOperationException( "Another command already has name : " + command.Name );
				}
			}
			RegisteredCommands.Add( command );
		}
		
		public Command GetMatchingCommand( string commandName ) {
			bool matchFound = false;
			Command matchingCommand = null;
			foreach( Command cmd in RegisteredCommands ) {
				if( Utils.CaselessStarts( cmd.Name, commandName ) ) {
					if( matchFound ) {
						game.Chat.Add( "&e/client: Multiple commands found that start with: \"&f" + commandName + "&e\"." );
						return null;
					}
					matchFound = true;
					matchingCommand = cmd;
				}
			}
			
			if( matchingCommand == null ) {
				game.Chat.Add( "&e/client: Unrecognised command: \"&f" + commandName + "&e\"." );
			}
			return matchingCommand;
		}
		
		public void Execute( string text ) {
			CommandReader reader = new CommandReader( text );
			if( reader.TotalArgs == 0 ) {
				game.Chat.Add( "&e/client: No command name specified. See /client commands for a list of commands." );
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
