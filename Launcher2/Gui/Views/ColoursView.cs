// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using OpenTK.Input;

namespace Launcher.Gui.Views {
	
	public sealed class ColoursView : IView {
		
		public ColoursView( LauncherWindow game ) : base( game ) {
			widgets = new LauncherWidget[25];
		}
		internal int defIndex;

		public override void DrawAll() {
			UpdateWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
		}
		
		public override void Init() {
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			inputFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
			UpdateWidgets();
		}
		
		void UpdateWidgets() {
			widgetIndex = 0;
			MakeAllRGBTriplets( false );
			MakeLabelAt( "Background", inputFont, Anchor.Centre, Anchor.Centre, -60, -100 );
			MakeLabelAt( "Button border", inputFont, Anchor.Centre, Anchor.Centre, -70, -60 );
			MakeLabelAt( "Button highlight", inputFont, Anchor.Centre, Anchor.Centre, -80, -20 );
			MakeLabelAt( "Button", inputFont, Anchor.Centre, Anchor.Centre, -40, 20 );
			MakeLabelAt( "Active button", inputFont, Anchor.Centre, Anchor.Centre, -70, 60 );
			MakeLabelAt( "Red", titleFont, Anchor.Centre, Anchor.Centre, 30, -130 );
			MakeLabelAt( "Green", titleFont, Anchor.Centre, Anchor.Centre, 95, -130 );
			MakeLabelAt( "Blue", titleFont, Anchor.Centre, Anchor.Centre, 160, -130 );
			
			defIndex = widgetIndex;
			MakeButtonAt( "Default colours", 160, 35, titleFont, Anchor.Centre, 0, 120 );
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre, 0, 170 );
		}
		
		public void MakeAllRGBTriplets( bool force ) {
			widgetIndex = 0;
			MakeRGBTriplet( LauncherSkin.BackgroundCol, force, -100 );
			MakeRGBTriplet( LauncherSkin.ButtonBorderCol, force, -60 );
			MakeRGBTriplet( LauncherSkin.ButtonHighlightCol, force, -20 );
			MakeRGBTriplet( LauncherSkin.ButtonForeCol, force, 20 );
			MakeRGBTriplet( LauncherSkin.ButtonForeActiveCol, force, 60 );
		}
		
		void MakeRGBTriplet( FastColour defCol, bool force, int y ) {
			MakeInput( GetCol( defCol.R, force ), 55, 
			          Anchor.Centre, Anchor.Centre, false, 30, y, 3, null );
			MakeInput( GetCol( defCol.G, force ), 55, 
			          Anchor.Centre, Anchor.Centre, false, 95, y, 3, null );
			MakeInput( GetCol( defCol.B, force ), 55, 
			          Anchor.Centre, Anchor.Centre, false, 160, y, 3, null );
		}
		
		string GetCol( byte col, bool force ) {
			if( force ) return col.ToString();
			LauncherWidget widget = widgets[widgetIndex];
			return widget == null ? col.ToString() : widget.Text;
		}
	}
}
