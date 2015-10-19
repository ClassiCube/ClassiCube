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
		
		public void DownloadItems( Action<string> setStatus ) {
			downloader.DownloadData( "http://s3.amazonaws.com/Minecraft.Download/versions/c0.30_01c/c0.30_01c.jar", false, "classic_jar" );
			downloader.DownloadData( "http://s3.amazonaws.com/Minecraft.Download/versions/1.6.2/1.6.2.jar", false, "162_jar" );
			downloader.DownloadData( "http://static.classicube.net/terrain-patch.gpng", false, "terrain_patch" );
			setStatus( "&eFetching classic jar.. (1/3)" );
		}
		
		internal byte[] jarClassic, jar162, pngTerrainPatch;
		public bool Check( Action<string> setStatus ) {
			if( Done ) return true;
			
			DownloadedItem item;
			if( downloader.TryGetItem( "classic_jar", out item ) ) {
				if( item.Data == null ) {
					setStatus( "&cFailed to download classic jar" ); return false;
				}
				setStatus( "&eFetching 1.6.2 jar.. (2/3)" );
				jarClassic = (byte[])item.Data;
			}
			
			if( downloader.TryGetItem( "162_jar", out item ) ) {
				if( item.Data == null ) {
					setStatus( "&cFailed to download 1.6.2 jar" ); return false;
				}
				setStatus( "&eFetching terrain patch.. (3/3)" );
				jar162 = (byte[])item.Data;
			}
			
			if( downloader.TryGetItem( "terrain_patch", out item ) ) {
				if( item.Data == null ) {
					setStatus( "&cFailed to download terrain patch" ); return false;
				}
				setStatus( "&eCreating default.zip.." );
				pngTerrainPatch = (byte[])item.Data;
				Done = true;
			}
			return true;
		}
		
		public static bool CheckAllResourcesExist() {
			return File.Exists( "default.zip" );
		}
		
		public static float EstimateDownloadSize() {
			float sum = 0;
			if( !File.Exists( "classic.jar" ) ) sum += 291 / 1024f;
			if( !File.Exists( "1.6.2.jar" ) ) sum += 4621 / 1024f;
			if( !File.Exists( "terrain-patch.png" ) ) sum += 7 / 1024f;
			return sum;
		}
	}
}
