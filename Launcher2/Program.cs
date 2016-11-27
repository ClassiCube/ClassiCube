// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.99.4";
		
		public static string AppDirectory;
		
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main(string[] args) {
			AppDirectory = AppDomain.CurrentDomain.BaseDirectory;
			string clientPath = Path.Combine(AppDirectory, "ClassicalSharp.exe");
			if (!File.Exists(clientPath)) {
				MessageBox.Show("ClassicalSharp.exe needs to be in the same folder as the launcher.", "Missing file");
				return;
			}
			
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
