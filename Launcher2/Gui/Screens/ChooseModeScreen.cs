// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using Launcher.Updater;
using OpenTK.Input;

namespace Launcher {
	
	public abstract class ChooseModeScreen : LauncherScreen {
		
		protected Font titleFont, infoFont;
		public ChooseModeScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			infoFont = new Font( game.FontName, 14, FontStyle.Regular );
			buttonFont = titleFont;
			widgets = new LauncherWidget[14];
		}

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
			int middle = game.Width / 2;			
			MakeLabelAt("&fChoose game mode", titleFont, Anchor.Centre, Anchor.Centre, 0, -135 );
						
			MakeButtonAt( "Normal", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, middle - 250, -72,
			             (x, y) => ModeClick( false, false ) );
			MakeLabelAt( "&eEnables custom blocks, env settings,",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 - 12 );
			MakeLabelAt( "&elonger messages, and more",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, -72 + 12 );
			
			MakeButtonAt( "Classic", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, middle - 250, 0,
			             (x, y) => ModeClick( true, false ) );	
			MakeLabelAt( "&eOnly uses blocks and features from",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 0 - 12 );
			MakeLabelAt( "&ethe original minecraft classic",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 0 + 12 );
			
			MakeButtonAt( "Classic +hax", 145, 35, titleFont, Anchor.LeftOrTop, Anchor.Centre, middle - 250, 72,
			             (x, y) => ModeClick( true, true ) );
			MakeLabelAt( "&eSame as Classic mode, except that",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 72 - 12 );
			MakeLabelAt( "&ehacks (noclip/fly/speed) are enabled",
			            infoFont, Anchor.LeftOrTop, Anchor.Centre, middle - 85, 72 + 12 );
			MakeOtherWidgets();
		}
		
		protected virtual void MakeOtherWidgets() { }
		
		void ModeClick( bool classic, bool classicHacks ) {
			game.ClassicBackground = classic;
			Options.Load();
			Options.Set( "mode-classic", classic );
			if( classic )
				Options.Set( "nostalgia-hacks", classicHacks );
			
			Options.Set( "nostalgia-classicbg", classic );
			Options.Set( "nostalgia-customblocks", !classic );
			Options.Set( "nostalgia-usecpe", !classic );
			Options.Set( "nostalgia-servertextures", !classic );
			Options.Set( "nostalgia-classictablist", classic );
			Options.Set( "nostalgia-classicoptions", classic );
			Options.Set( "nostalgia-classicgui", true );
			Options.Save();
			
			game.SetScreen( new MainScreen( game ) );
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
	
	public sealed class ChooseModeNormalScreen : ChooseModeScreen {
		
		public ChooseModeNormalScreen( LauncherWindow game ) : base( game ) { }
			
		protected override void MakeOtherWidgets() {
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 175, (x, y) => game.SetScreen( new MainScreen( game ) ) );
		}
	}
	
	public sealed class ChooseModeFirstTimeScreen : ChooseModeScreen {
		
		public ChooseModeFirstTimeScreen( LauncherWindow game ) : base( game ) { }
			
		protected override void MakeOtherWidgets() {
			MakeLabelAt( "&eClick &fNormal &eif you are unsure which mode to choose.",
			            infoFont, Anchor.Centre, Anchor.Centre, 0, 160 );
		}
	}
}
