using System;
using System.IO;
using System.Net;
using OpenTK;

namespace ClassicalSharp {
	
	internal static class Program {
		
		public const string AppName = "ClassicalSharp 0.96";
		
		[STAThread]
		static void Main( string[] args ) {
			ErrorHandler.InstallHandler( "client.log" );
			
			Utils.LogDebug( "Starting " + AppName + ".." );
			if( !File.Exists( "default.zip" ) ) {
				Utils.LogDebug( "default.zip not found. Cannot start." );
				return;
			}
			bool nullContext = true;
			#if !USE_DX
			nullContext = false;
			#endif
			int width, height;
			SelectResolution( out width, out height );
				
			if( args.Length == 0 || args.Length == 1 ) {
				const string skinServer = "http://s3.amazonaws.com/MinecraftSkins/";
				string pack = args.Length >= 1 ? args[0] : "default.zip";
				
				using( Game game = new Game( "LocalPlayer", null, skinServer, 
				                            pack, nullContext, width, height ) )
					game.Run();
			} else if( args.Length < 4 ) {
				Utils.LogDebug( "ClassicalSharp.exe is only the raw client. You must either use the launcher or"
				     + " provide command line arguments to start the client." );
			} else {
				RunMultiplayer( args, nullContext, width, height );
			}
		}
		
		static void SelectResolution( out int width, out int height ) {
			DisplayDevice device = DisplayDevice.Default;
			width = 640; height = 480;
			
			if( device.Width >= 1024 && device.Height >= 768 ) {
				width = 800; height = 600;
			}
			if( device.Width >= 1920 && device.Height >= 1080 ) {
				width = 1024; height = 768;
			}
		}
		
		static void RunMultiplayer( string[] args, bool nullContext, int width, int height ) {
			IPAddress ip = null;
			if( !IPAddress.TryParse( args[2], out ip ) ) {
				Utils.LogDebug( "Invalid IP \"" + args[2] + '"' );
			}

			int port = 0;
			if( !Int32.TryParse( args[3], out port ) ) {
				Utils.LogDebug( "Invalid port \"" + args[3] + '"' );
				return;
			} else if( port < ushort.MinValue || port > ushort.MaxValue ) {
				Utils.LogDebug( "Specified port " + port + " is out of valid range." );
			}

			string skinServer = args.Length >= 5 ? args[4] : "http://s3.amazonaws.com/MinecraftSkins/";
			string pack = args.Length >= 6 ? args[5] : "default.zip";
			using( Game game = new Game( args[0], args[1], skinServer, pack, 
			                            nullContext, width, height ) ) {
				game.IPAddress = ip;
				game.Port = port;
				game.Run();
			}
		}
	}
}