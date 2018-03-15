// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {
	
	/// <summary> Represents a 2D packed texture atlas, specifically for terrain.png. </summary>
	public static class TerrainAtlas2D {
		
		public const int TilesPerRow = 16, RowsCount = 16;
		public static Bitmap Atlas;
		public static int ElemSize;
		internal static Game game;
		
		/// <summary> Updates the underlying atlas bitmap, fields, and texture. </summary>
		public static void UpdateState(Bitmap bmp) {
			Atlas = bmp;
			ElemSize = bmp.Width / TilesPerRow;
			BlockInfo.RecalculateSpriteBB();
		}

		/// <summary> Creates a new texture that contains the tile at the specified index. </summary>
		public static int LoadTextureElement(int index) {
			int size = ElemSize;
			using (FastBitmap atlas = new FastBitmap(Atlas, true, true))
				using (Bitmap bmp = Platform.CreateBmp(size, size))
					using (FastBitmap dst = new FastBitmap(bmp, true, false))
			{
				int x = index % TilesPerRow, y = index / TilesPerRow;
				FastBitmap.MovePortion(x * size, y * size, 0, 0, atlas, dst, size);
				return game.Graphics.CreateTexture(dst, false, game.Graphics.Mipmaps);
			}
		}

		/// <summary> Disposes of the underlying atlas bitmap and texture. </summary>
		public static void Dispose() {
			if (Atlas != null) Atlas.Dispose();
		}
	}
}
