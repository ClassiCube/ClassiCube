using System;

namespace OpenTK {
	
	/// <summary> Placeholder for System.Diagnostics.Debug class because it crashes on some Mono version on Linux. </summary>
	public static class Debug {
		
		public static void Assert( bool condition ) {
			if( !condition )
				throw new InvalidOperationException( "Assertion failed!" );
		}
		
		public static void Print( string text ) {
			Console.WriteLine( text );
		}
		
		public static void Print( object arg ) {
			Console.WriteLine( arg );
		}
		
		public static void Print( string text, params object[] args ) {
			Console.WriteLine( text, args );
		}
	}
}
