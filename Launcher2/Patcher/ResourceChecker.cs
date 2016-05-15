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
			
			for( int i = 0; i < musicFiles.Length; i++ ) {
				string file = Path.Combine( audioPath, musicFiles[i] + ".ogg" );
				musicExists[i] = File.Exists( file );
				if( !musicExists[i] ) {
					DownloadSize += musicSizes[i] / 1024f;
					ResourcesCount++;
					AllResourcesExist = false;
				}
			}
			
			if( !DigSoundsExist ) {
				ResourcesCount += digSounds.Length;
				DownloadSize += 173 / 1024f;
			}
			if( !StepSoundsExist ) {
				ResourcesCount += stepSounds.Length;
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
			string path = Path.Combine( Program.AppDirectory, "audio" );
			for( int i = 0; i < digSounds.Length; i++ ) {
				string file = "dig_" + digSounds[i].Substring( 1 ) + ".wav";
				if( !File.Exists( Path.Combine( path, file ) ) ) return false;
			}
			return true;
		}
		
		bool CheckStepSoundsExist() {
			string path = Path.Combine( Program.AppDirectory, "audio" );
			for( int i = 0; i < stepSounds.Length; i++ ) {
				string file = "step_" + stepSounds[i].Substring( 1 ) + ".wav";
				if( !File.Exists( Path.Combine( path, file ) ) ) return false;
			}
			return true;
		}
		
		internal static string[] digSounds = new [] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bglass1",
			"Bglass2", "Bglass3", "Agrass1", "Agrass2", "Agrass3", "Agrass4", "Agravel1", "Agravel2",
			"Agravel3", "Agravel4", "Asand1", "Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3",
			"Asnow4", "Astone1", "Astone2", "Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		internal static string[] stepSounds = new [] { "Acloth1", "Acloth2", "Acloth3", "Acloth4", "Bgrass1",
			"Bgrass2", "Bgrass3", "Bgrass4", "Agravel1", "Agravel2", "Agravel3", "Agravel4", "Asand1",
			"Asand2", "Asand3", "Asand4", "Asnow1", "Asnow2", "Asnow3", "Asnow4", "Astone1", "Astone2",
			"Astone3", "Astone4", "Awood1", "Awood2", "Awood3", "Awood4" };
		
		internal static string[] musicFiles = new [] { "calm1", "calm2", "calm3", "hal1", "hal2", "hal3", "hal4" };
		static int[] musicSizes = new [] { 2472, 1931, 2181, 1926, 1714, 1879, 2499 };
	}
}
