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
		
		/// <summary> Gets whether the given url has data associated with it in the cache. </summary>
		public static bool HasUrl(string url) {
			return File.Exists(MakePath(url));
		}

		/// <summary> Gets the stream of data associated with the url from the cache, returning null if the
		/// data for the url was not found in the cache. </summary>
		public static FileStream GetStream(string url) {
			string path = MakePath(url);
			if (!File.Exists(path)) return null;
			
			try {
				return File.OpenRead(path);
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.GetData", ex);
				return null;
			}
		}
		
		/// <summary> Gets the time the data associated with the url from the cache was last modified,
		/// returning DateTime.MinValue if data for the url was not found in the cache. </summary>
		public static DateTime GetLastModified(string url, EntryList tags) {
			string entry = GetFromTags(url, tags);
			long ticks = 0;
			if (entry != null && long.TryParse(entry, out ticks))
				return new DateTime(ticks, DateTimeKind.Utc);
			
			string path = MakePath(url);
			if (!File.Exists(path)) return DateTime.MinValue;			
			return File.GetLastWriteTimeUtc(path);
		}

		public static string GetETag(string url, EntryList tags) {
			return GetFromTags(url, tags);
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
				string basePath = PathIO.Combine(Program.AppDirectory, Folder);
				if (!Directory.Exists(basePath))
					Directory.CreateDirectory(basePath);
				
				using (FileStream fs = File.Create(path))
					Platform.WriteBmp(bmp, fs);
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.AddToCache", ex);
			}
		}
		
		/// <summary> Adds the url and the data associated with it to the cache. </summary>
		public static void Add(string url, byte[] data) {
			string path = MakePath(url);
			try {
				string basePath = PathIO.Combine(Program.AppDirectory, Folder);
				if (!Directory.Exists(basePath))
					Directory.CreateDirectory(basePath);
				
				File.WriteAllBytes(path, data);
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.AddToCache", ex);
			}
		}
		
		public static void AddETag(string url, string etag, EntryList tags) {
			if (etag == null) return;
			AddToTags(url, etag, tags);
		}
		
		public static void AdddLastModified(string url, DateTime lastModified, EntryList tags) {
			if (lastModified == DateTime.MinValue) return;
			string data = lastModified.ToUniversalTime().Ticks.ToString();
			AddToTags(url, data, tags);
		}
		
		static void AddToTags(string url, string data, EntryList tags) {
			string crc32 = CRC32(url);
			for (int i = 0; i < tags.Entries.Count; i++) {
				if (!tags.Entries[i].StartsWith(crc32)) continue;
				tags.Entries[i] = crc32 + " " + data;
				tags.Save(); return;
			}
			tags.AddEntry(crc32 + " " + data);
		}
		
		
		const string Folder = "texturecache";
		
		static string MakePath(string url) {
			string crc32 = CRC32(url);
			string basePath = PathIO.Combine(Program.AppDirectory, Folder);
			return PathIO.Combine(basePath, crc32);
		}
		
		static string CRC32(string url) {
			byte[] data = Encoding.UTF8.GetBytes(url);
			uint crc = 0xffffffffU;
			
			for (int i = 0; i < data.Length; i++) {
				crc ^= data[i];
				for (int j = 0; j < 8; j++)
					crc = (crc >> 1) ^ (crc & 1) * 0xEDB88320;
			}
			
			uint result = crc ^ 0xffffffffU;
			return result.ToString();
		}
	}
}
