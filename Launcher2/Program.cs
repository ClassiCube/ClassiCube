using System;
using System.IO;
using ClassicalSharp;

namespace Launcher2 {

	internal sealed class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.98.4";
		
		public static string AppDirectory;
		
		[STAThread]
		static void Main( string[] args ) {
			AppDirectory = AppDomain.CurrentDomain.BaseDirectory;			
			string logPath = Path.Combine( AppDirectory, "launcher.log" );
			ErrorHandler.InstallHandler( logPath );
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}
	}
}
