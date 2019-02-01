// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using BlockID = System.UInt16;
using BlockRaw = System.Byte;

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting through a simple heightmap, where each block is either in sun or shadow. </summary>
	public sealed partial class BasicLighting : IWorldLighting {
		
		int CalcHeightAt(int x, int maxY, int z, int index) {
			int i = (maxY * length + z) * width + x;
			BlockRaw[] blocks = game.World.blocks;
			
			if (BlockInfo.MaxUsed < 256) {
				for (int y = maxY; y >= 0; y--, i -= oneY) {
					int block = blocks[i];
					if (BlockInfo.BlocksLight[block]) {
						int offset = (BlockInfo.LightOffset[block] >> Side.Top) & 1;
						heightmap[index] = (short)(y - offset);
						return y - offset;
					}
				}
			} else {
				BlockRaw[] blocks2 = game.World.blocks2;
				for (int y = maxY; y >= 0; y--, i -= oneY) {
					int block = blocks[i] | (blocks2[i] << 8);
					if (BlockInfo.BlocksLight[block]) {
						int offset = (BlockInfo.LightOffset[block] >> Side.Top) & 1;
						heightmap[index] = (short)(y - offset);
						return y - offset;
					}
				}
			}
			
			heightmap[index] = -10;
			return -10;
		}
		
		unsafe int InitialHeightmapCoverage(int x1, int z1, int xCount, int zCount, int* skip) {
			int elemsLeft = 0, index = 0, curRunCount = 0;
			for (int z = 0; z < zCount; z++) {
				int heightmapIndex = (z1 + z) * width + x1;
				for (int x = 0; x < xCount; x++) {
					int lightH = heightmap[heightmapIndex++];
					skip[index] = 0;
					if (lightH == short.MaxValue) {
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
		
		unsafe bool CalculateHeightmapCoverage_8Bit(int x1, int z1, int xCount, int zCount, int elemsLeft, int* skip, BlockRaw* mapPtr) {
			int prevRunCount = 0;
			for (int y = height - 1; y >= 0; y--) {
				if (elemsLeft <= 0) return true;
				int i = x1 + width * (z1 + y * length);
				int heightmapIndex = x1 + z1 * width;
				
				for (int z = 0; z < zCount; z++) {
					int baseIndex = i;
					int index = z * xCount;
					for (int x = 0; x < xCount;) {
						int curRunCount = skip[index];
						x += curRunCount; i += curRunCount; index += curRunCount;
						
						if (x < xCount && BlockInfo.BlocksLight[mapPtr[i]]) {
							int lightOffset = (BlockInfo.LightOffset[mapPtr[i]] >> Side.Top) & 1;
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
							x += oldRunCount; index += oldRunCount; i += oldRunCount;
							prevRunCount = newRunCount;
						} else {
							prevRunCount = 0;
						}
						x++; i++; index++;
					}
					prevRunCount = 0;
					heightmapIndex += width;
					i = baseIndex + width; // advance one Z
				}
			}
			return false;
		}
		
		#if !ONLY_8BIT
		unsafe bool CalculateHeightmapCoverage_16Bit(int x1, int z1, int xCount, int zCount, int elemsLeft, int* skip, BlockRaw* mapPtr, BlockRaw* mapPtr2) {
			int prevRunCount = 0;
			for (int y = height - 1; y >= 0; y--) {
				if (elemsLeft <= 0) return true;
				int i = x1 + width * (z1 + y * length);
				int heightmapIndex = x1 + z1 * width;
				
				for (int z = 0; z < zCount; z++) {
					int baseIndex = i;
					int index = z * xCount;
					for (int x = 0; x < xCount;) {
						int curRunCount = skip[index];
						x += curRunCount; i += curRunCount; index += curRunCount;
						
						if (x < xCount && BlockInfo.BlocksLight[mapPtr[i] | (mapPtr2[i] << 8)]) {
							int lightOffset = (BlockInfo.LightOffset[mapPtr[i] | (mapPtr2[i] << 8)] >> Side.Top) & 1;
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
							x += oldRunCount; index += oldRunCount; i += oldRunCount;
							prevRunCount = newRunCount;
						} else {
							prevRunCount = 0;
						}
						x++; i++; index++;
					}
					prevRunCount = 0;
					heightmapIndex += width;
					i = baseIndex + width; // advance one Z
				}
			}
			return false;
		}
		#endif

		unsafe void FinishHeightmapCoverage(int x1, int z1, int xCount, int zCount, int* skip) {
			for (int z = 0; z < zCount; z++) {
				int heightmapIndex = (z1 + z) * width + x1;
				for (int x = 0; x < xCount; x++) {
					int lightH = heightmap[heightmapIndex];
					if (lightH == short.MaxValue)
						heightmap[heightmapIndex] = -10;
					heightmapIndex++;
				}
			}
		}
	}
}
