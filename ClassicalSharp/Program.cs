// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using System.Net;
using OpenTK;

namespace ClassicalSharp {
	
	internal static class Program {
		
		public const string AppName = "ClassicalSharp 0.99.9.96";
		
		public const int APIVersion = 2;
		
		#if !LAUNCHER
		[STAThread]
		static void Main(string[] args) {
			Environment.CurrentDirectory = AppDomain.CurrentDomain.BaseDirectory;
			ErrorHandler.InstallHandler("client.log");
			Utils.LogDebug("Starting " + AppName + "..");
			
			Utils.EnsureDirectory("maps");
			Utils.EnsureDirectory("texpacks");
			Utils.EnsureDirectory("texturecache");
			
			string defPath = Path.Combine("texpacks", "default.zip");
			if (!Platform.FileExists(defPath)) {
				ErrorHandler.ShowDialog("Failed to start", "default.zip is missing, try running the launcher first.");
				return;
			}
			OpenTK.Configuration.SkipPerfCountersHack();
			

			IPAddress ip = null;
			int port = 0;
			string user = null, mppass = null;
			string skinServer = "http://static.classicube.net/skins/";
			
			if (args.Length == 0 || args.Length == 1) {
				user = args.Length > 0 ? args[0] : "Singleplayer";
			} else if (args.Length < 4) {
				ErrorHandler.ShowDialog("Failed to start", "ClassicalSharp.exe is only the raw client\n\n." +
				                        "Use the launcher instead, or provide command line arguments");
				return;
			} else {
				user   = args[0];
				mppass = args[1];
				if (args.Length > 4) skinServer = args[4];
				
				if (!IPAddress.TryParse(args[2], out ip)) {
					ErrorHandler.ShowDialog("Failed to start", "Invalid IP " + args[2]); return;
				}
				if (!Int32.TryParse(args[3], out port) || port < 0 || port > ushort.MaxValue) {
					ErrorHandler.ShowDialog("Failed to start", "Invalid port " + args[3]); return;
				}
			}
			
			Options.Load();
			DisplayDevice device = DisplayDevice.Default;
			int width  = Options.GetInt(OptionsKey.WindowWidth,  0, device.Bounds.Width,  0);
			int height = Options.GetInt(OptionsKey.WindowHeight, 0, device.Bounds.Height, 0);
			
			// No custom resolution has been set
			if (width == 0 || height == 0) {
				width = 854; height = 480;
				if (device.Bounds.Width < 854) width = 640;
			}
			
			using (Game game = new Game(user, mppass, skinServer, width, height)) {
				game.IPAddress = ip;
				game.Port = port;
				game.Run();
			}
		}
		#endif
	}
}