// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.99.9.94";
		
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main(string[] args) {
			Platform.AppDirectory = AppDomain.CurrentDomain.BaseDirectory;

			if (!Platform.FileExists("ClassicalSharp.exe")) { 
				ErrorHandler.ShowDialog("Missing file", "ClassicalSharp.exe needs to be in the same folder as the launcher."); 
				return;
			}

			if (!Platform.FileExists("OpenTK.dll")) { 
				ErrorHandler.ShowDialog("Missing file", "OpenTK.dll needs to be in the same folder as the launcher."); 
				return;
			}
			
			// NOTE: we purposely put this in another method, as we need to ensure
			// that we do not reference any OpenTK code directly in the main function
			// (which LauncherWindow does), which otherwise causes native crash.
			RunLauncher();
		}
		
		static void RunLauncher() {
			AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler;
			ErrorHandler.InstallHandler("launcher.log");
			OpenTK.Configuration.SkipPerfCountersHack();
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}

		static void UnhandledExceptionHandler(object sender, UnhandledExceptionEventArgs e) {
			ShowingErrorDialog = true;
		}
	}
}
