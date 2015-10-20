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
			button.Active = false;
			button.Redraw( drawer, button.Text, textFont );
			Dirty = true;
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			button.Active = true;
			button.Redraw( drawer, button.Text, textFont );
			Dirty = true;
		}

		public override void Init() { Resize(); }
		
		public override void Tick() {
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		Font textFont;
		void Draw() {
			widgetIndex = 0;
			MakeButtonAt( "Direct connect", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -100,
			             (x, y) => game.SetScreen( new DirectConnectScreen( game ) ) );
			
			MakeButtonAt( "ClassiCube.net", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -50,
			             (x, y) => game.SetScreen( new ClassiCubeScreen( game ) ) );
			
			MakeButtonAt( "Singleplayer", Anchor.LeftOrTop, Anchor.BottomOrRight,
			             sideButtonWidth, buttonHeight, 10, -10,
			             (x, y) => Client.Start( "default.zip" ) );
			
			MakeButtonAt( "Resume", Anchor.BottomOrRight, Anchor.BottomOrRight,
			             sideButtonWidth, buttonHeight, -10, -10, null );
		}
		
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		void MakeButtonAt( string text, Anchor horAnchor,
		                  Anchor verAnchor, int width, int height, int x, int y, Action<int, int> onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.Active = false;
			widget.DrawAt( drawer, text, textFont, horAnchor, verAnchor, width, height, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			textFont.Dispose();
		}
	}
}
