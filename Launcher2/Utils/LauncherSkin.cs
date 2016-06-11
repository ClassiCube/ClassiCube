// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {

	public static class LauncherSkin {
		
		public static FastColour BackgroundCol = new FastColour( 153, 127, 172 );
		public static FastColour ButtonBorderCol = new FastColour( 97, 81, 110 );
		public static FastColour ButtonForeActiveCol = new FastColour( 189, 168, 206 );
		public static FastColour ButtonForeCol = new FastColour( 141, 114, 165 );
		public static FastColour ButtonHighlightCol = new FastColour( 162, 131, 186 );
		
		public static void ResetToDefault() {
			BackgroundCol = new FastColour( 153, 127, 172 );
			ButtonBorderCol = new FastColour( 97, 81, 110 );
			ButtonForeActiveCol = new FastColour( 189, 168, 206 );
			ButtonForeCol = new FastColour( 141, 114, 165 );
			ButtonHighlightCol = new FastColour( 162, 131, 186 );
		}
		
		public static void LoadFromOptions() {
			Get( "launcher-back-col", ref BackgroundCol );
			Get( "launcher-btn-border-col", ref ButtonBorderCol );
			Get( "launcher-btn-fore-active-col", ref ButtonForeActiveCol );
			Get( "launcher-btn-fore-inactive-col", ref ButtonForeCol );
			Get( "launcher-btn-highlight-inactive-col", ref ButtonHighlightCol );
		}
		
		public static void SaveToOptions() {
			Options.Set( "launcher-back-col", BackgroundCol.ToRGBHexString() );
			Options.Set( "launcher-btn-border-col", ButtonBorderCol.ToRGBHexString() );
			Options.Set( "launcher-btn-fore-active-col", ButtonForeActiveCol.ToRGBHexString() );
			Options.Set( "launcher-btn-fore-inactive-col", ButtonForeCol.ToRGBHexString() );
			Options.Set( "launcher-btn-highlight-inactive-col", ButtonHighlightCol.ToRGBHexString() );
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