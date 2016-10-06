// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {	
	public sealed class ChooseModeView : IView {

		public bool FirstTime = true;
		internal int backIndex = -1, nIndex, clIndex, clHaxIndex;
		
		public ChooseModeView( LauncherWindow game ) : base( game ) {
			widgets = new Widget[11];
		}

		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			textFont = new Font( game.FontName, 14, FontStyle.Regular );
			MakeWidgets();
		}

		public override void DrawAll() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
				FastColour col = LauncherSkin.ButtonBorderCol;
				int midX = game.Width / 2, midY = game.Height / 2;
				game.Drawer.DrawRect( col, midX - 250, midY - 35, 490, 1 );
				game.Drawer.DrawRect( col, midX - 250, midY + 35, 490, 1 );
			}
		}
		
		protected override void MakeWidgets() {
			widgetIndex = 0;
			int middle = game.Width / 2;
			Makers.Label( this, "&fChoose game mode", titleFont )
				.SetLocation( Anchor.Centre, Anchor.Centre, 0, -135 );
			
			nIndex = widgetIndex;
			Makers.Button( this, "Enhanced", 145, 35, titleFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 250, -72 );
			Makers.Label( this, "&eEnables custom blocks, changing env", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 - 12 );
			Makers.Label( this, "&esettings, longer messages, and more", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 + 12 );

			clHaxIndex = widgetIndex;
			Makers.Button( this, "Classic +hax", 145, 35, titleFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 250, 0 );
			Makers.Label( this, "&eSame as Classic mode, except that", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, 0 - 12 );
			Makers.Label( this, "&ehacks (noclip/fly/speed) are enabled", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, 0 + 12 );
			
			clIndex = widgetIndex;
			Makers.Button( this, "Classic", 145, 35, titleFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 250, 72 );
			Makers.Label( this, "&eOnly uses blocks and features from", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, 72 - 12 );
			Makers.Label( this, "&ethe original minecraft classic", textFont )
				.SetLocation( Anchor.LeftOrTop, Anchor.Centre, middle - 85, 72 + 12 );
			
			if( FirstTime ) {
				backIndex = -1;
				Makers.Label( this, "&eClick &fEnhanced &eif you'e not sure which mode to choose.", textFont )
					.SetLocation( Anchor.Centre, Anchor.Centre, 0, 160 );
			} else {
				backIndex = widgetIndex;
				Makers.Button( this, "Back", 80, 35, titleFont )
					.SetLocation( Anchor.Centre, Anchor.Centre, 0, 170 );
			}
		}
	}
}