// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Textures;

namespace Launcher.Patcher {
	
	public class ResourcePatcher {

		public ResourcePatcher(ResourceFetcher fetcher, IDrawer2D drawer) {
			jarClassic = fetcher.jarClassic;
			jar162 = fetcher.jar162;
			pngTerrainPatch = fetcher.pngTerrainPatch;
			pngGuiPatch = fetcher.pngGuiPatch;
			this.drawer = drawer;
		}
		ZipReader reader;
		ZipWriter writer;
		IDrawer2D drawer;
		
		Bitmap animBitmap;
		List<string> existing = new List<string>();
		
		byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public void Run() {
			reader = new ZipReader();
			reader.SelectZipEntry = SelectZipEntry_Classic;
			reader.ProcessZipEntry = ProcessZipEntry_Classic;
			string texDir = Path.Combine(Program.AppDirectory, "texpacks");
			string path = Path.Combine(texDir, "default.zip");
			ExtractExisting(path);
			
			using (Stream dst = new FileStream(path, FileMode.Create, FileAccess.Write)) {
				writer = new ZipWriter(dst);
				writer.entries = new ZipEntry[100];
				for (int i = 0; i < entries.Count; i++)
					writer.WriteZipEntry(entries[i], datas[i]);
				
				ExtractClassic();
				ExtractModern();
				if (pngGuiPatch != null) {
					using (Bitmap gui = Platform.ReadBmp32Bpp(drawer, pngGuiPatch))
						writer.WriteNewImage(gui, "gui.png");
				}
				writer.WriteCentralDirectoryRecords();
			}
		}
		
		
		#region From default.zip
		
		List<ZipEntry> entries = new List<ZipEntry>();
		List<byte[]> datas = new List<byte[]>();
		void ExtractExisting(string path) {
			if (!File.Exists(path)) return;
			
			using (Stream src = new FileStream(path, FileMode.Open, FileAccess.Read)) {
				reader.ProcessZipEntry = ExtractExisting;
				reader.Extract(src);
			}
		}
		
		void ExtractExisting(string filename, byte[] data, ZipEntry entry) {
			filename = ResourceList.GetFile(filename);
			entry.Filename = filename;
			existing.Add(filename);
			entries.Add(entry);
			datas.Add(data);
		}
		
		#endregion
		
		#region From classic
		
		void ExtractClassic() {
			if (jarClassic == null) return;
			using (Stream src = new MemoryStream(jarClassic)) {
				reader.SelectZipEntry = SelectZipEntry_Classic;
				reader.ProcessZipEntry = ProcessZipEntry_Classic;
				reader.Extract(src);
			}
		}
		
		bool SelectZipEntry_Classic(string filename) {
			return filename.StartsWith("gui")
				|| filename.StartsWith("mob") || filename.IndexOf('/') < 0;
		}
		
		StringComparison comp = StringComparison.OrdinalIgnoreCase;
		void ProcessZipEntry_Classic(string filename, byte[] data, ZipEntry entry) {
			if (!filename.EndsWith(".png", comp)) return;
			entry.Filename = ResourceList.GetFile(filename);
			
			if (entry.Filename != "terrain.png") {
				if (entry.Filename == "gui.png")
					entry.Filename = "gui_classic.png";
				if (!existing.Contains(entry.Filename))
					writer.WriteZipEntry(entry, data);
				return;
			} else if (!existing.Contains("terrain.png")){
				using (Bitmap dst = Platform.ReadBmp32Bpp(drawer, data),
				      mask = Platform.ReadBmp32Bpp(drawer, pngTerrainPatch)) {
					PatchImage(dst, mask);
					writer.WriteNewImage(dst, "terrain.png");
				}
			}
		}
		
		#endregion
		
		#region From Modern
		
		void ExtractModern() {
			if (jar162 == null) return;
			
			using (Stream src = new MemoryStream(jar162)) {
				// Grab animations and snow
				animBitmap = Platform.CreateBmp(1024, 64);
				reader.SelectZipEntry = SelectZipEntry_Modern;
				reader.ProcessZipEntry = ProcessZipEntry_Modern;
				reader.Extract(src);
				
				if (!existing.Contains("animations.png"))
					writer.WriteNewImage(animBitmap, "animations.png");
				if (!existing.Contains("animations.txt"))
					writer.WriteNewString(animationsTxt, "animations.txt");
				animBitmap.Dispose();
			}
		}
		
