// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using ClassicalSharp.Network;

namespace Launcher.Patcher {
	
	public sealed class ResourceFetcher {
		
		public bool Done = false;
		internal AsyncDownloader downloader;
		SoundPatcher digPatcher, stepPatcher;
		
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
		
		ushort flags;
		public void DownloadItems(AsyncDownloader downloader, Action<string> setStatus) {
			this.downloader = downloader;
			DownloadMusicFiles();
			digPatcher = new SoundPatcher(ResourceList.DigSounds, "dig_", "step_cloth1");
			digPatcher.FetchFiles(digSoundsUri, altDigSoundsUri, this, DigSoundsExist);
			stepPatcher = new SoundPatcher(ResourceList.StepSounds, "step_", "classic jar");
			stepPatcher.FetchFiles(stepSoundsUri, altStepSoundsUri, this, StepSoundsExist);
			
			flags = 0;
			foreach (var entry in ResourceList.Files)
				flags |= entry.Value;
			
			if ((flags & ResourceList.cMask) != 0)
				downloader.DownloadData(jarClassicUri, false, "classic_jar");
			if ((flags & ResourceList.mMask) != 0)
				downloader.DownloadData(jar162Uri, false, "162_jar");
			if ((flags & ResourceList.gMask) != 0)
				downloader.DownloadData(pngGuiPatchUri, false, "gui_patch");
			if ((flags & ResourceList.tMask) != 0)
				downloader.DownloadData(pngTerrainPatchUri, false, "terrain_patch");
			SetFirstStatus(setStatus);
		}
		
		void DownloadMusicFiles() {
			string[] files = ResourceList.MusicFiles;
			for (int i = 0; i < files.Length; i++) {
				if (musicExists[i]) continue;
				string baseUri = i < 3 ? musicUri : newMusicUri;
				string url = baseUri + files[i] + ".ogg";
				downloader.DownloadData(url, false, files[i]);
			}
		}
		
		public void AddDownload(string url, string identifier) {
			downloader.DownloadData(url, false, identifier);
		}
		
		void SetFirstStatus(Action<string> setStatus) {
			for (int i = 0; i < musicExists.Length; i++) {
				if (musicExists[i]) continue;
				setStatus(MakeNext(ResourceList.MusicFiles[i]));
				return;
			}
			setStatus(MakeNext(FirstItem()));
		}
		
		
		internal byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public bool Check(Action<string> setStatus) {
			if (Done) return true;
			
			if (!CheckMusicFiles(setStatus))
				return false;
			if (!digPatcher.CheckDownloaded(this, setStatus))
				return false;
			if (!stepPatcher.CheckDownloaded(this, setStatus))
				return false;
			
			if (!Download("classic_jar", "classic jar", "1.6.2 jar", ref jarClassic, setStatus))
				return false;
			if (!Download("162_jar", "1.6.2 jar", "gui.png", ref jar162, setStatus))
				return false;
			if (!Download("gui_patch", "gui.png patch", "terrain.png patch", ref pngGuiPatch, setStatus))
				return false;
			if (!Download("terrain_patch", "terrain.png patch", null, ref pngTerrainPatch, setStatus))
				return false;

			Done |= IsDone();
			return true;
		}
		
		string FirstItem() {
			if (!DigSoundsExist) return "dig_cloth1";
			if (!StepSoundsExist) return "step_cloth1";
			
			if ((flags & ResourceList.cMask) != 0) 
				return "classic jar";
			if ((flags & ResourceList.mMask) != 0) 
				return "1.6.2 jar";
			if ((flags & ResourceList.gMask) != 0) 
				return "gui.png patch";
			if ((flags & ResourceList.tMask) != 0) 
				return "terrain.png patch";
			return "(unknown)";
		}
		
		bool IsDone() {
			if (flags == 0) return stepPatcher.Done;
			if ((flags & ResourceList.tMask) != 0) 
				return pngTerrainPatch != null;
			if ((flags & ResourceList.gMask) != 0) 
				return pngGuiPatch != null;
			if ((flags & ResourceList.mMask) != 0) 
				return jar162 != null;
			if ((flags & ResourceList.cMask) != 0) 
				return jarClassic != null;
			return true;
		}
		
		bool Download(string identifier, string name, string next,
		              ref byte[] data, Action<string> setStatus) {
			DownloadedItem item;
			if (downloader.TryGetItem(identifier, out item)) {
				Console.WriteLine("got resource " + identifier);
				if (item.Data == null) {
					setStatus("&cFailed to download " + name);
					return false;
				}
				
				if (next != null)
					setStatus(MakeNext(next));
				else
					setStatus("&eCreating default.zip..");
				data = (byte[])item.Data;
				return true;
			}
			return true;
		}
		
		const string lineFormat = "&eFetching {0}.. ({1}/{2})";
		public string MakeNext(string next) {
			CurrentResource++;
			return String.Format(lineFormat, next,
			                     CurrentResource, ResourcesCount);
		}
		
		bool CheckMusicFiles(Action<string> setStatus) {
			string[] files = ResourceList.MusicFiles;
			for (int i = 0; i < files.Length; i++) {
				string next = i < files.Length - 1 ? files[i + 1] : "dig_cloth1";
				string name = files[i];
				byte[] data = null;
				if (!Download(name, name, next, ref data, setStatus))
					return false;
				
				if (data == null) continue;
				string path = Path.Combine(Program.AppDirectory, "audio");
				path = Path.Combine(path, name + ".ogg");
				File.WriteAllBytes(path, data);
			}
			return true;
		}
		
		public void CheckResourceExistence() {
			ResourceChecker checker = new ResourceChecker();
			checker.CheckResourceExistence();
			
			AllResourcesExist = checker.AllResourcesExist;
			DigSoundsExist = checker.DigSoundsExist;
			StepSoundsExist = checker.StepSoundsExist;
			
			DownloadSize = checker.DownloadSize;
			ResourcesCount = checker.ResourcesCount;
			musicExists = checker.musicExists;
		}
		
		public bool AllResourcesExist, DigSoundsExist, StepSoundsExist;
		public float DownloadSize;
		public int ResourcesCount, CurrentResource;
		bool[] musicExists = new bool[7];
	}
}
