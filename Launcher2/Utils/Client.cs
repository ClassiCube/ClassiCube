// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using ClassicalSharp;
using OpenTK;

namespace Launcher {

	public static class Client {
		
		static DateTime lastJoin;
		public static bool Start( ClientStartData data, bool classicubeSkins, ref bool shouldExit ) {
			if( (DateTime.UtcNow - lastJoin).TotalSeconds < 1 )
				return false;
			lastJoin = DateTime.UtcNow;
			
			string skinServer = classicubeSkins ? "http://static.classicube.net/skins/" :
				"http://s3.amazonaws.com/MinecraftSkins/";
			string args = data.Username + " " + data.Mppass + " " +
				data.Ip + " " + data.Port + " " + skinServer;
			return StartImpl( data, classicubeSkins, args, ref shouldExit );
		}
		
		public static bool Start( string args, ref bool shouldExit ) {
			return StartImpl( null, true, args, ref shouldExit );
		}
		
		static bool StartImpl( ClientStartData data, bool classicubeSkins,
		                      string args, ref bool shouldExit ) {
			Process process = null;
			string path = Path.Combine( Program.AppDirectory, "ClassicalSharp.exe" );
			if( !File.Exists( path ) )
				return false;
			
			CheckSettings( data, classicubeSkins, out shouldExit );
			if( Configuration.RunningOnMono ) {
				// We also need to handle the case of running Mono through wine
				if( Configuration.RunningOnWindows ) {
					try {
						process = Process.Start( "mono", "\"" + path + "\" " + args );
					} catch( Win32Exception ex ) {
						if( !((uint)ex.ErrorCode == 0x80070002 || (uint)ex.ErrorCode == 0x80004005) )
							throw; // File not found HRESULT, HRESULT thrown when running on wine
						process = Process.Start( path, args );
					}
				} else {
					process = Process.Start( "mono", "\"" + path + "\" " + args );
				}			
			} else {
				process = Process.Start( path, args );
			}
			return true;
		}
		
		internal static void CheckSettings( ClientStartData data, bool classiCubeSkins, out bool shouldExit ) {
			shouldExit = false;
			// Make sure if the client has changed some settings in the meantime, we keep the changes
			if( !Options.Load() )
				return;
			shouldExit = Options.GetBool( OptionsKey.AutoCloseLauncher, false );
			if( data == null ) return;
			
			Options.Set( "launcher-username", data.RealUsername );
			Options.Set( "launcher-ip", data.Ip );
			Options.Set( "launcher-port", data.Port );
			Options.Set( "launcher-mppass", Secure.Encode( data.Mppass, data.RealUsername ) );
			Options.Set( "launcher-ccskins", classiCubeSkins );		
			Options.Save();
		}
	}
}
