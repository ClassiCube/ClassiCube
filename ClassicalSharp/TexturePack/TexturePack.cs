// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using ClassicalSharp.Network;
#if ANDROID
using Android.Graphics;
#endif
using PathIO = System.IO.Path; // Android.Graphics.Path clash otherwise

namespace ClassicalSharp.Textures {
	
	/// <summary> Extracts resources from a .zip texture pack. </summary>
	public sealed class TexturePack {
		
		static Game game;
		
		public static void ExtractDefault(Game game) {
			ExtractZip(game.DefaultTexturePack, game);
			game.World.TextureUrl = null;
		}
		
		public static void ExtractCurrent(Game game, string url) {
			if (url == null) { ExtractDefault(game); return; }
			
			using (Stream data = TextureCache.GetStream(url)) {
				if (data == null) { // e.g. 404 errors
					if (game.World.TextureUrl != null) ExtractDefault(game);
				} else if (url == game.World.TextureUrl) {
				} else if (url.Contains(".zip")) {
					game.World.TextureUrl = url;
					ExtractZip(data, game);
				} else {
					ExtractTerrainPng(game, data, url);
				}
			}
		}
		
		public static void ExtractZip(string file, Game game) {
			string path = PathIO.Combine("texpacks", file);
			using (Stream fs = Platform.FileOpen(path)) {
				ExtractZip(fs, game);
			}
		}
		
		static void ExtractZip(Stream stream, Game game) {
			TexturePack.game = game;
			game.Events.RaiseTexturePackChanged();
			if (game.Graphics.LostContext) return;
			
			ZipReader reader = new ZipReader();
			reader.ProcessZipEntry = ProcessZipEntry;
			reader.Extract(stream);
		}		
		
		static void ProcessZipEntry(string filename, byte[] data, ZipEntry entry) {
			// Ignore directories: convert x/name to name and x\name to name.
			string name = Utils.ToLower(filename);
			int i = name.LastIndexOf('\\');
			if (i >= 0) name = name.Substring(i + 1);
			i = name.LastIndexOf('/');
			if (i >= 0) name = name.Substring(i + 1);
			game.Events.RaiseTextureChanged(name, data);
		}	
		
		static void ExtractTerrainPng(Game game, Stream data, string url) {
			// Must read into a MemoryStream, because stream duration must be lifetime of bitmap
			// and we don't want to maintain a reference to the file
			MemoryStream ms = ReadAllBytes(data);
			Bitmap bmp = GetBitmap(game.Drawer2D, ms);
			
			if (bmp != null) {
				game.World.TextureUrl = url;
				game.Events.RaiseTexturePackChanged();
				if (game.ChangeTerrainAtlas(bmp)) return;
			}
			
			if (bmp != null) bmp.Dispose();
			ms.Dispose();
		}
		
		
		internal static void ExtractTerrainPng(Game game, Request item) {
			if (item.Data == null) return;
			game.World.TextureUrl = item.Url;
			game.Events.RaiseTexturePackChanged();
			
			Bitmap bmp = (Bitmap)item.Data;
			TextureCache.Add(item.Url, bmp);
			TextureCache.AddETag(item.Url, item.ETag);
			TextureCache.AdddLastModified(item.Url, item.LastModified);
			
			if (!game.ChangeTerrainAtlas(bmp)) bmp.Dispose();
		}
		
		internal static void ExtractTexturePack(Game game, Request item) {
			if (item.Data == null) return;
			game.World.TextureUrl = item.Url;
			byte[] data = (byte[])item.Data;
			
			TextureCache.Add(item.Url, data);
			TextureCache.AddETag(item.Url, item.ETag);
			TextureCache.AdddLastModified(item.Url, item.LastModified);
			
			using (Stream ms = new MemoryStream(data)) {
				ExtractZip(ms, game);
			}
		}

		static Bitmap GetBitmap(IDrawer2D drawer, Stream src) {
			try {
				return Platform.ReadBmp32Bpp(drawer, src);
			} catch (ArgumentException ex) {
				ErrorHandler.LogError("Cache.GetBitmap", ex);
				return null;
			} catch (IOException ex) {
				ErrorHandler.LogError("Cache.GetBitmap", ex);
				return null;
			}
		}
		
		static MemoryStream ReadAllBytes(Stream src) {
			MemoryStream dst = new MemoryStream((int)src.Length);
			byte[] buffer = new byte[4096];
			for (int i = 0; i < (int)src.Length; i += 4096) {
				int count = Math.Min(4096, (int)src.Length - i);
				src.Read(buffer, 0, count);
				dst.Write(buffer, 0, count);
			}
			return dst;
		}
	}
}
