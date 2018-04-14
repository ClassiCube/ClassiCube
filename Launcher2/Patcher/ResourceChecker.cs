// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Textures;

namespace Launcher.Patcher {
	
	public sealed class ResourceChecker {

		public void CheckResourceExistence() {
			if (!Platform.DirectoryExists("audio")) {
				Platform.DirectoryCreate("audio");
			}
			
			DigSoundsExist = CheckDigSoundsExist();
			StepSoundsExist = CheckStepSoundsExist();
			AllResourcesExist = DigSoundsExist && StepSoundsExist;
			
			string defPath = Path.Combine("texpacks", "default.zip");
			if (Platform.FileExists(defPath)) {
				CheckDefaultZip(defPath);
			}
			
			CheckTexturePack();
			CheckMusic();
			CheckSounds();
		}
		
		void CheckTexturePack() {
			byte flags = ResourceList.GetFetchFlags();
			if (flags != 0) AllResourcesExist = false;
			
			if ((flags & ResourceList.mask_classic) != 0) {
				DownloadSize += 291/1024f; ResourcesCount++;
			}
			if ((flags & ResourceList.mask_modern) != 0) {
				DownloadSize += 4621/1024f; ResourcesCount++;
			}
			if ((flags & ResourceList.mask_terrain) != 0) {
				DownloadSize += 7/1024f; ResourcesCount++;
			}
			if ((flags & ResourceList.mask_gui) != 0) {
				DownloadSize += 21/1024f; ResourcesCount++;
			}
		}
		
		void CheckMusic() {
			string[] files = ResourceList.MusicFiles;
			for (int i = 0; i < files.Length; i++) {
				string path = Path.Combine("audio", files[i]);
				musicExists[i] = Platform.FileExists(path);
				if (musicExists[i]) continue;
				
				DownloadSize += musicSizes[i] / 1024f;
				ResourcesCount++;
				AllResourcesExist = false;
			}
		}
		
		void CheckSounds() {
			if (!DigSoundsExist) {
				ResourcesCount += ResourceList.DigSounds.Length;
				DownloadSize += 173 / 1024f;
			}
			if (!StepSoundsExist) {
				ResourcesCount += ResourceList.StepSounds.Length;
				DownloadSize += 244 / 1024f;
			}
		}
		
		public bool AllResourcesExist, DigSoundsExist, StepSoundsExist;
		public float DownloadSize;
		public int ResourcesCount;
		internal bool[] musicExists = new bool[7];
		
		void CheckDefaultZip(string relPath) {
			ZipReader reader = new ZipReader();
			reader.SelectZipEntry = SelectZipEntry;
			reader.ProcessZipEntry = ProcessZipEntry;
			
			using (Stream src = Platform.FileOpen(relPath)) {
				reader.Extract(src);
			}
		}

		bool SelectZipEntry(string filename) {
			string name = ResourceList.GetFile(filename);
			for (int i = 0; i < ResourceList.Filenames.Length; i++) {
				if (ResourceList.FilesExist[i]) continue;
				if (name != ResourceList.Filenames[i]) continue;
				
				ResourceList.FilesExist[i] = true;
				break;
			}
			return false;
		}
		
		void ProcessZipEntry(string filename, byte[] data, ZipEntry entry) { }
		
		bool CheckDigSoundsExist() {
			string[] files = ResourceList.DigSounds;
			for (int i = 0; i < files.Length; i++) {
				string path = Path.Combine("audio", "dig_" + files[i] + ".wav");
				if (!Platform.FileExists(path)) return false;
			}
			return true;
		}
		
		bool CheckStepSoundsExist() {
			string[] files = ResourceList.StepSounds;
			for (int i = 0; i < files.Length; i++) {
				string path = Path.Combine("audio", "step_" + files[i] + ".wav");
				if (!Platform.FileExists(path)) return false;
			}
			return true;
		}
		static int[] musicSizes = new int[] { 2472, 1931, 2181, 1926, 1714, 1879, 2499 };
	}
}
