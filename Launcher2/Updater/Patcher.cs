// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;
using ClassicalSharp;
using ClassicalSharp.TexturePack;

namespace Launcher.Updater {
	
	public static class Patcher {
		
		public static DateTime PatchTime;
		
		public static void Update( string dir ) {
			using( WebClient client = new WebClient() ) {
				byte[] zipData = client.DownloadData( UpdateCheckTask.UpdatesUri + dir );
				MakeUpdatesFolder( zipData );
			}
		}

		public static void LaunchUpdateScript() {
			ProcessStartInfo info;
			if( OpenTK.Configuration.RunningOnWindows ) {
				string path = Path.Combine( Program.AppDirectory, "update.bat" );
				File.WriteAllText( path, Scripts.BatchFile );
				info = new ProcessStartInfo( "cmd.exe", "/c update.bat" );
			} else {
				string path = Path.Combine( Program.AppDirectory, "update.sh" );
				File.WriteAllText( path, Scripts.BashFile.Replace( "\r\n", "\n" ) );
				const int flags = 0x7;// read | write | executable
				int code = chmod( path, (flags << 6) | (flags << 3) | 4 );
				if( code != 0 )
					throw new InvalidOperationException( "chmod returned : " + code );
				info = new ProcessStartInfo( "xterm", '"' + path + '"');
			}
			info.CreateNoWindow = false;
			info.UseShellExecute = false;
			Process.Start( info );
		}

		[DllImport( "libc", SetLastError = true )]
		internal static extern int chmod( string path, int mode );
		
		static void MakeUpdatesFolder( byte[] zipData ) {
			using( MemoryStream stream = new MemoryStream( zipData ) ) {
				ZipReader reader = new ZipReader();
				Directory.CreateDirectory( "CS_Update" );
				
				reader.ShouldProcessZipEntry = (f) => true;
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract( stream );
			}
		}
		
		static void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			string path = Path.Combine( Program.AppDirectory, "CS_Update" );
			path = Path.Combine( path, Path.GetFileName( filename ) );
			File.WriteAllBytes( path, data );
			
			try {
				File.SetLastWriteTimeUtc( path, PatchTime );
			} catch( IOException ex ) {
				ErrorHandler.LogError( "I/O exception when trying to set modified time for: " + filename, ex );
			} catch( UnauthorizedAccessException ex ) {
				ErrorHandler.LogError( "Permissions exception when trying to set modified time for: " + filename, ex );
			}
		}
	}
}
