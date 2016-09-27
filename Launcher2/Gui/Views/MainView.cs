// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {	
	public sealed partial class MainView : IView {
		
		Font buttonFont, updateFont;
		internal int loginIndex, resIndex, dcIndex, spIndex, colIndex, statusIndex;
		internal int updatesIndex, modeIndex, sslIndex;
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		
		public MainView( LauncherWindow game ) : base( game ) {
			widgets = new Widget[13];
		}
		
		public override void Init() {
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			textFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
			
			buttonFont = new Font( game.FontName, 16, FontStyle.Bold );
			updateFont = new Font( game.FontName, 12, FontStyle.Italic );
			MakeWidgets();
		}
		
		string Get( int index ) {
			Widget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		public override void Dispose() {
			buttonFont.Dispose();
			updateFont.Dispose();
			base.Dispose();
		}

		
		internal string updateText = "&eChecking for updates..";
		protected override void MakeWidgets() {
			widgetIndex = 0;
			MakeInput( Get( 0 ), 280, false, 16, "&7Username.." )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, -120 );
			MakeInput( Get( 1 ), 280, true, 64, "&7Password.." )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, -70 );
			
			loginIndex = widgetIndex;
			Makers.Button( this, "Sign in", 100, buttonHeight, buttonFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, -90, -20 );
			statusIndex = widgetIndex;
			Makers.Label( this, Get( statusIndex ), textFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, 20 );
			
			
			resIndex = widgetIndex;
			Makers.Button( this, "Resume", 100, buttonHeight, buttonFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 90, -20 );
			dcIndex = widgetIndex;
			Makers.Button( this, "Direct connect", 200, buttonHeight, buttonFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, 60 );
			spIndex = widgetIndex;
			Makers.Button( this, "Singleplayer", 200, buttonHeight, buttonFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, 110 );
			
			
			colIndex = widgetIndex;
			Makers.Button( this, "Colours", 110, buttonHeight, buttonFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10 );
			if( game.ClassicBackground )
				widgets[widgetIndex - 1].Visible = false;

			modeIndex = widgetIndex;
			Makers.Button( this, "Choose mode", 200, buttonHeight, buttonFont )
				.SetLocation( Anchor.Centre, Anchor.BottomOrRight, 0, -10 );
			
			updatesIndex = widgetIndex;
			Makers.Button( this, "Updates", 110, buttonHeight, buttonFont )
				.SetLocation( Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -10 );			
			Makers.Label( this, updateText, updateFont )
				.SetLocation( Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -50 );
			
			
			sslIndex = widgetIndex;
			bool sslVisible = widgets[sslIndex] != null && widgets[sslIndex].Visible;
			Makers.Checkbox( this, textFont, true, 30 )
				.SetLocation( Anchor.Centre, Anchor.Centre, 160, -20 );
			Makers.Label( this, "Skip SSL check", textFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 250, -20 );
			widgets[sslIndex].Visible = sslVisible;
			widgets[sslIndex + 1].Visible = sslVisible;
		}
	}
}
