using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public static class LauncherSkin {
		
		public static FastColour BackgroundCol = new FastColour( 127, 107, 140 );
		//new FastColour( 104, 87, 119 );
		
		public static FastColour ButtonBorderCol = new FastColour( 97, 81, 110 );
		public static FastColour ButtonForeActiveCol = new FastColour( 189, 168, 206 );
		public static FastColour ButtonForeCol = new FastColour( 164, 138, 186 );
		public static FastColour ButtonHighlightCol = new FastColour( 182, 158, 201 );
		
		public static void ResetToDefault() {
			BackgroundCol = new FastColour( 127, 107, 140 );
			ButtonBorderCol = new FastColour( 97, 81, 110 );
			ButtonForeActiveCol = new FastColour( 189, 168, 206 );
			ButtonForeCol = new FastColour( 164, 138, 186 );
			ButtonHighlightCol = new FastColour( 182, 158, 201 );
		}
		
		public static void LoadFromOptions() {
			Get( "launcher-backcol", ref BackgroundCol );
			Get( "launcher-button-backcol", ref ButtonBorderCol );
			Get( "launcher-button-foreactivecol", ref ButtonForeActiveCol );
			Get( "launcher-button-forecol", ref ButtonForeCol );
			Get( "launcher-button-highlightcol", ref ButtonHighlightCol );
		}
		
		public static void SaveToOptions() {
			Options.Set( "launcher-backcol", BackgroundCol.ToRGBHexString() );
			Options.Set( "launcher-button-backcol", ButtonBorderCol.ToRGBHexString() );
			Options.Set( "launcher-button-foreactivecol", ButtonForeActiveCol.ToRGBHexString() );
			Options.Set( "launcher-button-forecol", ButtonForeCol.ToRGBHexString() );
			Options.Set( "launcher-button-highlightcol", ButtonHighlightCol.ToRGBHexString() );
		}
		
		static void Get( string key, ref FastColour col ) {
			FastColour defaultCol = col;
			string value = Options.Get( key );
			if( String.IsNullOrEmpty( value ) ) return;
			
			if( !FastColour.TryParse( value, out col ) )
				col = defaultCol;
		}
	}
}