		bool SelectZipEntry_Modern(string filename) {
			return filename.StartsWith("assets/minecraft/textures") &&
				(filename == "assets/minecraft/textures/environment/snow.png" ||
				 filename == "assets/minecraft/textures/blocks/water_still.png" ||
				 filename == "assets/minecraft/textures/blocks/lava_still.png" ||
				 filename == "assets/minecraft/textures/blocks/fire_layer_1.png" ||
				 filename == "assets/minecraft/textures/entity/chicken.png");
		}
		
		void ProcessZipEntry_Modern(string filename, byte[] data, ZipEntry entry) {
			entry.Filename = ResourceList.GetFile(filename);
			switch(filename) {
				case "assets/minecraft/textures/environment/snow.png":
					if (!existing.Contains("snow.png"))
						writer.WriteZipEntry(entry, data);
					break;
				case "assets/minecraft/textures/entity/chicken.png":
					if (!existing.Contains("chicken.png"))
						writer.WriteZipEntry(entry, data);
					break;
				case "assets/minecraft/textures/blocks/water_still.png":
					PatchDefault(data, 0);
					break;
				case "assets/minecraft/textures/blocks/lava_still.png":
					PatchCycle(data, 16);
					break;
				case "assets/minecraft/textures/blocks/fire_layer_1.png":
					PatchDefault(data, 32);
					break;
			}
		}
		
		#endregion
		
		unsafe void PatchImage(Bitmap dstBitmap, Bitmap maskBitmap) {
			using (FastBitmap dst = new FastBitmap(dstBitmap, true, false),
			      src = new FastBitmap(maskBitmap, true, true)) {
				int size = src.Width, tileSize = size / 16;
				
				for (int y = 0; y < size; y += tileSize) {
					int* row = src.GetRowPtr(y);
					for (int x = 0; x < size; x += tileSize) {
						if (row[x] != unchecked((int)0x80000000)) {
							FastBitmap.MovePortion(x, y, x, y, src, dst, tileSize);
						}
					}
				}
			}
		}
		
		void CopyTile(int src, int dst, int y, Bitmap bmp) {
			for (int yy = 0; yy < 16; yy++) {
				for (int xx = 0; xx < 16; xx++) {
					animBitmap.SetPixel(dst + xx, y + yy,
					                    bmp.GetPixel(xx, src + yy));
				}
			}
		}
		
		
		const string animationsTxt = @"# This file defines the animations used in a texture pack for ClassicalSharp and other supporting applications.
# Each line is in the format: <TileX> <TileY> <FrameX> <FrameY> <Frame size> <Frames count> <Tick delay>
# - TileX and TileY indicate the coordinates of the tile in terrain.png that 
#     will be replaced by the animation frames. These range from 0 to 15. (inclusive of 15)
# - FrameX and FrameY indicates the pixel coordinates of the first animation frame in animations.png.
# - Frame Size indicates the size in pixels of an animation frame.
# - Frames count indicates the number of used frames.  The first frame is located at 
#     (FrameX, FrameY), the second one at (FrameX + FrameSize, FrameY) and so on.
# - Tick delay is the number of ticks a frame doesn't change. For instance, a value of 0
#     means that the frame would be changed every tick, while a value of 2 would mean 
#    'replace with frame 1, don't change frame, don't change frame, replace with frame 2'.
# NOTE: If a file called 'uselavaanim' is in the texture pack,  ClassicalSharp 0.99.2 onwards uses its built-in dynamic generation for the lava texture animation.
# NOTE: If a file called 'usewateranim' is in the texture pack, ClassicalSharp 0.99.5 onwards uses its built-in dynamic generation for the water texture animation.

# still water
14 0 0 0 16 32 2
# still lava
14 1 0 16 16 39 2
# fire
6 2 0 32 16 32 0";
		
		unsafe void PatchDefault(byte[] data, int y) {
			// Sadly files in modern are 24 rgb, so we can't use fastbitmap here
			using (Bitmap bmp = Platform.ReadBmp32Bpp(drawer, data)) {
				for (int tile = 0; tile < bmp.Height; tile += 16) {
					CopyTile(tile, tile, y, bmp);
				}
			}
		}
		
		unsafe void PatchCycle(byte[] data, int y) {
			using (Bitmap bmp = Platform.ReadBmp32Bpp(drawer, data)) {
				int dst = 0;
				for (int tile = 0; tile < bmp.Height; tile += 16, dst += 16) {
					CopyTile(tile, dst, y, bmp);
				}
				// Cycle back to first frame.
				for (int tile = bmp.Height - 32; tile >= 0; tile -= 16, dst += 16) {
					CopyTile(tile, dst, y, bmp);
				}
			}
		}
	}
}
