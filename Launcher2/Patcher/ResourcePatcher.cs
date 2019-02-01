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
		
		Bitmap animsBmp;
		List<string> filenames = new List<string>();
		Bitmap terrainBmp;
		bool patchedTerrain;
		
		byte[] jarClassic, jar162, pngTerrainPatch, pngGuiPatch;
		public void Run() {
			reader = new ZipReader();
			reader.SelectZipEntry = SelectZipEntry_Classic;
			reader.ProcessZipEntry = ProcessZipEntry_Classic;
			string defPath = Path.Combine("texpacks", "default.zip");
			
			if (Platform.FileExists(defPath)) {			
				using (Stream src = Platform.FileOpen(defPath)) {
					reader.ProcessZipEntry = ExtractExisting;
					reader.Extract(src);
				}
			}
			
			using (Stream dst = Platform.FileCreate(defPath)) {
				writer = new ZipWriter(dst);
				writer.entries = new ZipEntry[100];
				for (int i = 0; i < entries.Count; i++)
					writer.WriteZipEntry(entries[i], datas[i]);
				
				ExtractClassic();
				ExtractModern();
				if (pngGuiPatch != null) {
					using (Bitmap gui = Platform.ReadBmp(drawer, pngGuiPatch))
						writer.WriteNewImage(gui, "gui.png");
				}
				if (patchedTerrain) {
					writer.WriteNewImage(terrainBmp, "terrain.png");
				}
				writer.WriteCentralDirectoryRecords();
			}
		}
		
		
		#region From default.zip
		
		List<ZipEntry> entries = new List<ZipEntry>();
		List<byte[]> datas = new List<byte[]>();
		
		void ExtractExisting(string path, byte[] data, ZipEntry entry) {
			entry.Path = Utils.GetFilename(path);
			filenames.Add(entry.Path);
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
		
		bool SelectZipEntry_Classic(string path) {
			return 
				Utils.CaselessStarts(path, "gui") ||
				Utils.CaselessStarts(path, "mob") || 
				path.IndexOf('/') < 0;
		}

		void ProcessZipEntry_Classic(string path, byte[] data, ZipEntry entry) {
			if (!Utils.CaselessEnds(path, ".png")) return;
			entry.Path = Utils.GetFilename(path);
			
			if (entry.Path != "terrain.png") {
				if (entry.Path == "gui.png")
					entry.Path = "gui_classic.png";
				
				if (!filenames.Contains(entry.Path))
					writer.WriteZipEntry(entry, data);
			} else if (!filenames.Contains("terrain.png")){
				terrainBmp = Platform.ReadBmp(drawer, data);
				using (Bitmap mask = Platform.ReadBmp(drawer, pngTerrainPatch)) {
					CopyTile( 0,  0,  3 * 16, 3 * 16, mask, terrainBmp);
					CopyTile(16,  0,  6 * 16, 3 * 16, mask, terrainBmp);
					CopyTile(32,  0,  6 * 16, 2 * 16, mask, terrainBmp);
					
					CopyTile( 0, 16,  5 * 16, 3 * 16, mask, terrainBmp);
					CopyTile(16, 16,  6 * 16, 5 * 16, mask, terrainBmp);
					CopyTile(32, 16, 11 * 16, 0 * 16, mask, terrainBmp);
					patchedTerrain = true;
				}
			}
		}
		
		#endregion
		
		#region From Modern
		
		void ExtractModern() {
			if (jar162 == null) return;
			
			using (Stream src = new MemoryStream(jar162)) {
				// Grab animations and snow
				animsBmp = Platform.CreateBmp(512, 16);
				reader.SelectZipEntry = SelectZipEntry_Modern;
				reader.ProcessZipEntry = ProcessZipEntry_Modern;
				reader.Extract(src);
				
				if (!filenames.Contains("animations.png"))
					writer.WriteNewImage(animsBmp, "animations.png");
				if (!filenames.Contains("animations.txt"))
					writer.WriteNewString(animationsTxt, "animations.txt");
				animsBmp.Dispose();
			}
		}
		
		bool SelectZipEntry_Modern(string path) {
			return path.StartsWith("assets/minecraft/textures") &&
				(path == "assets/minecraft/textures/environment/snow.png"              ||
				 path == "assets/minecraft/textures/entity/chicken.png"                ||
				 path == "assets/minecraft/textures/blocks/fire_layer_1.png"           ||
				 path == "assets/minecraft/textures/blocks/sandstone_bottom.png"       ||
				 path == "assets/minecraft/textures/blocks/sandstone_normal.png"       ||
				 path == "assets/minecraft/textures/blocks/sandstone_top.png"          ||
				 path == "assets/minecraft/textures/blocks/quartz_block_lines_top.png" ||
				 path == "assets/minecraft/textures/blocks/quartz_block_lines.png"     ||
				 path == "assets/minecraft/textures/blocks/stonebrick.png"             ||
				 path == "assets/minecraft/textures/blocks/snow.png"                   ||
				 path == "assets/minecraft/textures/blocks/wool_colored_blue.png"      ||
				 path == "assets/minecraft/textures/blocks/wool_colored_brown.png"     ||
				 path == "assets/minecraft/textures/blocks/wool_colored_cyan.png"      ||
				 path == "assets/minecraft/textures/blocks/wool_colored_green.png"     ||
				 path == "assets/minecraft/textures/blocks/wool_colored_pink.png"
				);
		}
		
		void ProcessZipEntry_Modern(string path, byte[] data, ZipEntry entry) {
			entry.Path = Utils.GetFilename(path);
			if (path == "assets/minecraft/textures/environment/snow.png") {
				if (!filenames.Contains("snow.png")) {
					writer.WriteZipEntry(entry, data);
				}
			} else if (path == "assets/minecraft/textures/entity/chicken.png") {
				if (!filenames.Contains("chicken.png")) {
					writer.WriteZipEntry(entry, data);
				}
			} else if (path == "assets/minecraft/textures/blocks/fire_layer_1.png") {
				PatchAnim(data);
			} else if (path == "assets/minecraft/textures/blocks/sandstone_bottom.png") {
				PatchBlock(data, 9, 3);
			} else if (path == "assets/minecraft/textures/blocks/sandstone_normal.png") {
				PatchBlock(data, 9, 2);
			} else if (path == "assets/minecraft/textures/blocks/sandstone_top.png") {
				PatchBlock(data, 9, 1);
			} else if (path == "assets/minecraft/textures/blocks/quartz_block_lines_top.png") {
				PatchBlock(data, 10, 3);
				PatchBlock(data, 10, 1);
			} else if (path == "assets/minecraft/textures/blocks/quartz_block_lines.png") {
				PatchBlock(data, 10, 2);
			} else if (path == "assets/minecraft/textures/blocks/stonebrick.png") {
				PatchBlock(data, 4, 3);
			} else if (path == "assets/minecraft/textures/blocks/snow.png") {
				PatchBlock(data, 2, 3);
			} else if (path == "assets/minecraft/textures/blocks/wool_colored_blue.png") {
				PatchBlock(data, 3, 5);
			} else if (path == "assets/minecraft/textures/blocks/wool_colored_brown.png") {
				PatchBlock(data, 2, 5);
			} else if (path == "assets/minecraft/textures/blocks/wool_colored_cyan.png") {
				PatchBlock(data, 4, 5);
			} else if (path == "assets/minecraft/textures/blocks/wool_colored_green.png") {
				PatchBlock(data, 1, 5);
			} else if (path == "assets/minecraft/textures/blocks/wool_colored_pink.png") {
				PatchBlock(data, 0, 5);
			}
		}
		
		#endregion
		
		void CopyTile(int srcX, int srcY, int dstX, int dstY, Bitmap src, Bitmap dst) {
			for (int yy = 0; yy < 16; yy++) {
				for (int xx = 0; xx < 16; xx++) {
					dst.SetPixel(dstX + xx, dstY + yy, src.GetPixel(srcX + xx, srcY + yy));
				}
			}
		}
		
		
		const string animationsTxt = @"# This file defines the animations used in a texture pack for ClassicalSharp and other supporting applications.
# Each line is in the format: <TileX> <TileY> <FrameX> <FrameY> <Frame size> <Frames count> <Tick delay>
# - TileX and TileY are the coordinates of the tile in terrain.png that will be replaced by the animation frames.
#     Essentially, TileX and TileY are the remainder and quotient of an ID in F10 menu divided by 16
#     For instance, obsidian texture (37) has TileX of 5, and TileY of 2
# - FrameX and FrameY are the pixel coordinates of the first animation frame in animations.png.
# - Frame Size is the size in pixels of an animation frame.
# - Frames count is the number of used frames.  The first frame is located at
#     (FrameX, FrameY), the second one at (FrameX + FrameSize, FrameY) and so on.
# - Tick delay is the number of ticks a frame doesn't change. For instance, delay of 0
#     means that the tile would be replaced every tick, while delay of 2 means
#     'replace with frame 1, don't change frame, don't change frame, replace with frame 2'.
# NOTE: If a file called 'uselavaanim' is in the texture pack,  ClassicalSharp 0.99.2 onwards uses its built-in dynamic generation for the lava texture animation.
# NOTE: If a file called 'usewateranim' is in the texture pack, ClassicalSharp 0.99.5 onwards uses its built-in dynamic generation for the water texture animation.

# fire
6 2 0 0 16 32 0";
		
		void PatchBlock(byte[] data, int x, int y) {
			using (Bitmap bmp = Platform.ReadBmp(drawer, data)) {
				CopyTile(0, 0, x * 16, y * 16, bmp, terrainBmp);
			}
			patchedTerrain = true;
		}
		
		void PatchAnim(byte[] data) {
			using (Bitmap bmp = Platform.ReadBmp(drawer, data)) {
				for (int i = 0; i < bmp.Height; i += 16) {
					CopyTile(0, i, i, 0, bmp, animsBmp);
				}
			}
		}
	}
}
