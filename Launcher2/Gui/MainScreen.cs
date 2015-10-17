using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class MainScreen : LauncherScreen {
		
		public MainScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			textFont = new Font( "Arial", 16, FontStyle.Bold );
			widgets = new LauncherWidget[4];
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			button.Redraw( game.Drawer, button.Text, textFont );
			FilterArea( widget.X, widget.Y, widget.Width, widget.Height, 180 );
			Dirty = true;
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			button.Redraw( game.Drawer, button.Text, textFont );
			Dirty = true;
		}

		public override void Init() { Resize(); }
		
		public override void Resize() {
			using( IDrawer2D drawer = game.Drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				DrawButtons( drawer );
			}
			Dirty = true;
		}
		
		Font textFont;
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		void DrawButtons( IDrawer2D drawer ) {
			widgetIndex = 0;
			MakeButtonAt( drawer, "Direct connect", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -100,
			             () => game.SetScreen( new DirectConnectScreen( game ) ) );
			
			MakeButtonAt( drawer, "ClassiCube.net", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -50,
			             () => game.SetScreen( new ClassiCubeScreen( game ) ) );
			
			MakeButtonAt( drawer, "Singleplayer", Anchor.LeftOrTop, Anchor.BottomOrRight,
			             sideButtonWidth, buttonHeight, 10, -10,
			             () => Client.Start( "default.zip" ) );
			
			MakeButtonAt( drawer, "Resume", Anchor.BottomOrRight, Anchor.BottomOrRight,
			             sideButtonWidth, buttonHeight, -10, -10, null );
		}
		
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		void MakeButtonAt( IDrawer2D drawer, string text, Anchor horAnchor,
		                  Anchor verAnchor, int width, int height, int x, int y, Action onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.DrawAt( drawer, text, textFont, horAnchor, verAnchor, width, height, x, y );
			FilterArea( widget.X, widget.Y, widget.Width, widget.Height, 180 );
			widgets[widgetIndex++] = widget;
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			textFont.Dispose();
		}
	}
}
