using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public static class LauncherSkin {
		
		public static FastColour BackgroundCol = new FastColour( 153, 127, 172 );
		public static FastColour ButtonBorderCol = new FastColour( 97, 81, 110 );
		public static FastColour ButtonForeActiveCol = new FastColour( 189, 168, 206 );
		public static FastColour ButtonForeCol = new FastColour( 164, 138, 186 );
		public static FastColour ButtonHighlightCol = new FastColour( 182, 158, 201 );
		
		public static void ResetToDefault() {
			BackgroundCol = new FastColour( 153, 127, 172 );
			ButtonBorderCol = new FastColour( 97, 81, 110 );
			ButtonForeActiveCol = new FastColour( 189, 168, 206 );
			ButtonForeCol = new FastColour( 164, 138, 186 );
			ButtonHighlightCol = new FastColour( 182, 158, 201 );
		}
		
		public static void LoadFromOptions() {
			Get( "launcher-back-col", ref BackgroundCol );
			Get( "launcher-button-border-col", ref ButtonBorderCol );
			Get( "launcher-button-fore-active-col", ref ButtonForeActiveCol );
			Get( "launcher-button-fore-col", ref ButtonForeCol );
			Get( "launcher-button-highlight-col", ref ButtonHighlightCol );
		}
		
		public static void SaveToOptions() {
			Options.Set( "launcher-back-col", BackgroundCol.ToRGBHexString() );
			Options.Set( "launcher-button-border-col", ButtonBorderCol.ToRGBHexString() );
			Options.Set( "launcher-button-fore-active-col", ButtonForeActiveCol.ToRGBHexString() );
			Options.Set( "launcher-button-fore-col", ButtonForeCol.ToRGBHexString() );
			Options.Set( "launcher-button-highlight-col", ButtonHighlightCol.ToRGBHexString() );
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