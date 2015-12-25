using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class MainScreen : LauncherScreen {
		
		public MainScreen( LauncherWindow game ) : base( game ) {
			textFont = new Font( "Arial", 16, FontStyle.Bold );
			widgets = new LauncherWidget[4];
			buttonFont = textFont;
		}
		
		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab ) {
				HandleTab();
			} else if( e.Key == Key.Enter ) {
				LauncherWidget widget = selectedWidget != null ?
					selectedWidget : widgets[0];
				if( widget.OnClick != null )
					widget.OnClick( 0, 0 );
			}
		}
		
		void KeyUp( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab )
				tabDown = false;
		}

		public override void Init() {
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
			
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			Resize();
		}
		
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
			MakeButtonAt( "ClassiCube.net", Anchor.Centre, Anchor.Centre,
			             buttonWidth, 0, -100,
			             (x, y) => game.SetScreen( new ClassiCubeScreen( game ) ) );
			
			MakeButtonAt( "Direct connect", Anchor.Centre, Anchor.Centre,
			             buttonWidth, 0, -50,
			             (x, y) => game.SetScreen( new DirectConnectScreen( game ) ) );			
			
			MakeButtonAt( "Singleplayer", Anchor.Centre, Anchor.Centre,
			             buttonWidth, 0, 0,
			             (x, y) => Client.Start( "", ref game.ShouldExit ) );
			
			MakeButtonAt( "Check for updates", Anchor.Centre, Anchor.Centre,
			             buttonWidth, 0, 100,
			             (x, y) => game.SetScreen( new UpdatesScreen( game ) ) );
		}
		
		const int buttonWidth = 220, sideButtonWidth = 150;
		void MakeButtonAt( string text, Anchor horAnchor, Anchor verAnchor, 
		                  int width, int x, int y, Action<int, int> onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.Active = false;
			widget.DrawAt( drawer, text, textFont, horAnchor, verAnchor, width, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
			textFont.Dispose();
		}
	}
}
