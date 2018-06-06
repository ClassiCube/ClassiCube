// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using System.Net;
using OpenTK;

namespace ClassicalSharp {
	
	internal static class Program {
		
		public const string AppName = "ClassicalSharp 0.99.9.96";
		
		#if !LAUNCHER
		[STAThread]
		static void Main(string[] args) {
			Environment.CurrentDirectory = AppDomain.CurrentDomain.BaseDirectory;
			CleanupMainDirectory();
			
			string defPath = Path.Combine("texpacks", "default.zip");
			if (!Platform.FileExists(defPath)) {
				ErrorHandler.ShowDialog("Missing file", "default.zip not found, try running the launcher first.");
				return;
			}
			
			if (!Platform.FileExists("OpenTK.dll")) {
				ErrorHandler.ShowDialog("Missing file", "OpenTK.dll needs to be in the same folder as the game");
				return;
			}
			
			// NOTE: we purposely put this in another method, as we need to ensure
			// that we do not reference any OpenTK code directly in the main function
			// (such as DisplayDevice), which otherwise causes native crash.
			RunGame(args);
		}
		
		static void RunGame(string[] args) {
			ErrorHandler.InstallHandler("client.log");
			OpenTK.Configuration.SkipPerfCountersHack();
			Utils.LogDebug("Starting " + AppName + "..");
			
			bool nullContext = true;
			#if !USE_DX
			nullContext = false;
			#endif
			
			Options.Load();
			DisplayDevice device = DisplayDevice.Primary;
			int width  = Options.GetInt(OptionsKey.WindowWidth,  0, device.Width,  0);
			int height = Options.GetInt(OptionsKey.WindowHeight, 0, device.Height, 0);
			
			// No custom resolution has been set
			if (width == 0 || height == 0) {
				width = 854; height = 480;
				if (device.Width < 854) width = 640;
			}
			
			if (args.Length == 0 || args.Length == 1) {
				const string skinServer = "http://static.classicube.net/skins/";
				string user = args.Length > 0 ? args[0] : "Singleplayer";
				using (Game game = new Game(user, null, skinServer, nullContext, width, height))
					game.Run();
			} else if (args.Length < 4) {
				Utils.LogDebug("ClassicalSharp.exe is only the raw client. You must either use the launcher or"
				               + " provide command line arguments to start the client.");
			} else {
				RunMultiplayer(args, nullContext, width, height);
			}
		}
		
		static void RunMultiplayer(string[] args, bool nullContext, int width, int height) {
			IPAddress ip = null;
			if (!IPAddress.TryParse(args[2], out ip)) {
				Utils.LogDebug("Invalid IP \"" + args[2] + '"'); return;
			}

			int port = 0;
			if (!Int32.TryParse(args[3], out port)) {
				Utils.LogDebug("Invalid port \"" + args[3] + '"');
				return;
			} else if (port < ushort.MinValue || port > ushort.MaxValue) {
				Utils.LogDebug("Specified port " + port + " is out of valid range.");
				return;
			}

			string skinServer = args.Length >= 5 ? args[4] : "http://static.classicube.net/skins/";
			using (Game game = new Game(args[0], args[1], skinServer, nullContext, width, height)) {
				game.IPAddress = ip;
				game.Port = port;
				game.Run();
			}
		}
		#endif
		
		public static void CleanupMainDirectory() {
			if (!Platform.DirectoryExists("maps")) {
				Platform.DirectoryCreate("maps");
			}

			if (!Platform.DirectoryExists("texpacks")) {
				Platform.DirectoryCreate("texpacks");
			}
			
			if (!Platform.DirectoryExists("texturecache")) {
				Platform.DirectoryCreate("texturecache");
			}
		}
	}
}