// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher {

	internal static class Program {
		
		public const string AppName = "ClassicalSharp Launcher 0.99.9.98";
		public static bool ShowingErrorDialog = false;
		
		[STAThread]
		static void Main(string[] args) {
			Environment.CurrentDirectory = AppDomain.CurrentDomain.BaseDirectory;
			ErrorHandler.InstallHandler("launcher.log");

			if (!Platform.FileExists("ClassicalSharp.exe")) {
				ErrorHandler.ShowDialog("Missing file", "ClassicalSharp.exe needs to be in the same folder as the launcher.");
				return;
			}
			OpenTK.Configuration.SkipPerfCountersHack();
			
			Utils.EnsureDirectory("maps");
			Utils.EnsureDirectory("texpacks");
			Utils.EnsureDirectory("texturecache");
			
			AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler;			
			LauncherWindow window = new LauncherWindow();
			window.Run();
		}

		static void UnhandledExceptionHandler(object sender, UnhandledExceptionEventArgs e) {
			ShowingErrorDialog = true;
		}
	}
}
