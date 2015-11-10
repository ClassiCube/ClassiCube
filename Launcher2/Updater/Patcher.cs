using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using ClassicalSharp.TexturePack;

namespace Launcher2.Updater {
	
	public static class Patcher {
		
		public static void Update( string dir ) {
			using( WebClient client = new WebClient() ) {
				byte[] zipData = client.DownloadData( UpdateCheckTask.UpdatesUri + dir );
				MakeUpdatesFolder( zipData );
			}
			if( !OpenTK.Configuration.RunningOnWindows ) return;
			LaunchUpdateScript();
			Process.GetCurrentProcess().Kill();
		}

		static void LaunchUpdateScript() {
			ProcessStartInfo info;
			if( OpenTK.Configuration.RunningOnWindows ) {
				File.WriteAllText( "update.bat", Scripts.BatchFile );
				info = new ProcessStartInfo( "cmd.exe", "/c update.bat" );
			} else {
				File.WriteAllText( "update.sh", Scripts.BashFile.Replace( "\r\n", "\n" ) );
				string path = Path.Combine( Environment.CurrentDirectory, "update.sh" );
				const int flags = 0x7;// read | write | executable
				int code = chmod( path, (flags << 6) | (flags << 3) | 4 ); 
				if( code != 0 )
					throw new InvalidOperationException( "chmod returned : " + code );
				info = new ProcessStartInfo( "/bin/bash", "-c " + path );
			}
			// TODO: delete directory
			// TODO: why no start new window?
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
			string path = Path.Combine( "CS_Update", Path.GetFileName( filename ) );
			File.WriteAllBytes( path, data );
		}
	}
}
