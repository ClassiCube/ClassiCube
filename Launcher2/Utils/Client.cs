using System;
using System.Diagnostics;
using System.IO;
using ClassicalSharp;

namespace Launcher2 {

	public static class Client {
		
		public static bool Start( ClientStartData data, bool classicubeSkins ) {
			string skinServer = classicubeSkins ? "http://www.classicube.net/static/skins/" : "http://s3.amazonaws.com/MinecraftSkins/";
			string args = data.Username + " " + data.Mppass + " " +
				data.Ip + " " + data.Port + " " + skinServer;
			
			UpdateResumeInfo( data, classicubeSkins );
			return Start( args );
		}
		
		public static bool Start( string args ) {
			Process process = null;
			
			if( !File.Exists( "ClassicalSharp.exe" ) )
				return false;
			
			if( Type.GetType( "Mono.Runtime" ) != null ) {
				process = Process.Start( "mono", "\"ClassicalSharp.exe\" " + args );
			} else {
				process = Process.Start( "ClassicalSharp.exe", args );
			}
			return true;
		}
		
		internal static void UpdateResumeInfo( ClientStartData data, bool classiCubeSkins ) {
			// If the client has changed some settings in the meantime, make sure we keep the changes
			try {
				Options.Load();
			} catch( IOException ) {
			}
			
			Options.Set( "launcher-username", data.Username );
			Options.Set( "launcher-ip", data.Ip );
			Options.Set( "launcher-port", data.Port );
			Options.Set( "launcher-mppass", Secure.Encode( data.Mppass, data.Username ) );
			Options.Set( "launcher-ccskins", classiCubeSkins );
			
			try {
				Options.Save();
			} catch( IOException ) {
			}
		}
	}
}
