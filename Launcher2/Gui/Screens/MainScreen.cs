using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Input;

namespace Launcher2 {
	
	public sealed partial class MainScreen : LauncherInputScreen {
		
		public MainScreen( LauncherWindow game ) : base( game ) {
			titleFont = new Font( "Arial", 15, FontStyle.Bold );
			buttonFont = new Font( "Arial", 16, FontStyle.Bold );
			inputFont = new Font( "Arial", 15, FontStyle.Regular );
			enterIndex = 4;
			widgets = new LauncherWidget[11];
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			DrawClassicube();
			
			MakeButtonAt( "Direct connect", buttonWidth, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, 50,
			             (x, y) => game.SetScreen( new DirectConnectScreen( game ) ) );			
			
			MakeButtonAt( "Singleplayer", buttonWidth, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, 100,
			             (x, y) => Client.Start( "", ref game.ShouldExit ) );
			
			MakeButtonAt( "Colour scheme", buttonWidth - 40, buttonHeight, buttonFont,
			             Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10,
			             (x, y) => game.SetScreen( new ColoursScreen( game ) ) );
			
			MakeButtonAt( "Update check", buttonWidth - 40, buttonHeight, buttonFont,
			             Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -10,
			             (x, y) => game.SetScreen( new UpdatesScreen( game ) ) );
		}		
		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		
		public override void Dispose() {
			buttonFont.Dispose();
			StoreFields();
			base.Dispose();
		}
	}
}
