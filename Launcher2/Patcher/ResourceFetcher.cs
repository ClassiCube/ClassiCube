// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;

namespace Launcher {
	
	public sealed class ResourceFetcher {
		
		public bool Done = false;
		internal AsyncDownloader downloader;
		SoundPatcher digPatcher, stepPatcher;
		public ResourceFetcher() {
			string basePath = Path.Combine( Program.AppDirectory, "audio" );
			digPath = Path.Combine( basePath, "dig" );
			stepPath = Path.Combine( basePath, "step" );
		}
		
		const string jarClassicUri = "http://s3.amazonaws.com/Minecraft.Download/versions/c0.30_01c/c0.30_01c.jar";
		const string jar162Uri = "http://s3.amazonaws.com/Minecraft.Download/versions/1.6.2/1.6.2.jar";
		const string pngTerrainPatchUri = "http://static.classicube.net/terrain-patch.png";
		const string pngGuiPatchUri = "http://static.classicube.net/gui.png";
		const string digSoundsUri = "http://s3.amazonaws.com/MinecraftResources/sound3/dig/";
		const string altDigSoundsUri = "http://s3.amazonaws.com/MinecraftResources/sound3/random/";
		const string stepSoundsUri = "http://s3.amazonaws.com/MinecraftResources/newsound/step/";
		const string altStepSoundsUri = "http://s3.amazonaws.com/MinecraftResources/sound3/step/";
		const string musicUri = "http://s3.amazonaws.com/MinecraftResources/music/";
		const string newMusicUri = "http://s3.amazonaws.com/MinecraftResources/newmusic/";
		
		public void DownloadItems( AsyncDownloader downloader, Action<string> setStatus ) {
			this.downloader = downloader;
			DownloadMusicFiles();
			digPatcher = new SoundPatcher( digSounds, "dig_",
			                              "step_cloth1", digPath );
			digPatcher.FetchFiles( digSoundsUri, altDigSoundsUri, this );
			stepPatcher = new SoundPatcher( stepSounds, "step_",
			                               "classic jar", stepPath );
			stepPatcher.FetchFiles( stepSoundsUri, altStepSoundsUri, this );
			if( !defaultZipExists ) {
				downloader.DownloadData( jarClassicUri, false, "classic_jar" );
				downloader.DownloadData( jar162Uri, false, "162_jar" );
				downloader.DownloadData( pngTerrainPatchUri, false, "terrain_patch" );
				downloader.DownloadData( pngGuiPatchUri, false, "gui_patch" );
			}
			SetFirstStatus( setStatus );
		}
		
		void SetFirstStatus( Action<string> setStatus ) {
			for( int i = 0; i < musicExists.Length; i++ ) {
				if( musicExists[i] ) continue;
				setStatus( MakeNext( musicFiles[i] ) );
				return;
			}
			setStatus( MakeNext( "dig_cloth1" ) );
		}
		
		internal byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public bool Check( Action<string> setStatus ) {
			if( Done ) return true;
			
			if( !CheckMusicFiles( setStatus ) )
				return false;
			if( !digPatcher.CheckDownloaded( this, setStatus ) )
				return false;
			if( !stepPatcher.CheckDownloaded( this, setStatus ) )
				return false;
			
			if( !DownloadItem( "classic_jar", "classic jar",
			                  "1.6.2 jar", ref jarClassic, setStatus ) )
				return false;
			if( !DownloadItem( "162_jar", "1.6.2 jar",
			                  "terrain patch", ref jar162, setStatus ) )
				return false;
			if( !DownloadItem( "terrain_patch", "terrain.png patch",
			                  "gui", ref pngTerrainPatch, setStatus ) )
				return false;
			if( !DownloadItem( "gui_patch", "gui.png patch",
			                  null, ref pngGuiPatch, setStatus ) )
				return false;
			
			bool done = !defaultZipExists ? pngGuiPatch != null :
				stepPatcher.Done;
			if( done ) {
				Done = true;
				return true;
			}
			return true;
		}
		
		bool DownloadItem( string identifier, string name, string next,
		                  ref byte[] data, Action<string> setStatus ) {
			DownloadedItem item;
			if( downloader.TryGetItem( identifier, out item ) ) {
				Console.WriteLine( "got resource " + identifier );
				if( item.Data == null ) {
					setStatus( "&cFailed to download " + name );
					return false;
				}
				
				if( next != null )
					setStatus( MakeNext( next ) );
				else
					setStatus( "&eCreating default.zip.." );
				data = (byte[])item.Data;
				return true;
			}
			return true;
		}
		
