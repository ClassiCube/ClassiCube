using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public static class LauncherSkin {
		
		public static FastColour BackgroundCol = new FastColour( 127, 107, 140 ); 
		//new FastColour( 104, 87, 119 );
		
		
		public static void LoadFromOptions() {
			Get( "launcher-back-col", ref BackgroundCol );
		}
		
		static void Get( string key, ref FastColour col ) {
			FastColour defaultCol = col;
			string value = Options.Get( key );
			if( String.IsNullOrEmpty( value ) ) return;
			
			if( !FastColour.TryParse( value, out col ) )
				col = defaultCol;
		}
		
		public static void SaveToOptions() {
			Options.Set( "launcher-back-col", BackgroundCol.ToRGBHexString() );
		}
	}
}