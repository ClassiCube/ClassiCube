// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.99.5";
		
		public static string AppDirectory;
		
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main(string[] args) {
			AppDirectory = AppDomain.CurrentDomain.BaseDirectory;
			if (!CheckFilesExist()) return;
			
			// NOTE: we purposely put this in another method, as we need to ensure
			// that we do not reference any OpenTK code directly in the main function
			// (which LauncherWindow does), which otherwise causes native crash.
			RunLauncher();
		}
		
		static bool CheckFilesExist() {
			string path = Path.Combine(AppDirectory, "ClassicalSharp.exe");
			if (!File.Exists(path)) {
				MessageBox.Show("ClassicalSharp.exe needs to be in the same folder as the launcher.", "Missing file");
				return false;
			}
			
			path = Path.Combine(AppDirectory, "OpenTK.dll");
			if (!File.Exists(path)) {
				MessageBox.Show("OpenTK.dll needs to be in the same folder as the launcher.", "Missing file");
				return false;
			}
			return true;		
		}
		
		static void RunLauncher() {
			string logPath = Path.Combine(AppDirectory, "launcher.log");
			AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler;
			ErrorHandler2.InstallHandler(logPath);
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}

		static void UnhandledExceptionHandler(object sender, UnhandledExceptionEventArgs e) {
			ShowingErrorDialog = true;
		}
	}
}
