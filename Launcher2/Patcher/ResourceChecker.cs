// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Textures;

namespace Launcher.Patcher {
	
	public sealed class ResourceChecker {

		public void CheckResourceExistence() {
			string audioPath = Path.Combine( Program.AppDirectory, "audio" );
			if( !Directory.Exists( audioPath ) )
				Directory.CreateDirectory( audioPath );
			DigSoundsExist = CheckDigSoundsExist();
			StepSoundsExist = CheckStepSoundsExist();
			AllResourcesExist = DigSoundsExist && StepSoundsExist;
			
			string texDir = Path.Combine( Program.AppDirectory, "texpacks" );
			string zipPath = Path.Combine( texDir, "default.zip" );
			bool defaultZipExists = File.Exists( zipPath );
			if( File.Exists( zipPath ) )
				CheckDefaultZip( zipPath );
			
			CheckTexturePack();
			CheckMusic( audioPath );
			CheckSounds();
		}
		
		void CheckTexturePack() {
			ushort flags = 0;
			foreach( var entry in ResourceList.Files )
				flags |= entry.Value;
			if( flags != 0 ) AllResourcesExist = false;
			
			if( (flags & ResourceList.cMask) != 0 ) {
				DownloadSize += 291/1024f; ResourcesCount++;
			}
			if( (flags & ResourceList.mMask) != 0 ) {
				DownloadSize += 4621/1024f; ResourcesCount++;
			}
			if( (flags & ResourceList.tMask) != 0 ) {
				DownloadSize += 7/1024f; ResourcesCount++;
			}
			if( (flags & ResourceList.gMask) != 0 ) {
				DownloadSize += 21/1024f; ResourcesCount++;
			}
		}
		
		void CheckMusic( string audioPath ) {
			string[] files = ResourceList.MusicFiles;
			for( int i = 0; i < files.Length; i++ ) {
				string file = Path.Combine( audioPath, files[i] + ".ogg" );
				musicExists[i] = File.Exists( file );
				if( !musicExists[i] ) {
					DownloadSize += musicSizes[i] / 1024f;
					ResourcesCount++;
					AllResourcesExist = false;
				}
			}
		}
		
		void CheckSounds() {
			if( !DigSoundsExist ) {
				ResourcesCount += ResourceList.DigSounds.Length;
				DownloadSize += 173 / 1024f;
			}
			if( !StepSoundsExist ) {
				ResourcesCount += ResourceList.StepSounds.Length;
				DownloadSize += 244 / 1024f;
			}
		}
		
		public bool AllResourcesExist, DigSoundsExist, StepSoundsExist;
		public float DownloadSize;
		public int ResourcesCount;
		internal bool[] musicExists = new bool[7];
		
		void CheckDefaultZip( string path ) {
			ZipReader reader = new ZipReader();
			reader.ShouldProcessZipEntry = ShouldProcessZipEntry;
			reader.ProcessZipEntry = ProcessZipEntry;
			
			using( Stream src = new FileStream( path, FileMode.Open, FileAccess.Read ) )
				reader.Extract( src );
		}

		bool ShouldProcessZipEntry( string filename ) {			
			string name = ResourceList.GetFile( filename );
			ResourceList.Files.Remove( name );
			return false;
		}
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) { }
		
		bool CheckDigSoundsExist() {
			string[] files = ResourceList.DigSounds;
			string path = Path.Combine( Program.AppDirectory, "audio" );
			for( int i = 0; i < files.Length; i++ ) {
				string file = "dig_" + files[i].Substring( 1 ) + ".wav";
				if( !File.Exists( Path.Combine( path, file ) ) ) return false;
			}
			return true;
		}
		
		bool CheckStepSoundsExist() {
			string[] files = ResourceList.StepSounds;
			string path = Path.Combine( Program.AppDirectory, "audio" );
			for( int i = 0; i < files.Length; i++ ) {
				string file = "step_" + files[i].Substring( 1 ) + ".wav";
				if( !File.Exists( Path.Combine( path, file ) ) ) return false;
			}
			return true;
		}
		static int[] musicSizes = new int[] { 2472, 1931, 2181, 1926, 1714, 1879, 2499 };
	}
}
