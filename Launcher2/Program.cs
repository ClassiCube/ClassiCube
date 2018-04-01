// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.99.9.93";
		
		public static string AppDirectory;
		
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main(string[] args) {
			AppDirectory = AppDomain.CurrentDomain.BaseDirectory;
			
			string path = Path.Combine(AppDirectory, "ClassicalSharp.exe");
			if (!File.Exists(path)) { 
				Message("ClassicalSharp.exe needs to be in the same folder as the launcher."); return; 
			}
			
			path = Path.Combine(AppDirectory, "OpenTK.dll");
			if (!File.Exists(path)) { 
				Message("OpenTK.dll needs to be in the same folder as the launcher."); return;
			}
			
			// NOTE: we purposely put this in another method, as we need to ensure
			// that we do not reference any OpenTK code directly in the main function
			// (which LauncherWindow does), which otherwise causes native crash.
			RunLauncher();
		}
		
		// put in separate function, because we don't want to load winforms assembly if possible
		static void Message(string message) { MessageBox.Show(message, "Missing file"); }
		
		static void RunLauncher() {
			string logPath = Path.Combine(AppDirectory, "launcher.log");
			AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler;
			ErrorHandler.InstallHandler(logPath);
			OpenTK.Configuration.SkipPerfCountersHack();
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}

		static void UnhandledExceptionHandler(object sender, UnhandledExceptionEventArgs e) {
			ShowingErrorDialog = true;
		}
	}
}
