// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.TexturePack;

namespace Launcher {
	
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
			defaultZipExists = File.Exists( zipPath );
			if( defaultZipExists )
				CheckClassicGuiPng( zipPath );
			if( !defaultZipExists ) {
				// classic.jar + 1.6.2.jar + terrain-patch.png + gui.png
				DownloadSize += (291 + 4621 + 7 + 21) / 1024f;
				ResourcesCount += 4;
				AllResourcesExist = false;
			}
			
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
		internal bool classicGuiPngExists, defaultZipExists;
		
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
		static int[] musicSizes = new [] { 2472, 1931, 2181, 1926, 1714, 1879, 2499 };
	}
}