		public void CheckResourceExistence() {
			string audioPath = Path.Combine( Program.AppDirectory, "audio" );
			if( !Directory.Exists( audioPath ) )
				Directory.CreateDirectory( audioPath );
			AllResourcesExist = File.Exists( digPath + ".bin" )
				&& File.Exists( stepPath + ".bin" );
			
			string texDir = Path.Combine( Program.AppDirectory, "texpacks" );
			string zipPath = Path.Combine( texDir, "default.zip" );
			defaultZipExists = File.Exists( zipPath );
			if( defaultZipExists )
				CheckClassicGuiPng( zipPath );
			if( !defaultZipExists ) {
				// classic.jar + 1.6.2.jar + terrain-patch.png + gui.png
				DownloadSize += (291 + 4621 + 7 + 21) / 1024f;
				ResourcesCount += 4;
				AllResourcesExist = false;
			}
			
			for( int i = 0; i < musicFiles.Length; i++ ) {
				string file = Path.Combine( audioPath, musicFiles[i] + ".ogg" );
				musicExists[i] = File.Exists( file );
				if( !musicExists[i] ) {
					DownloadSize += musicSizes[i] / 1024f;
					ResourcesCount++;
					AllResourcesExist = false;
				}
			}
			ResourcesCount += digSounds.Length;
			DownloadSize += 173 / 1024f;
			ResourcesCount += stepSounds.Length;
			DownloadSize += 244 / 1024f;
		}
		public bool AllResourcesExist;
		public float DownloadSize;
		public int ResourcesCount, CurrentResource;
		
		const string lineFormat = "&eFetching {0}.. ({1}/{2})";
		public string MakeNext( string next ) {
			CurrentResource++;
			return String.Format( lineFormat, next,
			                     CurrentResource, ResourcesCount );
		}
		
		bool classicGuiPngExists = false;
		void CheckClassicGuiPng( string path ) {
			ZipReader reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry;
			reader.ProcessZipEntry = ProcessZipEntry;
			
			using( Stream src = new FileStream( path, FileMode.Open, FileAccess.Read ) )
				reader.Extract( src );
			if( !classicGuiPngExists )
				defaultZipExists = false;
		}
		
		bool ShouldProcessZipEntry( string filename ) {
			if( filename == "gui_classic.png" )
				classicGuiPngExists = true;
			return false;
		}
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) { }
		
		bool CheckMusicFiles( Action<string> setStatus ) {
			for( int i = 0; i < musicFiles.Length; i++ ) {
				string next = i < musicFiles.Length - 1 ?
					musicFiles[i + 1] : "dig_cloth1";
				string name = musicFiles[i];
				byte[] data = null;
				if( !DownloadItem( name, name, next,
				                  ref data, setStatus ) )
					return false;
				
				if( data == null ) continue;
				string path = Path.Combine( Program.AppDirectory, "audio" );
				path = Path.Combine( path, name + ".ogg" );
				File.WriteAllBytes( path, data );
			}
			return true;
		}
		
		void DownloadMusicFiles() {
			for( int i = 0; i < musicFiles.Length; i++ ) {
				if( musicExists[i] ) continue;
				string baseUri = i < 3 ? musicUri : newMusicUri;
				string url = baseUri + musicFiles[i] + ".ogg";
				downloader.DownloadData( url, false, musicFiles[i] );
			}
		}
		
		string digPath, stepPath;
		string[] digSounds = new [] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bglass1",
			"Bglass2", "Bglass3", "Agrass1", "Agrass2", "Agrass3", "Agrass4", "Agravel1", "Agravel2",
			"Agravel3", "Agravel4", "Asand1", "Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3",
			"Asnow4", "Astone1", "Astone2", "Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		string[] stepSounds = new [] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bgrass1",
			"Bgrass2", "Bgrass3", "Bgrass4", "Agravel1", "Agravel2", "Agravel3", "Agravel4", "Asand1",
			"Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3", "Asnow4", "Astone1", "Astone2",
			"Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		string[] musicFiles = new [] { "calm1", "calm2", "calm3", "hal1", "hal2", "hal3", "hal4" };
		int[] musicSizes = new [] { 2472, 1931, 2181, 1926, 1714, 1879, 2499 };
		bool[] musicExists = new bool[7];
		internal bool defaultZipExists;
	}
}
