// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	public sealed class SettingsView : IView {
		internal int backIndex, updatesIndex, modeIndex, coloursIndex;
		
		public SettingsView( LauncherWindow game ) : base( game ) {
			widgets = new Widget[3];
		}

		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			textFont = new Font( game.FontName, 14, FontStyle.Regular );
			MakeWidgets();
		}
		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			int middle = game.Width / 2;
			
			modeIndex = widgetIndex;
			Makers.Button( this, "Choose mode", 145, 35, titleFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 250, -72 );
			Makers.Label( this, "&eChange the client's mode/features", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 - 12 );

			
			backIndex = widgetIndex;
			Makers.Button( this, "Back", 80, 35, titleFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, 175 );
		}
	}
}