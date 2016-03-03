using System;
using System.IO;
using System.Net;
using ClassicalSharp.TexturePack;
using OpenTK;

namespace ClassicalSharp {
	
	internal static class Program {
		
		public const string AppName = "ClassicalSharp 0.98.7";
		
		public static string AppDirectory;
		
		[STAThread]
		static void Main( string[] args ) {
			AppDirectory = AppDomain.CurrentDomain.BaseDirectory;
			string logPath = Path.Combine( AppDirectory, "client.log" );
			ErrorHandler.InstallHandler( logPath );
			CleanupMainDirectory();
			
			Utils.LogDebug( "Starting " + AppName + ".." );
			string path = Path.Combine( Program.AppDirectory, TexturePackExtractor.Dir );
			if( !File.Exists( Path.Combine( path, "default.zip" ) ) ) {
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
				const string skinServer = "http://static.classicube.net/skins/";
				
				using( Game game = new Game( "Singleplayer", null, skinServer, nullContext, width, height ) )
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
				width = 1600; height = 900;
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
			using( Game game = new Game( args[0], args[1], skinServer, nullContext, width, height ) ) {
				game.IPAddress = ip;
				game.Port = port;
				game.Run();
			}
		}
		
		internal static void CleanupMainDirectory() {
			string mapPath = Path.Combine( Program.AppDirectory, "maps" );
			if( !Directory.Exists( mapPath ) )
				Directory.CreateDirectory( mapPath );
			string texPath = Path.Combine( Program.AppDirectory, TexturePackExtractor.Dir );
			if( !Directory.Exists( texPath ) )
				Directory.CreateDirectory( texPath );
			CopyFiles( "*.cw", mapPath );
			CopyFiles( "*.dat", mapPath );
			CopyFiles( "*.zip", texPath );
		}
		
		static void CopyFiles( string filter, string folder ) {
			string[] files = Directory.GetFiles( AppDirectory, filter );
			for( int i = 0; i < files.Length; i++ ) {
				string name = Path.GetFileName( files[i] );
				string dst = Path.Combine( folder, name );
				if( File.Exists( dst ) ) 
					continue;
				
				try {
					File.Copy( files[i], dst );
					File.Delete( files[i] );
				} catch( IOException ex ) {
				}
			}
		}
	}
}