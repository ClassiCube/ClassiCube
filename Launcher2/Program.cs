// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.99.9.96";	
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main(string[] args) {
			Environment.CurrentDirectory = AppDomain.CurrentDomain.BaseDirectory;

			if (!Platform.FileExists("ClassicalSharp.exe")) { 
				ErrorHandler.ShowDialog("Missing file", "ClassicalSharp.exe needs to be in the same folder as the launcher."); 
				return;
			}
					
			ErrorHandler.InstallHandler("launcher.log");
			OpenTK.Configuration.SkipPerfCountersHack();
			AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler;
			
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}

		static void UnhandledExceptionHandler(object sender, UnhandledExceptionEventArgs e) {
			ShowingErrorDialog = true;
		}
	}
}
