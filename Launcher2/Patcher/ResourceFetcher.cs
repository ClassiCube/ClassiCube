// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Network;

namespace Launcher.Patcher {
	
	public sealed class ResourceFetcher {
		
		public bool Done = false;
		internal AsyncDownloader downloader;
		SoundPatcher digPatcher, stepPatcher;
		public List<string> FilesToDownload = new List<string>();
		
		public void QueueItem(string url, string identifier) {
			downloader.AsyncGetData(url, false, identifier);
			FilesToDownload.Add(identifier);
		}
		
		const string jarClassicUri = "http://launcher.mojang.com/mc/game/c0.30_01c/client/54622801f5ef1bcc1549a842c5b04cb5d5583005/client.jar";
		const string jar162Uri = "https://launcher.mojang.com/mc/game/1.6.2/client/b6cb68afde1d9cf4a20cbf27fa90d0828bf440a4/client.jar";
		const string pngTerrainPatchUri = "http://static.classicube.net/terrain-patch2.png";
		const string pngGuiPatchUri = "http://static.classicube.net/gui.png";
		public const string assetsUri = "http://resources.download.minecraft.net/";
		
		public void DownloadItems(AsyncDownloader downloader, Action<string> setStatus) {
			this.downloader = downloader;		
			byte fetchFlags = ResourceList.GetFetchFlags();
			
			if ((fetchFlags & ResourceList.mask_classic) != 0)
				QueueItem(jarClassicUri, "classic jar");
			if ((fetchFlags & ResourceList.mask_modern) != 0)
				QueueItem(jar162Uri, "1.6.2 jar");
			if ((fetchFlags & ResourceList.mask_gui) != 0)
				QueueItem(pngGuiPatchUri, "gui.png patch");
			if ((fetchFlags & ResourceList.mask_terrain) != 0)
				QueueItem(pngTerrainPatchUri, "terrain.png patch");
			
			DownloadMusicFiles();
			digPatcher = new SoundPatcher(ResourceList.DigSounds, ResourceList.DigHashes, "dig_");
			digPatcher.FetchFiles(this, DigSoundsExist);
			// seems step sounds are just dig sounds
			stepPatcher = new SoundPatcher(ResourceList.StepSounds, ResourceList.DigHashes, "step_");
			stepPatcher.FetchFiles(this, StepSoundsExist);
			
			setStatus(MakeNext());
		}
		
		void DownloadMusicFiles() {
			string[] files = ResourceList.MusicFiles;
			string[] hashes = ResourceList.MusicHashes;
			for (int i = 0; i < files.Length; i++) {
				if (musicExists[i]) continue;
				QueueItem(assetsUri + hashes[i], files[i]);
			}
		}
		
		public void AddDownload(string url, string identifier) {
			downloader.AsyncGetData(url, false, identifier);
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
			Request item;
			if (downloader.TryGetItem(identifier, out item)) {
				FilesToDownload.RemoveAt(0);
				Utils.LogDebug("got resource " + identifier);
				
				if (item.Data == null) {
					if (item.WebEx != null) {
						ErrorHandler.LogError("ResourceFetcher.Download " + identifier, item.WebEx);
					}
					
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
				string file = files[i];
				byte[] data = null;
				if (!Download(file, ref data, setStatus))
					return false;
				
				if (data == null) continue;
				string path = Path.Combine(Program.AppDirectory, "audio");
				path = Path.Combine(path, file);
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
