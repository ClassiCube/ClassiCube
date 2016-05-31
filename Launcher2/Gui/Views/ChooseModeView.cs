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
			widgets = new LauncherWidget[14];
		}

		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			inputFont = new Font( game.FontName, 14, FontStyle.Regular );
			UpdateWidgets();
		}

		public override void DrawAll() {
			UpdateWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
				FastColour col = LauncherSkin.ButtonBorderCol;
				int middle = game.Height / 2;
				game.Drawer.DrawRect( col, game.Width / 2 - 250, middle - 35, 490, 1 );
				game.Drawer.DrawRect( col, game.Width / 2 - 250, middle + 35, 490, 1 );
			}
		}
		
		void UpdateWidgets() {
			widgetIndex = 0;
			int middle = game.Width / 2;
			MakeLabelAt( "&fChoose game mode", titleFont, Anchor.Centre, Anchor.Centre, 0, -135 );
			
			nIndex = widgetIndex;
			MakeButtonAt( "Enhanced", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, middle - 250, -72 );
			MakeLabelAt( "&eEnables custom blocks, changing env",
			             inputFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 - 12 );
			MakeLabelAt( "&esettings, longer messages, and more",
			            inputFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 + 12 );

			clHaxIndex = widgetIndex;
			MakeButtonAt( "Classic +hax", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, middle - 250, 0 );
			MakeLabelAt( "&eSame as Classic mode, except that",
			            inputFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 0 - 12 );
			MakeLabelAt( "&ehacks (noclip/fly/speed) are enabled",
			            inputFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 0 + 12 );	
			
			clIndex = widgetIndex;
			MakeButtonAt( "Classic", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, middle - 250, 72 );
			MakeLabelAt( "&eOnly uses blocks and features from",
			            inputFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 72 - 12 );
			MakeLabelAt( "&ethe original minecraft classic",
			            inputFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 72 + 12 );
			
			if( FirstTime ) {
				backIndex = -1;
				MakeLabelAt( "&eClick &fEnhanced &eif you are unsure which mode to choose.",
				            inputFont, Anchor.Centre, Anchor.Centre, 0, 160 );
			} else {
				backIndex = widgetIndex;
				MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre, 0, 175 );
			}
		}
	}
}