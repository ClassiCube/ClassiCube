using System;

namespace OpenTK {
	
	/// <summary> Placeholder for System.Diagnostics.Debug class because it crashes on some Mono version on Linux. </summary>
	public static class Debug {
		
		public static void Print( string text ) {
			try {
				Console.WriteLine( text );
			} catch( Exception ) {
			} // raised by Mono sometimes when trying to write to console from the finalizer thread.
		}
		
		public static void Print( object arg ) {
			try {
				Console.WriteLine( arg );
			} catch( Exception ) {
			}
		}
		
		public static void Print( string text, params object[] args ) {
			try {
				Console.WriteLine( text, args );
			} catch( Exception ) {
			}
		}
	}
}
