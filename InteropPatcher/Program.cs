using System;

namespace InteropPatcher {
	
	public static class Program {

		public static int Main( string[] args ) {
			try {
				if( args.Length == 0 ) {
					Console.WriteLine( "Expecting single argument specifying the file to patch" );
					return 2;
				}
				// Some older IDEs seem to like splitting the path when it has spaces.
				// So we undo this and treat the arguments as a single path.
				string path = String.Join( " ", args );
				Patcher.PatchFile( path );
				return 0;
			} catch( Exception ex ) {
				Console.WriteLine( ex );
				return 1;
			}
		}
	}
}