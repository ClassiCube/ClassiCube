// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Textures {
	
	/// <summary> Represents a 2D packed texture atlas, specifically for terrain.png. </summary>
	public static class Atlas2D {		
		public const int TilesPerRow = 16, MaxRowsCount = 32;
		public static Bitmap Atlas;
		public static int TileSize, RowsCount;
		internal static Game game;

		public static void UpdateState(Bitmap bmp) {
			Atlas = bmp;
			TileSize = bmp.Width / TilesPerRow;
			RowsCount = bmp.Height / TilesPerRow;
			BlockInfo.RecalculateSpriteBB();
		}

		public static int LoadTile(int texLoc) {
			int size = TileSize;
			using (FastBitmap atlas = new FastBitmap(Atlas, true, true))
				using (Bitmap bmp = Platform.CreateBmp(size, size))
					using (FastBitmap dst = new FastBitmap(bmp, true, false))
			{
				int x = texLoc % TilesPerRow, y = texLoc / TilesPerRow;
				y %= RowsCount;
				
				FastBitmap.MovePortion(x * size, y * size, 0, 0, atlas, dst, size);
				return game.Graphics.CreateTexture(dst, false, game.Graphics.Mipmaps);
			}
		}

		public static void Dispose() {
			if (Atlas != null) Atlas.Dispose();
		}
	}
	
	/// <summary> Represents a 2D packed texture atlas that has been converted into an array of 1D atlases. </summary>
	public static class Atlas1D {
		public static int TilesPerAtlas, AtlasesCount;
		public static float invTileSize;		
		internal static Game game;
		public const int MaxAtlases = Atlas2D.MaxRowsCount * Atlas2D.TilesPerRow;
		public static int[] TexIds = new int[MaxAtlases];
		
		public static TextureRec GetTexRec(int texLoc, int uCount, out int index) {
			index = texLoc / TilesPerAtlas;
			int y = texLoc % TilesPerAtlas;
			// Adjust coords to be slightly inside - fixes issues with AMD/ATI cards.
			return new TextureRec(0, y * invTileSize, (uCount - 1) + 15.99f/16f, (15.99f/16f) * invTileSize);
		}
		
		/// <summary> Returns the index of the 1D texture within the array of 1D textures
		/// containing the given texture id. </summary>
		public static int Get1DIndex(int texLoc) { return texLoc / TilesPerAtlas; }
		
		/// <summary> Returns the index of the given texture id within a 1D texture. </summary>
		public static int Get1DRowId(int texLoc) { return texLoc % TilesPerAtlas; }
		
		public static void UpdateState() {
			int tileSize = Atlas2D.TileSize;
			int maxTiles = Atlas2D.RowsCount * Atlas2D.TilesPerRow;
			int maxAtlasHeight = Math.Min(4096, game.Graphics.MaxTextureDimensions);
			int maxTilesPerAtlas = maxAtlasHeight / tileSize;
			
			TilesPerAtlas = Math.Min(maxTilesPerAtlas, maxTiles);
			AtlasesCount = Utils.CeilDiv(maxTiles, TilesPerAtlas);
			invTileSize = 1f / TilesPerAtlas;
			
			Utils.LogDebug("Loaded new atlas: {0} bmps, {1} per bmp", AtlasesCount, TilesPerAtlas);
			int index = 0, atlasHeight = TilesPerAtlas * tileSize;
			
			using (FastBitmap atlas = new FastBitmap(Atlas2D.Atlas, true, true)) {
				for (int i = 0; i < AtlasesCount; i++) {
					Make1DTexture(i, atlas, atlasHeight, ref index);
				}
			}
		}
		
		static void Make1DTexture(int i, FastBitmap atlas, int atlas1DHeight, ref int index) {
			int tileSize = Atlas2D.TileSize;
			using (Bitmap atlas1d = Platform.CreateBmp(tileSize, atlas1DHeight))
				using (FastBitmap dst = new FastBitmap(atlas1d, true, false))
			{
				for (int index1D = 0; index1D < TilesPerAtlas; index1D++) {
					int atlasX = (index % Atlas2D.TilesPerRow) * tileSize;
					int atlasY = (index / Atlas2D.TilesPerRow) * tileSize;
					
					FastBitmap.MovePortion(atlasX, atlasY,
					                       0, index1D * tileSize, atlas, dst, tileSize);
					index++;
				}
				TexIds[i] = game.Graphics.CreateTexture(dst, true, game.Graphics.Mipmaps);
			}
		}
		
		public static int UsedAtlasesCount() {
			int maxTexLoc = 0;
			for (int i = 0; i < BlockInfo.textures.Length; i++) {
				maxTexLoc = Math.Max(maxTexLoc, BlockInfo.textures[i]);
			}			
			return Get1DIndex(maxTexLoc) + 1;
		}
		
		public static void Dispose() {
			if (TexIds == null) return;
			
			for (int i = 0; i < AtlasesCount; i++) {
				game.Graphics.DeleteTexture(ref TexIds[i]);
			}
		}
	}
}