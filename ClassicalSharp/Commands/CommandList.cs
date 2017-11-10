// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;

namespace ClassicalSharp.Commands {
	
	public class CommandList : IGameComponent {
		
		const string prefix = "/client";
		public bool IsCommandPrefix(string input) {
			if (game.Server.IsSinglePlayer && Utils.CaselessStarts(input, "/"))
				return true;
			
			return Utils.CaselessStarts(input, prefix + " ")
				|| Utils.CaselessEquals(input, prefix);
		}
		
		protected Game game;
		public List<Command> RegisteredCommands = new List<Command>();
		public void Init(Game game) {
			this.game = game;
			Register(new CommandsCommand());
			Register(new GpuInfoCommand());
			Register(new HelpCommand());
			Register(new RenderTypeCommand());
			Register(new ResolutionCommand());
			
			if (!game.Server.IsSinglePlayer) return;
			Register(new ModelCommand());
			Register(new CuboidCommand());
			Register(new TeleportCommand());
		}

		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		public void Register(Command command) {
			command.game = game;
			for (int i = 0; i < RegisteredCommands.Count; i++) {
				Command cmd = RegisteredCommands[i];
				if (Utils.CaselessEquals(cmd.Name, command.Name))
					throw new InvalidOperationException("Another command already has name : " + command.Name);
			}
			RegisteredCommands.Add(command);
		}
		
		public Command GetMatch(string cmdName) {
			Command match = null;
			for (int i = 0; i < RegisteredCommands.Count; i++) {
				Command cmd = RegisteredCommands[i];
				if (!Utils.CaselessStarts(cmd.Name, cmdName)) continue;
				
				if (match != null) {
					game.Chat.Add("&e/client: Multiple commands found that start with: \"&f" + cmdName + "&e\".");
					return null;
				}
				match = cmd;
			}
			
			if (match == null)
				game.Chat.Add("&e/client: Unrecognised command: \"&f" + cmdName + "&e\".");
			return match;
		}
		
		static char[] splitChar = { ' ' };
		public void Execute(string text) {
			if (Utils.CaselessStarts(text, prefix)) { // /client command args
				text = text.Substring(prefix.Length).TrimStart(splitChar);
			} else { // /command args
				text = text.Substring(1);
			}
			
			if (text.Length == 0) { // only / or /client
				game.Chat.Add("&eList of client commands:");
				PrintDefinedCommands(game);
				game.Chat.Add("&eTo see a particular command's help, type /client help [cmd name]");
				return;
			}
			
			string[] args = text.Split(splitChar);
			Command cmd = GetMatch(args[0]);
			if (cmd == null) return;
			cmd.Execute(args);
		}
		
		public void PrintDefinedCommands(Game game) {
			StringBuffer sb = new StringBuffer(Utils.StringLength);
			int index = 0;
			
			for (int i = 0; i < RegisteredCommands.Count; i++) {
				Command cmd = RegisteredCommands[i];
				string name = cmd.Name;
				
				if ((sb.Length + name.Length + 2) > sb.Capacity) {
					game.Chat.Add(sb.ToString());
					sb.Clear();
					index = 0;
				}
				sb.Append(ref index, name);
				sb.Append(ref index, ", ");
			}
			
			if (sb.Length > 0)
				game.Chat.Add(sb.ToString());
		}
		
		public void Dispose() {
			RegisteredCommands.Clear();
		}
	}
}
