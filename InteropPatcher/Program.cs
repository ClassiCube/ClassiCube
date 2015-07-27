using System;

namespace InteropPatcher {
	
	public static class Program {

		public static int Main( string[] args ) {
			try {
				if( args.Length != 1 ) {
					Console.WriteLine( "Expecting single argument specifying the file to patch" );
					return 1;
				}
				Patcher.PatchFile( args[0] );
				return 0;
			} catch( Exception ex ) {
				Console.WriteLine( ex );
				return 1;
			}
		}
	}
}