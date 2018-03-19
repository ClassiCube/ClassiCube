// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Renderers;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting through a simple heightmap, where each block is either in sun or shadow. </summary>
	public sealed partial class BasicLighting : IWorldLighting {
		
		public override void OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {
			int index = (z * width) + x;
			int lightH = heightmap[index];
			// Since light wasn't checked to begin with, means column never had meshes for any of its chunks built.
			// So we don't need to do anything.
			if (lightH == short.MaxValue) return;
			
			UpdateLighting(x, y, z, oldBlock, newBlock, index, lightH);
			int newHeight = heightmap[index] + 1;
			RefreshAffected(x, y, z, newBlock, lightH + 1, newHeight);
		}
		
		MapRenderer renderer;		
		void UpdateLighting(int x, int y, int z, BlockID oldBlock, 
		                    BlockID newBlock, int index, int lightH) {
			bool didBlock = BlockInfo.BlocksLight[oldBlock];
			bool nowBlocks = BlockInfo.BlocksLight[newBlock];
			int oldOffset = (BlockInfo.LightOffset[oldBlock] >> Side.Top) & 1;
			int newOffset = (BlockInfo.LightOffset[newBlock] >> Side.Top) & 1;
			
			// Two cases we need to handle here:
			if (didBlock == nowBlocks) {
				if (!didBlock) return;              // a) both old and new block do not block light
				if (oldOffset == newOffset) return; // b) both blocks blocked light at the same Y coordinate
			}
			
			if ((y - newOffset) >= lightH) {
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
				BlockID above = y == (height - 1) ? Block.Air : game.World.GetBlock(x, y + 1, z);
				if (BlockInfo.BlocksLight[above]) return;
				
				if (nowBlocks) {
					heightmap[index] = (short)(y - newOffset);
				} else {
					CalcHeightAt(x, y - 1, z, index);
				}
			}
			
		}
		
		void RefreshAffected(int x, int y, int z, BlockID block, int oldHeight, int newHeight) {
			int cx = x >> 4, cy = y >> 4, cz = z >> 4;
			renderer = game.MapRenderer;
			
			// NOTE: It's a lot faster to only update the chunks that are affected by the change in shadows,
			// rather than the entire column.
			int newCy = newHeight < 0 ? 0 : newHeight >> 4;
			int oldCy = oldHeight < 0 ? 0 : oldHeight >> 4;
			int minCy = Math.Min(oldCy, newCy), maxCy = Math.Max(oldCy, newCy);
			ResetColumn(cx, cy, cz, minCy, maxCy);
			World world = game.World;
			
			int bX = x & 0x0F, bY = y & 0x0F, bZ = z & 0x0F;
			if (bX == 0 && cx > 0)
				ResetNeighbour(x - 1, y, z, block, cx - 1, cy, cz, minCy, maxCy);
			if (bY == 0 && cy > 0 && Needs(block, world.GetBlock(x, y - 1, z)))
				renderer.RefreshChunk(cx, cy - 1, cz);
			if (bZ == 0 && cz > 0)
				ResetNeighbour(x, y, z - 1, block, cx, cy, cz - 1, minCy, maxCy);
			
			if (bX == 15 && cx < renderer.chunksX - 1)
				ResetNeighbour(x + 1, y, z, block, cx + 1, cy, cz, minCy, maxCy);
			if (bY == 15 && cy < renderer.chunksY - 1 && Needs(block, world.GetBlock(x, y + 1, z)))
				renderer.RefreshChunk(cx, cy + 1, cz);
			if (bZ == 15 && cz < renderer.chunksZ - 1)
				ResetNeighbour(x, y, z + 1, block, cx, cy, cz + 1, minCy, maxCy);
		}
		
		bool Needs(BlockID block, BlockID other) {
			return BlockInfo.Draw[block] != DrawType.Opaque || BlockInfo.Draw[other] != DrawType.Gas;
		}
		
		void ResetNeighbour(int x, int y, int z, BlockID block,
		                    int cx, int cy, int cz, int minCy, int maxCy) {
			World world = game.World;
			if (minCy == maxCy) {
				int index = x + world.Width * (z + y * world.Length);
				ResetNeighourChunk(cx, cy, cz, block, y, index, y);
			} else {
				for (cy = maxCy; cy >= minCy; cy--) {
					int maxY = Math.Min(world.Height - 1, (cy << 4) + 15);
					int index = x + world.Width * (z + maxY * world.Length);
					ResetNeighourChunk(cx, cy, cz, block, maxY, index, y);
				}
			}
		}
		
		void ResetNeighourChunk(int cx, int cy, int cz, BlockID block,
		                        int y, int index, int nY) {
			World world = game.World;
			int minY = cy << 4;
			
			// Update if any blocks in the chunk are affected by light change
			for (; y >= minY; y--) {
				BlockID other = world.blocks1[index];
				bool affected = y == nY ? Needs(block, other) : BlockInfo.Draw[other] != DrawType.Gas;
				if (affected) { renderer.RefreshChunk(cx, cy, cz); return; }
				index -= world.Width * world.Length;
			}
		}
		
		void ResetColumn(int cx, int cy, int cz, int minCy, int maxCy) {
			if (minCy == maxCy) {
				renderer.RefreshChunk(cx, cy, cz);
			} else {
				for (cy = maxCy; cy >= minCy; cy--)
					renderer.RefreshChunk(cx, cy, cz);
			}
		}
	}
}
