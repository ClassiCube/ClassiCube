using System;

namespace ClassicalSharp.Commands {
	
	public abstract class Command {
		
		public abstract string Name { get; }
		
		public virtual string[] Help {
			get { return new [] { "No help is available for this command." }; }
		}
		
		public Game Window;
		
		public Command( Game window ) {
			Window = window;
		}
		
		public abstract void Execute( CommandReader reader );
	}
}
