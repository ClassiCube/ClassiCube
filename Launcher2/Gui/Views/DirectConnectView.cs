// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {	
	public sealed class DirectConnectView : IView {
		
		internal int connectIndex, backIndex, ccSkinsIndex, statusIndex;
		Font booleanFont;

		public DirectConnectView( LauncherWindow game ) : base( game ) {
			widgets = new Widget[8];
		}

		public override void Init() {
			booleanFont = new Font( game.FontName, 22, FontStyle.Regular );
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			textFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
			MakeWidgets();
		}
		
		string Get( int index ) {
			Widget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		public override void Dispose() {
			base.Dispose();
			booleanFont.Dispose();
		}

		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			
			MakeInput( Get( 0 ), 330, false, 32, "&7Username.." )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, -100 );
			MakeInput( Get( 1 ), 330, false, 64, "&7IP address:Port number.." )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, -50 );
			MakeInput( Get( 2 ), 330, false, 32, "&7Mppass.." )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, 0 );
			
			connectIndex = widgetIndex;
			Makers.Button( this, "Connect", 110, 35, titleFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, -110, 50 );
			
			backIndex = widgetIndex;
			Makers.Button( this, "Back", 80, 35, titleFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 125, 50 );
			statusIndex = widgetIndex;
			Makers.Label( this, "", textFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, 100 );
			Makers.Label( this, "Use classicube.net for skins", textFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 30, 130 );
			
			ccSkinsIndex = widgetIndex;
			Makers.Checkbox( this, booleanFont, true, 30 )
				.SetLocation( Anchor.Centre, Anchor.Centre, -110, 130 );
		}
	}
}
