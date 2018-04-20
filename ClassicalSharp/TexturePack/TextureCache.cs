// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using System.Text;
#if ANDROID
using Android.Graphics;
#endif
using PathIO = System.IO.Path; // Android.Graphics.Path clash otherwise

namespace ClassicalSharp.Textures {
	
	/// <summary> Caches terrain atlases and texture packs to avoid making redundant downloads. </summary>
	public static class TextureCache {
		
		const string folder = "texturecache";
		static EntryList Accepted     = new EntryList(folder, "acceptedurls.txt"); 
		static EntryList Denied       = new EntryList(folder, "deniedurls.txt");
		static EntryList ETags        = new EntryList(folder, "etags.txt");
		static EntryList LastModified = new EntryList(folder, "lastmodified.txt");
		
		public static void Init() {
			Accepted.Load();
			Denied.Load();
			ETags.Load();
			LastModified.Load();
		}
		
		public static bool HasAccepted(string url) { return Accepted.Has(url); }
		public static bool HasDenied(string url)   { return Denied.Has(url); }		
		public static void Accept(string url) { Accepted.Add(url); }
		public static void Deny(string url)   { Denied.Add(url); }
		
		public static bool HasUrl(string url) { 
			return Platform.FileExists(MakePath(url)); 
		}
		
		public static Stream GetStream(string url) {
			string path = MakePath(url);
			if (!Platform.FileExists(path)) return null;
			
			try {
				return Platform.FileOpen(path);
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.GetData", ex);
				return null;
			}
		}
		
		public static DateTime GetLastModified(string url) {
			string entry = GetFromTags(url, LastModified);
			long ticks = 0;
			if (entry != null && long.TryParse(entry, out ticks)) {
				return new DateTime(ticks, DateTimeKind.Utc);
			} else {
				string path = MakePath(url);
				return Platform.FileGetWriteTime(path);
			}
		}

		public static string GetETag(string url) {
			return GetFromTags(url, ETags);
		}
		
		static string GetFromTags(string url, EntryList tags) {
			string crc32 = CRC32(url);
			
			for (int i = 0; i < tags.Entries.Count; i++) {
				string entry = tags.Entries[i];
				if (!entry.StartsWith(crc32)) continue;
				
				int sepIndex = entry.IndexOf(' ');
				if (sepIndex == -1) continue;
				return entry.Substring(sepIndex + 1);
			}
			return null;
		}
		
		
		/// <summary> Adds the url and the bitmap associated with it to the cache. </summary>
		public static void Add(string url, Bitmap bmp) {
			string path = MakePath(url);
			try {
				using (Stream fs = Platform.FileCreate(path)) {
					Platform.WriteBmp(bmp, fs);
				}
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.AddToCache", ex);
			}
		}
		
		/// <summary> Adds the url and the data associated with it to the cache. </summary>
		public static void Add(string url, byte[] data) {
			string path = MakePath(url);
			try {
				Platform.WriteAllBytes(path, data);
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.AddToCache", ex);
			}
		}
		
		public static void AddETag(string url, string etag) {
			if (etag == null) return;
			AddToTags(url, etag, ETags);
		}
		
		public static void AdddLastModified(string url, DateTime lastModified) {
			if (lastModified == DateTime.MinValue) return;
			string data = lastModified.ToUniversalTime().Ticks.ToString();
			AddToTags(url, data, LastModified);
		}
		
		static void AddToTags(string url, string data, EntryList tags) {
			string crc32 = CRC32(url);
			string entry = crc32 + " " + data;
			
			for (int i = 0; i < tags.Entries.Count; i++) {
				if (!tags.Entries[i].StartsWith(crc32)) continue;
				tags.Entries[i] = entry;
				tags.Save(); return;
			}
			tags.Add(entry);
		}
		
		static string MakePath(string url) { return PathIO.Combine(folder, CRC32(url)); }
		
		static string CRC32(string url) {
			byte[] data = Encoding.UTF8.GetBytes(url);
			uint result = Utils.CRC32(data, data.Length);
			return result.ToString();
		}
	}
}
