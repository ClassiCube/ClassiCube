using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using ClassicalSharp.TexturePack;

namespace Launcher2.Updater {
	
	public static class Patcher {
		
		public static void Update( string dir ) {
			using( WebClient client = new WebClient() ) {
				byte[] zipData = client.DownloadData( UpdateCheckTask.UpdatesUri + dir );
				MakeUpdatesFolder( zipData );
			}
			
			if( !OpenTK.Configuration.RunningOnWindows ) 
				return; // TODO: bash script for OSX and linux
			File.WriteAllText( "update.bat", Scripts.BatchFile );
			
			ProcessStartInfo info = new ProcessStartInfo( "cmd.exe", "/c update.bat" );
			info.CreateNoWindow = false;
			info.UseShellExecute = false;
			Process p = Process.Start( info );
			Process.GetCurrentProcess().Kill();
		}
		
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
