// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.Network;

namespace Launcher.Patcher {
	
	public sealed class ResourceFetcher {
		
		public bool Done = false;
		internal AsyncDownloader downloader;
		SoundPatcher digPatcher, stepPatcher;
		public List<string> FilesToDownload = new List<string>();
		
		public void QueueItem(string url, string identifier) {
			downloader.DownloadData(url, false, identifier);
			FilesToDownload.Add(identifier);
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
		
		ushort flags;
		public void DownloadItems(AsyncDownloader downloader, Action<string> setStatus) {
			this.downloader = downloader;
			
			flags = 0;
			foreach (var entry in ResourceList.Files)
				flags |= entry.Value;
			
			if ((flags & ResourceList.cMask) != 0)
				QueueItem(jarClassicUri, "classic jar");
			if ((flags & ResourceList.mMask) != 0)
				QueueItem(jar162Uri, "1.6.2 jar");
			if ((flags & ResourceList.gMask) != 0)
				QueueItem(pngGuiPatchUri, "gui.png patch");
			if ((flags & ResourceList.tMask) != 0)
				QueueItem(pngTerrainPatchUri, "terrain.png patch");
			
			DownloadMusicFiles();
			digPatcher = new SoundPatcher(ResourceList.DigSounds, "dig_");
			digPatcher.FetchFiles(digSoundsUri, altDigSoundsUri, this, DigSoundsExist);
			stepPatcher = new SoundPatcher(ResourceList.StepSounds, "step_");
			stepPatcher.FetchFiles(stepSoundsUri, altStepSoundsUri, this, StepSoundsExist);
			
			setStatus(MakeNext());
		}
		
		void DownloadMusicFiles() {
			string[] files = ResourceList.MusicFiles;
			for (int i = 0; i < files.Length; i++) {
				if (musicExists[i]) continue;
				string baseUri = i < 3 ? musicUri : newMusicUri;
				string url = baseUri + files[i] + ".ogg";
				QueueItem(url, files[i]);
			}
		}
		
		public void AddDownload(string url, string identifier) {
			downloader.DownloadData(url, false, identifier);
		}
		
		
		internal byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public bool Check(Action<string> setStatus) {
			if (Done) return true;
			
			if (!Download("classic jar", ref jarClassic, setStatus))
				return false;
			if (!Download("1.6.2 jar", ref jar162, setStatus))
				return false;
			if (!Download("gui.png patch", ref pngGuiPatch, setStatus))
				return false;
			if (!Download("terrain.png patch", ref pngTerrainPatch, setStatus))
				return false;			
			
			if (!CheckMusicFiles(setStatus))
				return false;
			if (!digPatcher.CheckDownloaded(this, setStatus))
				return false;
			if (!stepPatcher.CheckDownloaded(this, setStatus))
				return false;

			Done |= FilesToDownload.Count == 0;
			return true;
		}
		
		bool Download(string identifier, ref byte[] data, Action<string> setStatus) {
			DownloadedItem item;
			if (downloader.TryGetItem(identifier, out item)) {
				FilesToDownload.RemoveAt(0);
				Console.WriteLine("got resource " + identifier);
				
				if (item.Data == null) {
					setStatus("&cFailed to download " + identifier);
					return false;
				}
				
				setStatus(MakeNext());
				data = (byte[])item.Data;
				return true;
			}
			return true;
		}
		
		const string lineFormat = "&eFetching {0}.. ({1}/{2})";
		public string MakeNext() {
			CurrentResource++;
			string next = "&eCreating default.zip..";
			if (FilesToDownload.Count > 0)
				next = FilesToDownload[0];
			
			return String.Format(lineFormat, next,
			                     CurrentResource, ResourcesCount);
		}
		
		bool CheckMusicFiles(Action<string> setStatus) {
			string[] files = ResourceList.MusicFiles;
			for (int i = 0; i < files.Length; i++) {
				string name = files[i];
				byte[] data = null;
				if (!Download(name, ref data, setStatus))
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
