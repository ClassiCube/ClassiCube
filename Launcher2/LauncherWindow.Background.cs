// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.Textures;
using Launcher.Drawing;

namespace Launcher {

	public sealed partial class LauncherWindow {

		internal bool ClassicBackground = false;
		bool fontPng, terrainPng;
		
		internal void TryLoadTexturePack() {
			fontPng = false; terrainPng = false;
			Options.Load();
			LauncherSkin.LoadFromOptions();
			if (Options.Get("nostalgia-classicbg") != null)
				ClassicBackground = Options.GetBool("nostalgia-classicbg", false);
			else
				ClassicBackground = Options.GetBool("mode-classic", false);
			
			string texDir = Path.Combine(Program.AppDirectory, "texpacks");
			string texPack = Options.Get(OptionsKey.DefaultTexturePack) ?? "default.zip";
			texPack = Path.Combine(texDir, texPack);
			
			if (!File.Exists(texPack))
				texPack = Path.Combine(texDir, "default.zip");
			if (!File.Exists(texPack)) return;
			
			ExtractTexturePack(texPack);
			if (!fontPng || !terrainPng) {
				texPack = Path.Combine(texDir, "default.zip");
				ExtractTexturePack(texPack);
			}
		}
		
		void ExtractTexturePack(string texPack) {
			using (Stream fs = new FileStream(texPack, FileMode.Open, FileAccess.Read, FileShare.Read)) {
				ZipReader reader = new ZipReader();
				reader.SelectZipEntry = SelectZipEntry;
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract(fs);
			}
		}
		
		bool SelectZipEntry(string filename) {
			return filename == "default.png" || filename == "terrain.png";
		}
		
		void ProcessZipEntry(string filename, byte[] data, ZipEntry entry) {
			if (filename == "default.png") {
				if (fontPng) return;
				
				Bitmap bmp = Platform.ReadBmp32Bpp(Drawer, data);
				Drawer.SetFontBitmap(bmp);
				useBitmappedFont = !Options.GetBool(OptionsKey.UseChatFont, false);
				fontPng = true;
			} else if (filename == "terrain.png") {
				if (terrainPng) return;
				
				Bitmap bmp = Platform.ReadBmp32Bpp(Drawer, data);
				MakeClassicTextures(bmp);
				bmp.Dispose();
				terrainPng = true;
			}
		}
		
		bool useBitmappedFont;
		Bitmap terrainBmp;
		FastBitmap terrainPixels;
		const int tileSize = 48;
		
		void MakeClassicTextures(Bitmap bmp) {
			int elemSize = bmp.Width / 16;
			Size size = new Size(tileSize, tileSize);
			terrainBmp = Platform.CreateBmp(tileSize * 2, tileSize);
			terrainPixels = new FastBitmap(terrainBmp, true, false);
			
			// Precompute the scaled background
			using (FastBitmap src = new FastBitmap(bmp, true, true)) {
				BitmapDrawer.DrawScaled(src, terrainPixels, size,
				                        new Rectangle(2 * elemSize, 0, elemSize, elemSize),
				                        new Rectangle(tileSize, 0, tileSize, tileSize), 128, 64);
				BitmapDrawer.DrawScaled(src, terrainPixels, size,
				                        new Rectangle(1 * elemSize, 0, elemSize, elemSize),
				                        new Rectangle(0, 0, tileSize, tileSize), 96, 96);
			}
		}
		
		public void RedrawBackground() {
			if (Framebuffer == null || (Framebuffer.Width != Width || Framebuffer.Height != Height)) {
				if (Framebuffer != null)
					Framebuffer.Dispose();
				Framebuffer = platformDrawer.CreateFrameBuffer(Width, Height);
			}
			
			if (ClassicBackground && terrainPixels != null) {
				using (FastBitmap bmp = LockBits()) {
					ClearTile(0, 0, Width, 48, tileSize, bmp);
					ClearTile(0, 48, Width, Height - 48, 0, bmp);
				}
			} else {
				ResetArea(0, 0, Width, Height);
			}
			
			DrawTitle();
			Dirty = true;
		}
		
		void DrawTitle() {
			using (IDrawer2D drawer = Drawer) {
				drawer.SetBitmap(Framebuffer);

				drawer.UseBitmappedChat = (useBitmappedFont || ClassicBackground) && fontPng;
				DrawTextArgs args = new DrawTextArgs("&eClassical&fSharp", logoFont, false);
				Size size = drawer.MeasureSize(ref args);
				int xStart = Width / 2 - size.Width / 2;
				
				args.Text = "&0Classical&0Sharp";
				drawer.DrawText(ref args, xStart + 4, 4);
				args.Text = "&eClassical&fSharp";
				drawer.DrawText(ref args, xStart, 0);
				drawer.UseBitmappedChat = false;
			}
		}
		
		/// <summary> Redraws the specified region with the background pixels. </summary>
		public void ResetArea(int x, int y, int width, int height) {
			using (FastBitmap dst = LockBits())
				ResetArea(x, y, width, height, dst);
		}
		
		/// <summary> Redraws the specified region with the background pixels. </summary>
		public void ResetArea(int x, int y, int width, int height, FastBitmap dst) {
			if (ClassicBackground && terrainPixels != null) {
				ClearTile(x, y, width, height, 0, dst);
			} else {
				FastColour col = LauncherSkin.BackgroundCol;
				Gradient.Noise(dst, new Rectangle(x, y, width, height), col, 6);
			}
		}
		
		void ClearTile(int x, int y, int width, int height, int srcX, FastBitmap dst) {
			Rectangle srcRect = new Rectangle(srcX, 0, tileSize, tileSize);
			Rectangle dstRect = new Rectangle(x, y, width, height);
			BitmapDrawer.DrawTiled(terrainPixels, dst, srcRect, dstRect);
		}
	}
}
