using System;
using System.IO;
using ClassicalSharp.Network;

namespace Launcher2 {
	
	public sealed class ResourceFetcher {
		
		public bool Done = false;
		internal AsyncDownloader downloader;
		public ResourceFetcher() {
			digPath = Path.Combine( "audio", "dig" );
			stepPath = Path.Combine( "audio", "step" );
		}
		
		const string jarClassicUri = "http://s3.amazonaws.com/Minecraft.Download/versions/c0.30_01c/c0.30_01c.jar";
		const string jar162Uri = "http://s3.amazonaws.com/Minecraft.Download/versions/1.6.2/1.6.2.jar";
		const string pngTerrainPatchUri = "http://static.classicube.net/terrain-patch.png";
		const string pngGuiPatchUri = "http://static.classicube.net/gui.png";
		const string digSoundsUri = "http://s3.amazonaws.com/MinecraftResources/sound3/dig/";
		const string stepSoundsUri = "http://s3.amazonaws.com/MinecraftResources/sound3/step/";
		const string musicUri = "http://s3.amazonaws.com/MinecraftResources/music/";
		
		public void DownloadItems( AsyncDownloader downloader, Action<string> setStatus ) {
			this.downloader = downloader;
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
		
		public void CheckResourceExistence() {
			AllResourcesExist = File.Exists( "default.zip" );
				//&& Directory.Exists( "audio" ) && File.Exists( digPath + ".bin" )
				//&& File.Exists( stepPath + ".bin" );
			
			if( !File.Exists( "default.zip" ) ) {
				// classic.jar + 1.6.2.jar + terrain-patch.png + gui.png
				DownloadSize += (291 + 4621 + 7 + 21) / 1024f;
				ResourcesCount += 4;
			}
			
			for( int i = 0; i < musicFiles.Length; i++ ) {
				string file = Path.Combine( "audio", musicFiles[i] + ".ogg" );
				musicExists[i] = File.Exists( file );
				continue;
				// TODO: download music files
				if( !musicExists[i] ) {
					DownloadSize += musicSizes[i] / 1024f;
					ResourcesCount++;
				}
			}
			ResourcesCount += digSounds.Length;
			ResourcesCount += stepSounds.Length;
		}
		public bool AllResourcesExist;
		public float DownloadSize;
		public int ResourcesCount;
		
		string digPath, stepPath;
		string[] digSounds = new [] { "cloth1", "cloth2", "cloth3", "cloth4", 
			"glass1", "glass2", "glass3", "glass4", "grass1", "grass2", "grass3", 
			"grass4", "gravel1", "gravel2", "gravel3", "gravel4", "sand1", "sand2", 
			"sand3", "sand4", "snow1", "snow2", "snow3", "snow4", "stone1", "stone2", 
			"stone3", "stone4", "wood1", "wood2", "wood3", "wood4" };
		
		string[] stepSounds = new [] { "cloth1", "cloth2", "cloth3", "cloth4", "grass1", 
			"grass2", "grass3", "grass4", "grass5", "grass6", "gravel1", "gravel2", 
			"gravel3", "gravel4", "sand1", "sand2", "sand3", "sand4", "sand5", "snow1", 
			"snow2", "snow3", "snow4", "stone1", "stone2", "stone3", "stone4", "stone5", 
			"stone6", "wood1", "wood2", "wood3", "wood4", "wood5", "wood6" };
		
		string[] musicFiles = new [] { "calm1", "calm2", 
			"calm3", "hal1", "hal2", "hal3" };
		int[] musicSizes = new [] { 2472, 1931, 2181, 1926, 1714, 1879, 2499 };
		bool[] musicExists = new bool[6];
	}
}
