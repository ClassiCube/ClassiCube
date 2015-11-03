using System;
using ClassicalSharp;

namespace Launcher2 {

	internal sealed class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.97";
		
		[STAThread]
		static void Main( string[] args ) {
			ErrorHandler.InstallHandler( "launcher.log" );		
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}
	}
}
