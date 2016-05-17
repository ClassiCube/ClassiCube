// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	
	public sealed class DirectConnectView : IView {
		
		internal int connectIndex, backIndex, ccSkinsIndex;
		Font booleanFont;

		public DirectConnectView( LauncherWindow game ) : base( game ) {
			widgets = new LauncherWidget[8];
		}

		public override void Init() {
			booleanFont = new Font( game.FontName, 22, FontStyle.Regular );
			titleFont = new Font( game.FontName, 15, FontStyle.Bold );
			inputFont = new Font( game.FontName, 14, FontStyle.Regular );
			inputHintFont = new Font( game.FontName, 12, FontStyle.Italic );
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
		
		void MakeWidgets() {
			widgetIndex = 0;
			
			MakeInput( Get( 0 ), 330, Anchor.Centre, Anchor.Centre,
			          false, 0, -100, 32, "&7Username.." );
			MakeInput( Get( 1 ), 330, Anchor.Centre, Anchor.Centre,
			          false, 0, -50, 64, "&7IP address:Port number.." );
			MakeInput( Get( 2 ), 330, Anchor.Centre, Anchor.Centre,
			          false, 0, 0, 32, "&7Mppass.." );
			
			connectIndex = widgetIndex;
			MakeButtonAt( "Connect", 110, 35, titleFont, Anchor.Centre, -110, 50 );
			backIndex = widgetIndex;
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre, 125, 50 );
			MakeLabelAt( "", titleFont, Anchor.Centre, Anchor.Centre, 0, 100 );
			MakeLabelAt( "Use classicube.net for skins", inputFont, Anchor.Centre, Anchor.Centre, 30, 130 );
			ccSkinsIndex = widgetIndex;
			MakeBooleanAt( Anchor.Centre, Anchor.Centre, booleanFont, true, 30, 30, -110, 130 );
		}
		
		string Get( int index ) {
			LauncherWidget widget = widgets[index];
			return widget == null ? "" : widget.Text;
		}
		
		public override void Dispose() {
			base.Dispose();
			booleanFont.Dispose();
		}
	}
}
