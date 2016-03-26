// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using Launcher.Updater;
using OpenTK.Input;

namespace Launcher {
	
	public sealed class SelectModeScreen : LauncherScreen {
		
		Font titleFont, infoFont;
		public SelectModeScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			infoFont = new Font( game.FontName, 14, FontStyle.Regular );
			buttonFont = titleFont;
			widgets = new LauncherWidget[13];
		}

		UpdateCheckTask checkTask;
		public override void Init() {
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
			Resize();
		}
		
		public override void Tick() { }

		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab ) {
				HandleTab();
			} else if( e.Key == Key.Enter ) {
				LauncherWidget widget = selectedWidget;
				if( widget != null && widget.OnClick != null )
					widget.OnClick( 0, 0 );
			}
		}
		
		void KeyUp( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab )
				tabDown = false;
		}

		public override void Resize() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
				FastColour col = LauncherSkin.ButtonBorderCol;
				int middle = game.Height / 2;
				game.Drawer.DrawRect( col, game.Width / 2 - 250, middle - 35, 490, 1 );
				game.Drawer.DrawRect( col, game.Width / 2 - 250, middle + 35, 490, 1 );
			}
			Dirty = true;
		}
		
		void MakeWidgets() {
			widgetIndex = 0;
						
			MakeButtonAt( "Normal", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, 70, -72,
			             (x, y) => {} );
			MakeLabelAt( "&eEnables custom blocks, env settings,",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, 235, -72 - 12 );
			MakeLabelAt( "&elonger messages, and more",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, 235, -72 + 12 );
			
			MakeButtonAt( "Classic", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, 70, 0,
			             (x, y) => {} );		
			MakeLabelAt( "&eOnly uses blocks and features from",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, 235, 0 - 12 );
			MakeLabelAt( "&ethe original minecraft classic",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, 235, 0 + 12 );
			
			MakeButtonAt( "Classic +hax", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, 70, 72,
			             (x, y) => {} );
			MakeLabelAt( "&eSame as Classic mode, except that",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, 235, 72 - 12 );
			MakeLabelAt( "&ehacks (noclip/fly/speed) are enabled",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, 235, 72 + 12 );
			
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 175, (x, y) => game.SetScreen( new MainScreen( game ) ) );
		}

		public override void Dispose() {
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			titleFont.Dispose();
			infoFont.Dispose();
		}
	}
}
