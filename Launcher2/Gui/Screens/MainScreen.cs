using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed class MainScreen : LauncherScreen {
		
		public MainScreen( LauncherWindow game ) : base( game ) {
			textFont = new Font( "Arial", 16, FontStyle.Bold );
			widgets = new LauncherWidget[4];
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = (LauncherButtonWidget)widget;
			button.Active = false;
			button.Redraw( drawer, button.Text, textFont );
			Dirty = true;
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = (LauncherButtonWidget)widget;
			button.Active = true;
			button.Redraw( drawer, button.Text, textFont );
			Dirty = true;
		}
		
		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab )
				HandleTab();
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
			             buttonWidth, buttonHeight, 0, -100,
			             (x, y) => game.SetScreen( new ClassiCubeScreen( game ) ) );
			
			MakeButtonAt( "Direct connect", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, -50,
			             (x, y) => game.SetScreen( new DirectConnectScreen( game ) ) );			
			
			MakeButtonAt( "Singleplayer", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, 0,
			             (x, y) => Client.Start( "" ) );
			
			MakeButtonAt( "Check for updates", Anchor.Centre, Anchor.Centre,
			             buttonWidth, buttonHeight, 0, 100,
			             (x, y) => game.SetScreen( new UpdatesScreen( game ) ) );
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
			
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
			textFont.Dispose();
		}
	}
}
