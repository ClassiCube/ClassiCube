using System;
using ClassicalSharp;
using OpenTK;
using OpenTK.Graphics;
using System.IO;
using System.Diagnostics;

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
			screen.Init();
			screen.RecreateBackground();			
			
			while( true ) {
				window.ProcessEvents();
				if( !window.Exists ) break;
				
				screen.Display();
				System.Threading.Thread.Sleep( 10 );
			}
		}
		
		static string missingExeMessage = "Failed to start ClassicalSharp. (classicalsharp.exe was not found)"
			+ Environment.NewLine + Environment.NewLine +
			"This application is only the launcher, it is not the actual client. " +
			"Please place the launcher in the same directory as the client (classicalsharp.exe).";
		
		public static bool StartClient( string args ) {
			Process process = null;
			
			if( !File.Exists( "ClassicalSharp.exe" ) ) {
				// TODO: show message popup
				return false;
			}
			
			if( Type.GetType( "Mono.Runtime" ) != null ) {
				process = Process.Start( "mono", "\"ClassicalSharp.exe\" " + args );
			} else {
				process = Process.Start( "ClassicalSharp.exe", args );
			}
			return true;
		}
	}
}
