// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {
	
	/// <summary> Represents a 2D packed texture atlas, specifically for terrain.png. </summary>
	public class TerrainAtlas2D : IDisposable {
		
		public const int TilesPerRow = 16, RowsCount = 16;
		public Bitmap AtlasBitmap;
		public int TileSize;
		
		Game game;		
		public TerrainAtlas2D(Game game) { this.game = game; }
		
		/// <summary> Updates the underlying atlas bitmap, fields, and texture. </summary>
		public void UpdateState(Bitmap bmp) {
			AtlasBitmap = bmp;
			TileSize = bmp.Width / TilesPerRow;
			using (FastBitmap fastBmp = new FastBitmap(bmp, true, true))
				BlockInfo.RecalculateSpriteBB(fastBmp);
		}
		
		/// <summary> Creates a new texture that contains the tile at the specified index. </summary>
		public int LoadTextureElement(int index) {
			int size = TileSize;
			using (FastBitmap atlas = new FastBitmap(AtlasBitmap, true, true))
				using (Bitmap bmp = Platform.CreateBmp(size, size))
					using (FastBitmap dst = new FastBitmap(bmp, true, false))
			{
				int x = index % TilesPerRow, y = index / TilesPerRow;
				FastBitmap.MovePortion(x * size, y * size, 0, 0, atlas, dst, size);
				return game.Graphics.CreateTexture(dst, false, game.Graphics.Mipmaps);
			}
		}
		
		/// <summary> Disposes of the underlying atlas bitmap and texture. </summary>
		public void Dispose() {
			if (AtlasBitmap != null)
				AtlasBitmap.Dispose();
		}
	}
}