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
		
		public const string Dir = "texpacks";
		Game game;
		public void Extract(string path, Game game) {
			path = PathIO.Combine(Dir, path);
			path = PathIO.Combine(Program.AppDirectory, path);
			using (Stream fs = File.OpenRead(path))
				Extract(fs, game);
		}
		
		public void Extract(Stream stream, Game game) {
			this.game = game;
			game.Events.RaiseTexturePackChanged();
			ZipReader reader = new ZipReader();
			
			reader.ShouldProcessZipEntry = (f) => true;
			reader.ProcessZipEntry = ProcessZipEntry;
			reader.Extract(stream);
		}
		
		public static void ExtractDefault(Game game) {
			TexturePack extractor = new TexturePack();
			extractor.Extract(game.DefaultTexturePack, game);
			game.World.TextureUrl = null;
		}
		
		void ProcessZipEntry(string filename, byte[] data, ZipEntry entry) {
			// Ignore directories: convert x/name to name and x\name to name.
			string name = Utils.ToLower(filename);
			int i = name.LastIndexOf('\\');
			if (i >= 0) name = name.Substring(i + 1, name.Length - 1 - i);
			i = name.LastIndexOf('/');
			if (i >= 0) name = name.Substring(i + 1, name.Length - 1 - i);
			game.Events.RaiseTextureChanged(name, data);
		}
		
		
		internal static void ExtractTerrainPng(Game game, DownloadedItem item) {
			if (item.Data == null) return;
			Bitmap bmp = (Bitmap)item.Data;
			game.World.TextureUrl = item.Url;
			game.Events.RaiseTexturePackChanged();
			
			if (!game.ChangeTerrainAtlas(bmp, null)) { bmp.Dispose(); return; }
			
			TextureCache.Add(item.Url, bmp);
			TextureCache.AddETag(item.Url, item.ETag, game.ETags);
			TextureCache.AdddLastModified(item.Url, item.LastModified, game.LastModified);
		}
		
		internal static void ExtractCachedTerrainPng(Game game, string url) {
			FileStream data = TextureCache.GetStream(url);
			if (data == null) { // e.g. 404 errors
				if (game.World.TextureUrl != null) ExtractDefault(game);
			} else if (url != game.World.TextureUrl) {
				Bitmap bmp = GetBitmap(game.Drawer2D, data);
				if (bmp == null) { data.Dispose(); return; }
				
				game.World.TextureUrl = url;
				game.Events.RaiseTexturePackChanged();
				if (game.ChangeTerrainAtlas(bmp, data)) return;
				
				bmp.Dispose();
				data.Dispose();
			} else {
				data.Dispose();
			}
		}
		
		internal static void ExtractTexturePack(Game game, DownloadedItem item) {
			if (item.Data == null) return;
			game.World.TextureUrl = item.Url;
			byte[] data = (byte[])item.Data;
			
			TexturePack extractor = new TexturePack();
			using (Stream ms = new MemoryStream(data)) {
				extractor.Extract(ms, game);
			}
			
			TextureCache.Add(item.Url, data);
			TextureCache.AddETag(item.Url, item.ETag, game.ETags);
			TextureCache.AdddLastModified(item.Url, item.LastModified, game.LastModified);
		}
		
		internal static void ExtractCachedTexturePack(Game game, string url) {
			using (Stream data = TextureCache.GetStream(url)) {
				if (data == null) { // e.g. 404 errors
					if (game.World.TextureUrl != null) ExtractDefault(game);
				} else if (url != game.World.TextureUrl) {
					game.World.TextureUrl = url;
					TexturePack extractor = new TexturePack();
					extractor.Extract(data, game);
				}
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
	}
}
