using System;
using ClassicalSharp;
using OpenTK;
using OpenTK.Graphics;

namespace Launcher2 {

	internal sealed class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.95";
		
		[STAThread]
		private static void Main( string[] args ) {
			NativeWindow window = new NativeWindow( 480, 480, AppName, 0, 
			                                       GraphicsMode.Default, DisplayDevice.Default );
			window.Visible = true;
			
			MainScreen screen = new MainScreen();
			screen.Drawer = new GdiPlusDrawer2D( null );
			screen.Window = window;
			screen.RecreateBackground();
			screen.Init();
			
			while( true ) {
				window.ProcessEvents();
				if( !window.Exists ) break;
				screen.Display();
				System.Threading.Thread.Sleep( 10 );
			}
		}
	}
}
