using System;

namespace ClassicalSharp.Commands {
	
	public abstract class Command {
		
		public string Name { get; set; }
		
		public string[] Help { get; set; }
		
		protected internal Game game;
		
		public abstract void Execute( CommandReader reader );
	}
}
