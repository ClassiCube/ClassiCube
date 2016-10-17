// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Commands {
	
	/// <summary> Represents a client side action that optionally accepts arguments. </summary>
	public abstract class Command {
		
		/// <summary> Full command name, note that the user does not 
		/// have to fully type this into chat. </summary>
		public string Name;
		
		/// <summary> Provides help about the purpose and syntax of this 
		/// command. Can use colour codes. </summary>
		public string[] Help;
		
		protected internal Game game;
		
		public abstract void Execute( string[] args );
	}
}
