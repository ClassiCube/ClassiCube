﻿using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Windows.Forms;
using ClassicalSharp.TexturePack;

namespace ClassicalSharp {
	
	static class Program {
		
		[STAThread]
		public static void Main( string[] args ) {
			if( !Debugger.IsAttached ) {
				AppDomain.CurrentDomain.UnhandledException += UnhandledException;
			}
			
			Utils.Log( "Starting " + Utils.AppName + ".." );
			if( !File.Exists( "default.zip" ) ) {
				Fail( "default.zip not found. Cannot start." );
				return;
			}
				
			if( args.Length == 0 || args.Length == 1 ) {
				Utils.Log( "Starting singleplayer mode." );
				const string skinServer = "http://s3.amazonaws.com/MinecraftSkins/";
				using( Game game = new Game( "LocalPlayer", null, skinServer, "default.zip" ) ) {
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
			using( Game game = new Game( args[0], args[1], skinServer, "default.zip" ) ) {
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

		static void UnhandledException( object sender, UnhandledExceptionEventArgs e ) {
			// So we don't get the normal unhelpful crash dialog on Windows.
			Exception ex = (Exception)e.ExceptionObject;
			string error = ex.GetType().FullName + ": " + ex.Message + Environment.NewLine + ex.StackTrace;
			bool wroteToCrashLog = true;
			try {
				using( StreamWriter writer = new StreamWriter( "crash.log", true ) ) {
					writer.WriteLine( "Crash time: " + DateTime.Now.ToString() );
					writer.WriteLine( error );
					writer.WriteLine();
				}
			} catch( Exception ) {
				wroteToCrashLog = false;
			}
			
			string message = wroteToCrashLog ?
				"The cause of the crash has been logged to \"crash.log\". Please consider reporting the crash " +
				"(and the circumstances that caused it) to github.com/UnknownShadow200/ClassicalSharp/issues" :
				
				"Failed to write the cause of the crash to \"crash.log\". Please consider reporting the crash " +
				"(and the circumstances that caused it) to github.com/UnknownShadow200/ClassicalSharp/issues" +
				Environment.NewLine + Environment.NewLine + error;
			
			MessageBox.Show( "Oh dear. ClassicalSharp has crashed." + Environment.NewLine + Environment.NewLine + message, "ClassicalSharp has crashed" );
			Environment.Exit( 1 );
		}
	}
}