// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting through a simple heightmap, where each block is either in sun or shadow. </summary>
	public sealed partial class BasicLighting : IWorldLighting {
		
		int width, height, length, oneY;
		BlockInfo info;
		Game game;
		
		public override void Dispose() { heightmap = null; }
		public override void Reset(Game game) { heightmap = null; }
		public override void OnNewMap(Game game) { heightmap = null; }
		
		public override void OnNewMapLoaded(Game game) {
			width = game.World.Width;
			height = game.World.Height;
			length = game.World.Length;
			info = game.BlockInfo;
			this.game = game;
			oneY = width * length;
			
			heightmap = new short[width * length];
			for (int i = 0; i < heightmap.Length; i++)
				heightmap[i] = short.MaxValue;
		}
		
		public unsafe override void LightHint(int startX, int startZ, byte* mapPtr) {
			int x1 = Math.Max(startX, 0), x2 = Math.Min(width, startX + 18);
			int z1 = Math.Max(startZ, 0), z2 = Math.Min(length, startZ + 18);
			int xCount = x2 - x1, zCount = z2 - z1;
			int* skip = stackalloc int[xCount * zCount];
			
			int elemsLeft = InitialHeightmapCoverage(x1, z1, xCount, zCount, skip);
			if (!CalculateHeightmapCoverage(x1, z1, xCount, zCount, elemsLeft, skip, mapPtr)) {
				FinishHeightmapCoverage(x1, z1, xCount, zCount, skip);
			}
		}
		
		public override int GetLightHeight(int x, int z) {
			int index = (z * width) + x;
			int lightH = heightmap[index];
			return lightH == short.MaxValue ? CalcHeightAt(x, height - 1, z, index) : lightH;
		}

		public override bool IsLit(int x, int y, int z) {
			if (x < 0 || y < 0 || z < 0 || 
			    x >= width || y >= height || z >= length) return true;
			return y > GetLightHeight(x, z);
		}

		public override bool IsLitNoCheck(int x, int y, int z) {
			return y > GetLightHeight(x, z);
		}
		
		
		public override void UpdateLight(int x, int y, int z, byte oldBlock, byte newBlock) {
			bool didBlock = info.BlocksLight[oldBlock];
			bool nowBlocks = info.BlocksLight[newBlock];
			if (didBlock == nowBlocks) return;
			int oldOffset = (info.LightOffset[oldBlock] >> Side.Top) & 1;
			int newOffset = (info.LightOffset[newBlock] >> Side.Top) & 1;
			
			int index = (z * width) + x;
			int lightH = heightmap[index];
			if (lightH == short.MaxValue) {
				// We have to calculate the entire column for visibility, because the old/new block info is
				// useless if there is another block higher than block.y that blocks sunlight.
				CalcHeightAt(x, height - 1, z, index);
			} else if ((y - newOffset) >= lightH) {
				if (nowBlocks) {
					heightmap[index] = (short)(y - newOffset);
				} else {
					// Part of the column is now visible to light, we don't know how exactly how high it should be though.
					// However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y
					CalcHeightAt(x, y, z, index);
				}
			} else if (y == lightH && oldOffset == 0) {
				// For a solid block on top of an upside down slab, they will both have the same light height.
				// So we need to account for this particular case.
				byte above = y == (height - 1) ? Block.Air : game.World.GetBlock(x, y + 1, z);
				if (info.BlocksLight[above]) return;
				
				if (nowBlocks)
					heightmap[index] = (short)(y - newOffset);
				else
					CalcHeightAt(x, y - 1, z, index);
			}
		}
	}
}
