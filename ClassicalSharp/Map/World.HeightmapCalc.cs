// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Map {

	/// <summary> Represents a fixed size map of blocks. Stores the raw block data,
	/// heightmap, dimensions and various metadata such as environment settings. </summary>
	public sealed partial class World {
		
		int CalcHeightAt(int x, int maxY, int z, int index) {
			int mapIndex = (maxY * Length + z) * Width + x;
			for (int y = maxY; y >= 0; y--) {
				byte block = blocks[mapIndex];
				if (info.BlocksLight[block]) {
					int offset = (info.LightOffset[block] >> Side.Top) & 1;
					heightmap[index] = (short)(y - offset);
					return y - offset;
				}
				mapIndex -= oneY;
			}			
			heightmap[index] = -10;
			return -10;
		}
		
		void UpdateHeight(int x, int y, int z, byte oldBlock, byte newBlock) {
			bool didBlock = info.BlocksLight[oldBlock];
			bool nowBlocks = info.BlocksLight[newBlock];
			if (didBlock == nowBlocks) return;
			int oldOffset = (info.LightOffset[oldBlock] >> Side.Top) & 1;
			int newOffset = (info.LightOffset[newBlock] >> Side.Top) & 1;
			
			int index = (z * Width) + x;
			int height = heightmap[index];
			if (height == short.MaxValue) {
				// We have to calculate the entire column for visibility, because the old/new block info is
				// useless if there is another block higher than block.y that blocks sunlight.
				CalcHeightAt(x, maxY, z, index);
			} else if ((y - newOffset >= height)) {
				if (nowBlocks) {
					heightmap[index] = (short)(y - newOffset);
				} else {
					// Part of the column is now visible to light, we don't know how exactly how high it should be though.
					// However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y
					CalcHeightAt(x, y, z, index);
				}
			} else if (y == height && oldOffset == 0) {
				// For a solid block on top of an upside down slab, they will both have the same light height.
				// So we need to account for this particular case.
				byte above = y == maxY ? Block.Air : GetBlock(x, y + 1, z);
				if (info.BlocksLight[above]) return;
				
				if (nowBlocks)
					heightmap[index] = (short)(y - newOffset);
				else
					CalcHeightAt(x, y - 1, z, index);
			}
		}
		
		// Equivalent to
		// for x = startX; x < startX + 18; x++
		//    for z = startZ; z < startZ + 18; z++
		//       CalcHeightAt(x, maxY, z) if height == short.MaxValue
		// Except this function is a lot more optimised and minimises cache misses.
		internal unsafe void HeightmapHint(int startX, int startZ, byte* mapPtr) {
			int x1 = Math.Max(startX, 0), x2 = Math.Min(Width, startX + 18);
			int z1 = Math.Max(startZ, 0), z2 = Math.Min(Length, startZ + 18);
			int xCount = x2 - x1, zCount = z2 - z1;
			int* skip = stackalloc int[xCount * zCount];
			
			int elemsLeft = InitialHeightmapCoverage(x1, z1, xCount, zCount, skip);
			if (!CalculateHeightmapCoverage(x1, z1, xCount, zCount, elemsLeft, skip, mapPtr)) {
				FinishHeightmapCoverage(x1, z1, xCount, zCount, skip);
			}
		}
		
		unsafe int InitialHeightmapCoverage(int x1, int z1, int xCount, int zCount, int* skip) {
			int elemsLeft = 0, index = 0, curRunCount = 0;
			for (int z = 0; z < zCount; z++) {
				int heightmapIndex = (z1 + z) * Width + x1;
				for (int x = 0; x < xCount; x++) {
					int height = heightmap[heightmapIndex++];
					skip[index] = 0;
					if (height == short.MaxValue) {
						elemsLeft++;
						curRunCount = 0;
					} else {
						skip[index - curRunCount]++;
						curRunCount++;
					}
					index++;
				}
				curRunCount = 0; // We can only skip an entire X row at most.
			}
			return elemsLeft;
		}
		
		unsafe bool CalculateHeightmapCoverage(int x1, int z1, int xCount, int zCount, int elemsLeft, int* skip, byte* mapPtr) {
			int prevRunCount = 0;
			for (int y = maxY; y >= 0; y--) {
				if (elemsLeft <= 0) return true;
				int mapIndex = x1 + Width * (z1 + y * Length);
				int heightmapIndex = x1 + z1 * Width;
				
				for (int z = 0; z < zCount; z++) {
					int baseIndex = mapIndex;
					int index = z * xCount;
					for (int x = 0; x < xCount;) {
						int curRunCount = skip[index];
						x += curRunCount; mapIndex += curRunCount; index += curRunCount;
						
						if (x < xCount && info.BlocksLight[mapPtr[mapIndex]]) {
							int lightOffset = (info.LightOffset[mapPtr[mapIndex]] >> Side.Top) & 1;
							heightmap[heightmapIndex + x] = (short)(y - lightOffset);
							elemsLeft--;
							skip[index] = 0;
							int offset = prevRunCount + curRunCount;
							int newRunCount = skip[index - offset] + 1;
							
							// consider case 1 0 1 0, where we are at 0
							// we need to make this 3 0 0 0 and advance by 1
							int oldRunCount = (x - offset + newRunCount) < xCount ? skip[index - offset + newRunCount] : 0;
							if (oldRunCount != 0) {
								skip[index - offset + newRunCount] = 0;
								newRunCount += oldRunCount;
							}
							skip[index - offset] = newRunCount;
							x += oldRunCount; index += oldRunCount; mapIndex += oldRunCount;
							prevRunCount = newRunCount;
						} else {
							prevRunCount = 0;
						}
						x++; mapIndex++; index++;
					}
					prevRunCount = 0;
					heightmapIndex += Width;
					mapIndex = baseIndex + Width; // advance one Z
				}
			}
			return false;
		}
		
		unsafe void FinishHeightmapCoverage(int x1, int z1, int xCount, int zCount, int* skip) {
			for (int z = 0; z < zCount; z++) {
				int heightmapIndex = (z1 + z) * Width + x1;
				for (int x = 0; x < xCount; x++) {
					int height = heightmap[heightmapIndex];
					if (height == short.MaxValue)
						heightmap[heightmapIndex] = -10;
					heightmapIndex++;
				}
			}
		}
	}
}