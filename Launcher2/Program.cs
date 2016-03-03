using System;
using System.IO;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.98.7";
		
		public static string AppDirectory;
		
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main( string[] args ) {
			AppDirectory = AppDomain.CurrentDomain.BaseDirectory;			
			string logPath = Path.Combine( AppDirectory, "launcher.log" );
			AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler;
			ErrorHandler.InstallHandler( logPath );
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}

		static void UnhandledExceptionHandler( object sender, UnhandledExceptionEventArgs e ) {
			ShowingErrorDialog = true;
		}
	}
}
