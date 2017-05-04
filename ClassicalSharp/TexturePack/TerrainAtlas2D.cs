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
		
		/// <summary> Updates the underlying atlas bitmap, fields, and texture. </summary>
		public void UpdateState(BlockInfo info, Bitmap bmp) {
			AtlasBitmap = bmp;
			TileSize = bmp.Width / TilesPerRow;
			using (FastBitmap fastBmp = new FastBitmap(bmp, true, true))
				info.RecalculateSpriteBB(fastBmp);
		}
		
		/// <summary> Creates a new texture that contains the tile at the specified index. </summary>
		public int LoadTextureElement(IGraphicsApi gfx, int index) {
			int size = TileSize;
			using (FastBitmap atlas = new FastBitmap(AtlasBitmap, true, true))
				using (Bitmap bmp = Platform.CreateBmp(size, size))
					using (FastBitmap dst = new FastBitmap(bmp, true, false))
			{
				int x = index % TilesPerRow, y = index / TilesPerRow;
				FastBitmap.MovePortion(x * size, y * size, 0, 0, atlas, dst, size);
				return gfx.CreateTexture(dst, true);
			}
		}
		
		/// <summary> Disposes of the underlying atlas bitmap and texture. </summary>
		public void Dispose() {
			if (AtlasBitmap != null)
				AtlasBitmap.Dispose();
		}
	}
}