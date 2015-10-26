using System;
using System.IO;
using System.Net;

namespace ClassicalSharp {
	
	internal static class Program {
		
		public static string AppName = "ClassicalSharp 0.95";
		
		[STAThread]
		static void Main( string[] args ) {
			ErrorHandler.InstallHandler( "client.log" );
			
			Utils.LogDebug( "Starting " + AppName + ".." );
			if( !File.Exists( "default.zip" ) ) {
				Fail( "default.zip not found. Cannot start." );
				return;
			}
				
			if( args.Length == 0 || args.Length == 1 ) {
				const string skinServer = "http://s3.amazonaws.com/MinecraftSkins/";
				string pack = args.Length >= 1 ? args[0] : "default.zip";
				using( Game game = new Game( "LocalPlayer", null, skinServer, pack ) ) {
					game.Run();
				}
			} else if( args.Length < 4 ) {
				Fail( "ClassicalSharp.exe is only the raw client. You must either use the launcher or"
				     + " provide command line arguments to start the client." );
			} else {
				RunMultiplayer( args );
			}
		}
		
		static void RunMultiplayer( string[] args ) {
			IPAddress ip = null;
			if( !IPAddress.TryParse( args[2], out ip ) ) {
				Fail( "Invalid IP \"" + args[2] + '"' );
			}

			int port = 0;
			if( !Int32.TryParse( args[3], out port ) ) {
				Fail( "Invalid port \"" + args[3] + '"' );
				return;
			} else if( port < ushort.MinValue || port > ushort.MaxValue ) {
				Fail( "Specified port " + port + " is out of valid range." );
			}

			string skinServer = args.Length >= 5 ? args[4] : "http://s3.amazonaws.com/MinecraftSkins/";
			string pack = args.Length >= 6 ? args[5] : "default.zip";
			using( Game game = new Game( args[0], args[1], skinServer, pack ) ) {
				game.IPAddress = ip;
				game.Port = port;
				game.Run();
			}
		}
		
		static void Fail( string text ) {
			Utils.LogWarning( text );
			Utils.Log( "Press any key to exit.." );
			Console.ReadKey( true );
		}
	}
}