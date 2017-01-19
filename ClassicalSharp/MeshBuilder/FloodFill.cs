// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#if OCCLUSION
using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {

		unsafe int ComputeOcclusion() {
			int* didFlags = stackalloc int[chunkSize2];
			OpenTK.MemUtils.memset((IntPtr)didFlags, 0x00, 0, chunkSize2 * sizeof(int));
			int* stack = stackalloc int[chunkSize3 * 4];
			
			int i = 0;
			bool solidX = true, solidY = true, solidZ = true;
			for (int y = 0; y < 16; y++) {
				for (int z = 0; z < 16; z++) {
					int flagIndex = (y << 4) | z;
					int chunkIndex = (y + 1) * extChunkSize2 + (z + 1) * extChunkSize + (0 + 1);
					for (int x = 0; x < 16; x++) {
						byte block = chunk[chunkIndex];
						if (info.Draw[block] == DrawType.Opaque) {
							didFlags[flagIndex] |= (1 << x);
						} else if ((didFlags[flagIndex] & (1 << x)) == 0) {
							FloodFill(didFlags, stack, i, ref solidX, ref solidY, ref solidZ);
						}
						
						i++;
						chunkIndex++;
					}
				}
			}
			return (solidX ? 0x1 : 0) | (solidZ ? 0x2 : 0) | (solidY ? 0x4 : 0);
		}
		
		unsafe void FloodFill(int* didFlags, int* stack, int startIndex,
		                      ref bool solidX, ref bool solidY, ref bool solidZ) {
			int index = 0;
			stack[index++] = startIndex;
			bool tX0 = false, tX1 = false, tY0 = false,
			tY1 = false, tZ0 = false, tZ1 = false;
			
			while (index > 0) {
				int bIndex = stack[--index];
				int x = (bIndex & 0xF);
				int z = ((bIndex >> 4) & 0xF);
				int y = ((bIndex >> 8) & 0xF);
				int flagIndex = (y << 4) | z;
				didFlags[flagIndex] |= (1 << x);
				
				int chunkIndex = (y + 1) * extChunkSize2 + (z + 1) * extChunkSize + (x + 1);
				byte block = chunk[chunkIndex];
				if (info.Draw[block] != DrawType.Opaque) {
					if (x == 0)
						tX0 = true;
					else if ((didFlags[flagIndex] & (1 << (x - 1))) == 0)
						stack[index++] = bIndex - 1;
					
					if (x == 15)
						tX1 = true;
					else if ((didFlags[flagIndex] & (1 << (x + 1))) == 0)
						stack[index++] = bIndex + 1;
					
					if (z == 0)
						tZ0 = true;
					else if ((didFlags[flagIndex - 1] & (1 << x)) == 0)
						stack[index++] = bIndex - 16;
					
					if (z == 15)
						tZ1 = true;
					else if ((didFlags[flagIndex + 1] & (1 << x)) == 0)
						stack[index++] = bIndex + 16;
				}
				
				if (!info.IsOpaqueY[block]) {					
					if (y == 0)
						tY0 = true;
					else if ((didFlags[flagIndex - 16] & (1 << x)) == 0 
					        && !info.IsOpaqueY[chunk[chunkIndex - extChunkSize2]])
						stack[index++] = bIndex - 256;
					
					if (y == 15)
						tY1 = true;
					else if ((didFlags[flagIndex + 16] & (1 << x)) == 0 
					        && !info.IsOpaqueY[chunk[chunkIndex + extChunkSize2]])
						stack[index++] = bIndex + 256;
				}
			}
			
			if (tX0 && tX1) solidX = false;
			if (tY0 && tY1) solidY = false;
			if (tZ0 && tZ1) solidZ = false;
		}
	}
}
#endif