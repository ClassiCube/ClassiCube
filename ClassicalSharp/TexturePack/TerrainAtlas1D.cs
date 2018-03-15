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
		
		public static int elementsPerAtlas1D;
		public static int elementsPerBitmap;
		public static float invElementSize;
		public static int[] TexIds;
		internal static Game game;
		
		public static TextureRec GetTexRec(int texLoc, int uCount, out int index) {
			index = texLoc / elementsPerAtlas1D;
			int y = texLoc % elementsPerAtlas1D;
			// Adjust coords to be slightly inside - fixes issues with AMD/ATI cards.
			return new TextureRec(0, y * invElementSize, (uCount - 1) + 15.99f/16f, (15.99f/16f) * invElementSize);
		}
		
		/// <summary> Returns the index of the 1D texture within the array of 1D textures
		/// containing the given texture id. </summary>
		public static int Get1DIndex(int texLoc) { return texLoc / elementsPerAtlas1D; }
		
		/// <summary> Returns the index of the given texture id within a 1D texture. </summary>
		public static int Get1DRowId(int texLoc) { return texLoc % elementsPerAtlas1D; }
		
		public static void UpdateState() {
			int elemSize = TerrainAtlas2D.ElemSize;
			int maxVerticalSize = Math.Min(4096, game.Graphics.MaxTextureDimensions);
			int elementsPerFullAtlas = maxVerticalSize / elemSize;
			const int totalElements = TerrainAtlas2D.RowsCount * TerrainAtlas2D.TilesPerRow;
			
			int atlasesCount = Utils.CeilDiv(totalElements, elementsPerFullAtlas);
			elementsPerAtlas1D = Math.Min(elementsPerFullAtlas, totalElements);
			int atlas1DHeight = Utils.NextPowerOf2(elementsPerAtlas1D * elemSize);
			
			Convert2DTo1D(atlasesCount, atlas1DHeight);
			elementsPerBitmap = atlas1DHeight / elemSize;
			invElementSize = 1f / elementsPerBitmap;
		}
		
		static void Convert2DTo1D(int atlasesCount, int atlas1DHeight) {
			TexIds = new int[atlasesCount];
			Utils.LogDebug("Loaded new atlas: {0} bmps, {1} per bmp", atlasesCount, elementsPerAtlas1D);
			int index = 0;
			
			using (FastBitmap atlas = new FastBitmap(TerrainAtlas2D.Atlas, true, true)) {
				for (int i = 0; i < TexIds.Length; i++)
					Make1DTexture(i, atlas, atlas1DHeight, ref index);
			}
		}
		
		static void Make1DTexture(int i, FastBitmap atlas, int atlas1DHeight, ref int index) {
			int elemSize = TerrainAtlas2D.ElemSize;
			using (Bitmap atlas1d = Platform.CreateBmp(elemSize, atlas1DHeight))
				using (FastBitmap dst = new FastBitmap(atlas1d, true, false))
			{
				for (int index1D = 0; index1D < elementsPerAtlas1D; index1D++) {
					FastBitmap.MovePortion((index & 0x0F) * elemSize, (index >> 4) * elemSize,
					                       0, index1D * elemSize, atlas, dst, elemSize);
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