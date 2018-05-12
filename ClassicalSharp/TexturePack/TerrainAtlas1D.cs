// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Textures {
	
	/// <summary> Represents a 2D packed texture atlas that has been converted into an array of 1D atlases. </summary>
	public static class TerrainAtlas1D {
		public static int TilesPerAtlas;
		public static float invTileSize;
		public static int[] TexIds;
		internal static Game game;
		
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
			int tileSize = TerrainAtlas2D.TileSize;
			int maxAtlasHeight = Math.Min(4096, game.Graphics.MaxTextureDimensions);
			int maxTilesPerAtlas = maxAtlasHeight / tileSize;
			const int maxTiles = TerrainAtlas2D.RowsCount * TerrainAtlas2D.TilesPerRow;
			
			TilesPerAtlas = Math.Min(maxTilesPerAtlas, maxTiles);
			int atlasesCount = Utils.CeilDiv(maxTiles, TilesPerAtlas);
			int atlasHeight = TilesPerAtlas * tileSize;
			
			invTileSize = 1f / TilesPerAtlas;
			Convert2DTo1D(atlasesCount, atlasHeight);
		}
		
		static void Convert2DTo1D(int atlasesCount, int atlas1DHeight) {
			TexIds = new int[atlasesCount];
			Utils.LogDebug("Loaded new atlas: {0} bmps, {1} per bmp", atlasesCount, TilesPerAtlas);
			int index = 0;
			
			using (FastBitmap atlas = new FastBitmap(TerrainAtlas2D.Atlas, true, true)) {
				for (int i = 0; i < TexIds.Length; i++)
					Make1DTexture(i, atlas, atlas1DHeight, ref index);
			}
		}
		
		static void Make1DTexture(int i, FastBitmap atlas, int atlas1DHeight, ref int index) {
			int tileSize = TerrainAtlas2D.TileSize;
			using (Bitmap atlas1d = Platform.CreateBmp(tileSize, atlas1DHeight))
				using (FastBitmap dst = new FastBitmap(atlas1d, true, false))
			{
				for (int index1D = 0; index1D < TilesPerAtlas; index1D++) {
					int atlasX = (index % TerrainAtlas2D.TilesPerRow) * tileSize;
					int atlasY = (index / TerrainAtlas2D.TilesPerRow) * tileSize;
					
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
			
			for (int i = 0; i < TexIds.Length; i++) {
				game.Graphics.DeleteTexture(ref TexIds[i]);
			}
		}
	}
}