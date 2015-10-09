#define DEBUG_OPENTK
using System;

namespace OpenTK {
	
	/// <summary> Placeholder for System.Diagnostics.Debug class because it crashes on some Mono version on Linux. </summary>
	public static class Debug {
		
		public static void Assert( bool condition ) {
			if( !condition )
				throw new InvalidOperationException( "Assertion failed!" );
		}
		
		public static void Print( string text ) {
			#if DEBUG_OPENTK
			Console.WriteLine( text );
			#endif
		}
		
		public static void Print( object arg ) {
			#if DEBUG_OPENTK
			Console.WriteLine( arg );
			#endif
		}
		
		public static void Print( string text, params object[] args ) {
			#if DEBUG_OPENTK
			Console.WriteLine( text, args );
			#endif
		}
	}
}
