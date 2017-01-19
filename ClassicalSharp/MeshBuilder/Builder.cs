// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
//#define DEBUG_OCCLUSION
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Class responsible for converting a 16x16x16 into an optimised mesh of vertices. </summary>
	/// <remarks> This class is heavily optimised and as such may suffer from slightly unreadable code. </remarks>
	public unsafe abstract partial class ChunkMeshBuilder {
		
		protected int X, Y, Z;
		protected float x1, y1, z1, x2, y2, z2;
		protected byte curBlock;
		protected BlockInfo info;
		protected World map;
		protected IWorldLighting lighting;
		protected WorldEnv env;
		protected Game game;
		protected IGraphicsApi gfx;
		protected const int chunkSize = 16, extChunkSize = 18;
		protected const int chunkSize2 = 16 * 16, extChunkSize2 = 18 * 18;
		protected const int chunkSize3 = 16 * 16 * 16, extChunkSize3 = 18 * 18 * 18;
		
		public void Init(Game game) {
			this.game = game;
			gfx = game.Graphics;
			info = game.BlockInfo;
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
		}
		
		protected internal int width, length, height, sidesLevel, edgeLevel;
		protected int maxX, maxY, maxZ;
		protected int cIndex;
		protected byte* chunk, counts;
		protected int* bitFlags;
		protected bool useBitFlags;
		
		bool BuildChunk(int x1, int y1, int z1) {
			lighting = game.Lighting;
			PreStretchTiles(x1, y1, z1);
			byte* chunkPtr = stackalloc byte[extChunkSize3]; chunk = chunkPtr;
			byte* countsPtr = stackalloc byte[chunkSize3 * Side.Sides]; counts = countsPtr;
			int* bitsPtr = stackalloc int[extChunkSize3]; bitFlags = bitsPtr;
			
			MemUtils.memset((IntPtr)chunkPtr, 0, 0, extChunkSize3);
			if (ReadChunkData(x1, y1, z1)) return false;
			MemUtils.memset((IntPtr)countsPtr, 1, 0, chunkSize3 * Side.Sides);
			
			Stretch(x1, y1, z1);
			PostStretchTiles(x1, y1, z1);
			
			int xMax = Math.Min(width, x1 + chunkSize);
			int yMax = Math.Min(height, y1 + chunkSize);
			int zMax = Math.Min(length, z1 + chunkSize);
			for (int y = y1, yy = 0; y < yMax; y++, yy++) {
				for (int z = z1, zz = 0; z < zMax; z++, zz++) {
					
					int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (0 + 1);
					for (int x = x1, xx = 0; x < xMax; x++, xx++) {
						curBlock = chunk[chunkIndex];
						if (info.Draw[curBlock] != DrawType.Gas) {
							int index = ((yy << 8) | (zz << 4) | xx) * Side.Sides;
							X = x; Y = y; Z = z;
							cIndex = chunkIndex;
							RenderTile(index);
						}
						chunkIndex++;
					}
				}
			}
			return true;
		}
		
		bool ReadChunkData(int x1, int y1, int z1) {
			bool allAir = true, allSolid = true;
			fixed(byte* mapPtr = map.blocks) {
				
				for (int yy = -1; yy < 17; yy++) {
					int y = yy + y1;
					if (y < 0) continue;
					if (y >= height) break;
					for (int zz = -1; zz < 17; zz++) {
						int z = zz + z1;
						if (z < 0) continue;
						if (z >= length) break;
						
						int index = (y * length + z) * width + (x1 - 1 - 1);
						int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (-1 + 1) - 1;
						
						for (int xx = -1; xx < 17; xx++) {
							int x = xx + x1;
							index++;
							chunkIndex++;
							if (x < 0) continue;
							if (x >= width) break;							
							byte rawBlock = mapPtr[index];
							
							allAir = allAir && info.Draw[rawBlock] == DrawType.Gas;
							allSolid = allSolid && info.Draw[rawBlock] == DrawType.Opaque;
							chunk[chunkIndex] = rawBlock;
						}
					}
				}
				
				if (x1 == 0 || y1 == 0 || z1 == 0 || x1 + chunkSize >= width ||
				   y1 + chunkSize >= height || z1 + chunkSize >= length) allSolid = false;
				if (allAir || allSolid) return true;
				
				lighting.LightHint(x1 - 1, z1 - 1, mapPtr);
				return false;
			}
		}
		
		public void GetDrawInfo(int x, int y, int z, ref ChunkPartInfo[] nParts, ref ChunkPartInfo[] tParts) {
			if (!BuildChunk(x, y, z)) return;
			
			for (int i = 0; i < arraysCount; i++) {
				SetPartInfo(normalParts[i], i, ref nParts);
				SetPartInfo(translucentParts[i], i, ref tParts);
			}
			#if OCCLUSION
			//  , ref byte occlusionFlags
			if (normalParts != null || translucentParts != null)
				occlusionFlags = (byte)ComputeOcclusion();
			#endif
		}
		
		void SetPartInfo(DrawInfo part, int i, ref ChunkPartInfo[] parts) {
			if (part.iCount == 0) return;
			
			ChunkPartInfo info;
			int vertCount = (part.iCount / 6 * 4) + 2;
			info.VbId = gfx.CreateVb(part.vertices, VertexFormat.P3fT2fC4b, vertCount);
			info.IndicesCount = part.iCount;
			info.LeftCount = (ushort)part.vCount.left; info.RightCount = (ushort)part.vCount.right;
			info.FrontCount = (ushort)part.vCount.front; info.BackCount = (ushort)part.vCount.back;
			info.BottomCount = (ushort)part.vCount.bottom; info.TopCount = (ushort)part.vCount.top;
			info.SpriteCount = part.spriteCount;
			
			info.LeftIndex = info.SpriteCount;
			info.RightIndex = info.LeftIndex + info.LeftCount;
			info.FrontIndex = info.RightIndex + info.RightCount;
			info.BackIndex = info.FrontIndex + info.FrontCount;
			info.BottomIndex = info.BackIndex + info.BackCount;
			info.TopIndex = info.BottomIndex + info.BottomCount;
			
			// Lazy initalize part arrays so we can save time in MapRenderer for chunks that only contain 1 or 2 part types.
			if (parts == null)
				parts = new ChunkPartInfo[arraysCount];
			parts[i] = info;
		}
		
		void Stretch(int x1, int y1, int z1) {
			int xMax = Math.Min(width, x1 + chunkSize);
			int yMax = Math.Min(height, y1 + chunkSize);
			int zMax = Math.Min(length, z1 + chunkSize);
			#if OCCLUSION
			int flags = ComputeOcclusion();
			#endif
			#if DEBUG_OCCLUSION
			FastColour col = new FastColour(60, 60, 60, 255);
			if ((flags & 1) != 0) col.R = 255; // x
			if ((flags & 4) != 0) col.G = 255; // y
			if ((flags & 2) != 0) col.B = 255; // z
			map.Sunlight = map.Shadowlight = col;
			map.SunlightXSide = map.ShadowlightXSide = col;
			map.SunlightZSide = map.ShadowlightZSide = col;
			map.SunlightYBottom = map.ShadowlightYBottom = col;
			#endif
			byte[] hidden = game.BlockInfo.hidden;
			
			for (int y = y1, yy = 0; y < yMax; y++, yy++) {
				for (int z = z1, zz = 0; z < zMax; z++, zz++) {
					int cIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (-1 + 1);
					for (int x = x1, xx = 0; x < xMax; x++, xx++) {
						cIndex++;
						byte b = chunk[cIndex];
						if (info.Draw[b] == DrawType.Gas) continue;
						int index = ((yy << 8) + (zz << 4) + xx) * Side.Sides;
						
						// Sprites only use one face to indicate stretching count, so we can take a shortcut here.
						// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
						if (info.Draw[b] == DrawType.Sprite) {
							index += Side.Top;
							if (counts[index] != 0) {
								X = x; Y = y; Z = z;
								AddSpriteVertices(b);
								counts[index] = 1;
							}
							continue;
						}
						
						X = x; Y = y; Z = z;
						fullBright = info.FullBright[b];
						int tileIdx = b << 8;
						// All of these function calls are inlined as they can be called tens of millions to hundreds of millions of times.
						
						if (counts[index] == 0 || 
						   (x == 0 && (Y < sidesLevel || (b >= Block.Water && b <= Block.StillLava && Y < edgeLevel))) ||
						   (x != 0 && (hidden[tileIdx + chunk[cIndex - 1]] & (1 << Side.Left)) != 0)) {
							counts[index] = 0;
						} else {
							int count = StretchZ(zz, index, X, Y, Z, cIndex, b, Side.Left);
							AddVertices(b, count, Side.Left); counts[index] = (byte)count;
						}
						
						index++;
						if (counts[index] == 0 || 
						   (x == maxX && (Y < sidesLevel || (b >= Block.Water && b <= Block.StillLava && Y < edgeLevel))) ||
						   (x != maxX && (hidden[tileIdx + chunk[cIndex + 1]] & (1 << Side.Right)) != 0)) {
							counts[index] = 0;
						} else {
							int count = StretchZ(zz, index, X, Y, Z, cIndex, b, Side.Right);
							AddVertices(b, count, Side.Right); counts[index] = (byte)count;
						}
						
						index++;
						if (counts[index] == 0 || 
						   (z == 0 && (Y < sidesLevel || (b >= Block.Water && b <= Block.StillLava && Y < edgeLevel))) ||
						   (z != 0 && (hidden[tileIdx + chunk[cIndex - 18]] & (1 << Side.Front)) != 0)) {
							counts[index] = 0;
						} else {
							int count = StretchX(xx, index, X, Y, Z, cIndex, b, Side.Front);
							AddVertices(b, count, Side.Front); counts[index] = (byte)count;
						}
						
						index++;
						if (counts[index] == 0 || 
						   (z == maxZ && (Y < sidesLevel || (b >= Block.Water && b <= Block.StillLava && Y < edgeLevel))) ||
						   (z != maxZ && (hidden[tileIdx + chunk[cIndex + 18]] & (1 << Side.Back)) != 0)) {
							counts[index] = 0;
						} else {
							int count = StretchX(xx, index, X, Y, Z, cIndex, b, Side.Back);
							AddVertices(b, count, Side.Back); counts[index] = (byte)count;
						}
						
						index++;
						if (counts[index] == 0 || y == 0 ||
						   (hidden[tileIdx + chunk[cIndex - 324]] & (1 << Side.Bottom)) != 0) {
							counts[index] = 0;
						} else {
							int count = StretchX(xx, index, X, Y, Z, cIndex, b, Side.Bottom);
							AddVertices(b, count, Side.Bottom); counts[index] = (byte)count;
						}
						
						index++;
						if (counts[index] == 0 ||
						   (hidden[tileIdx + chunk[cIndex + 324]] & (1 << Side.Top)) != 0) {
							counts[index] = 0;
						} else if (b < Block.Water || b > Block.StillLava) {
							int count = StretchX(xx, index, X, Y, Z, cIndex, b, Side.Top);
							AddVertices(b, count, Side.Top); counts[index] = (byte)count;
						} else {
							int count = StretchXLiquid(xx, index, X, Y, Z, cIndex, b);
							if (count > 0) AddVertices(b, count, Side.Top);
							counts[index] = (byte)count;
						}
					}
				}
			}
		}
		
		protected abstract int StretchXLiquid(int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block);
		
		protected abstract int StretchX(int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face);
		
		protected abstract int StretchZ(int zz, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face);
		
		protected static int[] offsets = { -1, 1, -extChunkSize, extChunkSize, -extChunkSize2, extChunkSize2 };
		
		protected bool OccludedLiquid(int chunkIndex) {
			return 
				info.Draw[chunk[chunkIndex + 324]] == DrawType.Opaque
				&& info.Draw[chunk[chunkIndex + 324 - 18]] != DrawType.Gas
				&& info.Draw[chunk[chunkIndex + 324 - 1]] != DrawType.Gas
				&& info.Draw[chunk[chunkIndex + 324 + 1]] != DrawType.Gas
				&& info.Draw[chunk[chunkIndex + 324 + 18]] != DrawType.Gas;
		}
		
		protected bool IsLit(int x, int y, int z, int face, byte type) {
			int offset = (info.LightOffset[type] >> face) & 1;
			switch (face) {
				case Side.Left:
					return x < offset || y > lighting.heightmap[(z * width) + (x - offset)];
				case Side.Right:
					return x > (maxX - offset) || y > lighting.heightmap[(z * width) + (x + offset)];
				case Side.Front:
					return z < offset || y > lighting.heightmap[((z - offset) * width) + x];
				case Side.Back:
					return z > (maxZ - offset) || y > lighting.heightmap[((z + offset) * width) + x];
				case Side.Bottom:
					return y <= 0 || (y - 1 - offset) >= (lighting.heightmap[(z * width) + x]);
				case Side.Top:
					return y >= maxY || (y - offset) >= (lighting.heightmap[(z * width) + x]);
			}
			return true;
		}
		
		public void OnNewMapLoaded() {
			map = game.World;
			env = game.World.Env;
			width = map.Width; height = map.Height; length = map.Length;
			maxX = width - 1; maxY = height - 1; maxZ = length - 1;
			
			sidesLevel = Math.Max(0, game.World.Env.SidesHeight);
			edgeLevel = Math.Max(0, game.World.Env.EdgeHeight);
		}
	}
	
	public struct ChunkPartInfo {
		
		public int VbId, IndicesCount, SpriteCount;
		public int LeftIndex, RightIndex, FrontIndex,
		BackIndex, BottomIndex, TopIndex;
		public ushort LeftCount, RightCount, FrontCount,
		BackCount, BottomCount, TopCount;
	}
}