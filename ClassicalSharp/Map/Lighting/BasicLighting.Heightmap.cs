// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting through a simple heightmap, where each block is either in sun or shadow. </summary>
	public sealed partial class BasicLighting : IWorldLighting {
		
		int CalcHeightAt(int x, int maxY, int z, int index) {
			int mapIndex = (maxY * length + z) * width + x;
			byte[] blocks = game.World.blocks;
			
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
		
		unsafe bool CalculateHeightmapCoverage(int x1, int z1, int xCount, int zCount, int elemsLeft, int* skip, byte* mapPtr) {
			int prevRunCount = 0;
			for (int y = height - 1; y >= 0; y--) {
				if (elemsLeft <= 0) return true;
				int mapIndex = x1 + width * (z1 + y * length);
				int heightmapIndex = x1 + z1 * width;
				
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
					heightmapIndex += width;
					mapIndex = baseIndex + width; // advance one Z
				}
			}
			return false;
		}
		
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
