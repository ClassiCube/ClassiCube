// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	
	public sealed partial class MainView : IView {
		
		Font buttonFont, updateFont;
		internal int loginIndex, resIndex, dcIndex, spIndex, colIndex;
		internal int updatesIndex, modeIndex, sslIndex;
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		
		public MainView( LauncherWindow game ) : base( game ) {
			widgets = new LauncherWidget[16];
		}
		
		public override void Init() {
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			inputFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
			
			buttonFont = new Font( game.FontName, 16, FontStyle.Bold );
			updateFont = new Font( game.FontName, 12, FontStyle.Italic );
			MakeWidgets();
		}
		
		public override void DrawAll() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
		}
		
		internal string updateText = "&eChecking for updates..";
		void MakeWidgets() {
			widgetIndex = 0;
			MakeInput( Get( 0 ), 280, false, 16, "&7Username.." )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 0, -120 );
			MakeInput( Get( 1 ), 280, true, 64, "&7Password.." )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 0, -70 );
			
			loginIndex = widgetIndex;
			Makers.Button( this, "Sign in", 100, buttonHeight, buttonFont )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, -90, -20 );
			Makers.Label( this, Get( 3 ), inputFont )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 0, 20 );
			
			resIndex = widgetIndex;
			Makers.Button( this, "Resume", 100, buttonHeight, buttonFont )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 90, -20 );
			dcIndex = widgetIndex;
			Makers.Button( this, "Direct connect", 200, buttonHeight, buttonFont )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 0, 60 );
			spIndex = widgetIndex;
			Makers.Button( this, "Singleplayer", 200, buttonHeight, buttonFont )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 0, 110 );
			
			colIndex = widgetIndex;
			if( !game.ClassicBackground ) {
				Makers.Button( this, "Colours", 110, buttonHeight, buttonFont )
					.UpdateLocation( Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10 );
			} else {
				widgets[widgetIndex++] = null;
			}
			
			updatesIndex = widgetIndex;
			Makers.Button( this, "Updates", 110, buttonHeight, buttonFont )
				.UpdateLocation( Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -10 );
			modeIndex = widgetIndex;
			Makers.Button( this, "Choose mode", 200, buttonHeight, buttonFont )
				.UpdateLocation( Anchor.Centre, Anchor.BottomOrRight, 0, -10 );
			
			Makers.Label( this, updateText, updateFont )
				.UpdateLocation( Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -50 );
			
			sslIndex = widgetIndex;
			if( widgets[widgetIndex] != null )
				MakeSslWidgets();
		}
		
		internal void MakeSslWidgets() {
			Makers.Boolean( this, inputFont, true, 30 )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 160, -20 );
			Makers.Label( this, "Skip SSL check", inputFont )
				.UpdateLocation( Anchor.Centre, Anchor.Centre, 250, -20 );
		}
		
		string Get( int index ) {
			LauncherWidget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		public override void Dispose() {
			buttonFont.Dispose();
			updateFont.Dispose();
			base.Dispose();
		}
	}
}
