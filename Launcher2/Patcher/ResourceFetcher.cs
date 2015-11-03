using System;
using System.IO;
using ClassicalSharp.Network;

namespace Launcher2 {
	
	public sealed class ResourceFetcher {
		
		public bool Done = false;
		AsyncDownloader downloader;
		public ResourceFetcher( AsyncDownloader downloader ) {
			this.downloader = downloader;
		}
		
		const string jarClassicUri = "http://s3.amazonaws.com/Minecraft.Download/versions/c0.30_01c/c0.30_01c.jar";
		const string jar162Uri = "http://s3.amazonaws.com/Minecraft.Download/versions/1.6.2/1.6.2.jar";
		const string pngTerrainPatchUri = "http://static.classicube.net/terrain-patch.png";
		const string pngGuiPatchUri = "http://static.classicube.net/gui.png";
		
		public void DownloadItems( Action<string> setStatus ) {
			downloader.DownloadData( jarClassicUri, false, "classic_jar" );
			downloader.DownloadData( jar162Uri, false, "162_jar" );
			downloader.DownloadData( pngTerrainPatchUri, false, "terrain_patch" );
			downloader.DownloadData( pngGuiPatchUri, false, "gui_patch" );
			setStatus( "&eFetching classic jar.. (1/4)" );
		}
		
		internal byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public bool Check( Action<string> setStatus ) {
			if( Done ) return true;
			
			if( !DownloadItem( "classic_jar", "classic jar",
			                   "&eFetching 1.6.2 jar.. (2/4)", ref jarClassic, setStatus))
				return false;
			
			if( !DownloadItem( "162_jar", "1.6.2 jar",
			                   "&eFetching terrain patch.. (3/4)", ref jar162, setStatus))
				return false;
			
			if( !DownloadItem( "terrain_patch", "terrain.png patch",
			                   "&eFetching gui.. (3/4)", ref pngTerrainPatch, setStatus))
				return false;
			
			if( !DownloadItem( "gui_patch", "gui.png patch",
			                   "&eCreating default.zip..", ref pngGuiPatch, setStatus))
				return false;
			
			if( pngGuiPatch != null ) {
				Done = true;
				return true;
			}
			return false;
		}
		
		bool DownloadItem( string identifier, string name, string next, 
		                  ref byte[] data, Action<string> setStatus ) {
			DownloadedItem item;
			if( downloader.TryGetItem( identifier, out item ) ) {
				Console.WriteLine( "FOUND" + identifier );
				if( item.Data == null ) {
					setStatus( "&cFailed to download " + name ); 
					return false;
				}
				
				setStatus( next );
				data = (byte[])item.Data;
				return true;
			}
			return true;
		}
		
		public static bool CheckAllResourcesExist() {
			return File.Exists( "default.zip" );
		}
		
		public static string EstimateDownloadSize() {
			float size = (291 + 4621 + 7 + 21) / 1024f;
			// classic.jar + 1.6.2.jar + terrain-patch.png + gui.png
			return size.ToString( "F2" );
		}
	}
}
