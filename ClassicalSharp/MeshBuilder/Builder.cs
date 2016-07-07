// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
//#define DEBUG_OCCLUSION
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Class responsible for converting a 16x16x16 into an optimised mesh of vertices. </summary>
	/// <remarks> This class is heavily optimised and as such may suffer from slightly unreadable code. </remarks>
	public unsafe partial class ChunkMeshBuilder {
		
		int X, Y, Z;
		float x1, y1, z1, x2, y2, z2;
		byte curBlock;
		BlockInfo info;
		World map;
		WorldEnv env;
		Game game;
		IGraphicsApi graphics;
		const int chunkSize = 16, extChunkSize = 18;
		const int chunkSize2 = 16 * 16, extChunkSize2 = 18 * 18;
		const int chunkSize3 = 16 * 16 * 16, extChunkSize3 = 18 * 18 * 18;
		
		public ChunkMeshBuilder( Game game ) {
			this.game = game;
			graphics = game.Graphics;
			info = game.BlockInfo;
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
		}
		
		internal int width, length, height, clipLevel;
		int maxX, maxY, maxZ;
		byte* chunk, counts;
		
		bool BuildChunk( int x1, int y1, int z1 ) {
			PreStretchTiles( x1, y1, z1 );
			byte* chunkPtr = stackalloc byte[extChunkSize3]; chunk = chunkPtr;
			byte* countsPtr = stackalloc byte[chunkSize3 * Side.Sides]; counts = countsPtr;
			MemUtils.memset( (IntPtr)chunkPtr, 0, 0, extChunkSize3 );
			if( ReadChunkData( x1, y1, z1 ) ) return false;
			MemUtils.memset( (IntPtr)countsPtr, 1, 0, chunkSize3 * Side.Sides );
			
			Stretch( x1, y1, z1 );
			PostStretchTiles( x1, y1, z1 );
			
			int xMax = Math.Min( width, x1 + chunkSize );
			int yMax = Math.Min( height, y1 + chunkSize );
			int zMax = Math.Min( length, z1 + chunkSize );
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (0 + 1);
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						curBlock = chunk[chunkIndex];
						if( !info.IsAir[curBlock] ) {
							int index = ((yy << 8) | (zz << 4) | xx) * Side.Sides;
							RenderTile( index, x, y, z );
						}
						chunkIndex++;
					}
				}
			}
			return true;
		}
		
		bool ReadChunkData( int x1, int y1, int z1 ) {
			bool allAir = true, allSolid = true;
			fixed( byte* mapPtr = map.blocks ) {
				
				for( int yy = -1; yy < 17; yy++ ) {
					int y = yy + y1;
					if( y < 0 ) continue;
					if( y > maxY ) break;
					for( int zz = -1; zz < 17; zz++ ) {
						int z = zz + z1;
						if( z < 0 ) continue;
						if( z > maxZ ) break;
						
						int index = (y * length + z) * width + (x1 - 1 - 1);
						int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (-1 + 1) - 1;
						
						for( int xx = -1; xx < 17; xx++ ) {
							int x = xx + x1;
							index++;
							chunkIndex++;
							if( x < 0 ) continue;
							if( x > maxX ) break;
							
							byte rawBlock = mapPtr[index];
							if( rawBlock == Block.StillWater ) rawBlock = Block.Water;
							if( rawBlock == Block.StillLava ) rawBlock = Block.Lava;
							
							allAir = allAir && rawBlock == 0;
							allSolid = allSolid && info.IsOpaque[rawBlock];
							chunk[chunkIndex] = rawBlock;
						}
					}
				}
				
				if( x1 == 0 || y1 == 0 || z1 == 0 || x1 + chunkSize >= width ||
				   y1 + chunkSize >= height || z1 + chunkSize >= length ) allSolid = false;
				if( allAir || allSolid ) return true;
				
				map.HeightmapHint( x1 - 1, z1 - 1, mapPtr );
				return false;
			}
		}
		
		public void GetDrawInfo( int x, int y, int z, ref ChunkPartInfo[] nParts, ref ChunkPartInfo[] tParts ) {
			if( !BuildChunk( x, y, z ) ) return;
			
			for( int i = 0; i < arraysCount; i++ ) {
				SetPartInfo( normalParts[i], i, ref nParts );
				SetPartInfo( translucentParts[i], i, ref tParts );
			}
			#if OCCLUSION
			//  , ref byte occlusionFlags
			if( normalParts != null || translucentParts != null )
				occlusionFlags = (byte)ComputeOcclusion();
			#endif
		}
		
		void SetPartInfo( DrawInfo part, int i, ref ChunkPartInfo[] parts ) {
			if( part.iCount == 0 ) return;
			
			ChunkPartInfo info;
			int vertCount = (part.iCount / 6 * 4) + 2;
			info.VbId = graphics.CreateVb( part.vertices, VertexFormat.P3fT2fC4b, vertCount );
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
			if( parts == null )
				parts = new ChunkPartInfo[arraysCount];
			parts[i] = info;
		}
		
		void Stretch( int x1, int y1, int z1 ) {
			int xMax = Math.Min( width, x1 + chunkSize );
			int yMax = Math.Min( height, y1 + chunkSize );
			int zMax = Math.Min( length, z1 + chunkSize );
			#if OCCLUSION
			int flags = ComputeOcclusion();			
			#endif
			#if DEBUG_OCCLUSION
			FastColour col = new FastColour( 60, 60, 60, 255 );
			if( (flags & 1) != 0 ) col.R = 255; // x
			if( (flags & 4) != 0 ) col.G = 255; // y
			if( (flags & 2) != 0 ) col.B = 255; // z
			map.Sunlight = map.Shadowlight = col;
			map.SunlightXSide = map.ShadowlightXSide = col;
			map.SunlightZSide = map.ShadowlightZSide = col;
			map.SunlightYBottom = map.ShadowlightYBottom = col;
			#endif
			byte[] hidden = game.BlockInfo.hidden;
			
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int cIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (-1 + 1);
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						cIndex++;
						byte rawBlock = chunk[cIndex];
						if( info.IsAir[rawBlock] ) continue;
						int index = ((yy << 8) + (zz << 4) + xx) * Side.Sides;
						
						// Sprites only use one face to indicate stretching count, so we can take a shortcut here.
						// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
						if( info.IsSprite[rawBlock] ) {
							index += Side.Top;
							if( counts[index] != 0 ) {
								X = x; Y = y; Z = z;
								AddSpriteVertices( rawBlock );
								counts[index] = 1;
							}
						} else {
							X = x; Y = y; Z = z;
							fullBright = info.FullBright[rawBlock];
							int tileIdx = rawBlock << 8;
							// All of these function calls are inlined as they can be called tens of millions to hundreds of millions of times.
							
							if( counts[index] == 0 || (x == 0 && Y < clipLevel) ||
							   (x != 0 && (hidden[tileIdx + chunk[cIndex - 1]] & (1 << Side.Left)) != 0 ) ) {
								counts[index] = 0;
							} else {
								int count = StretchZ( zz, index, X, Y, Z, cIndex, rawBlock, Side.Left );
								AddVertices( rawBlock, count, Side.Left ); counts[index] = (byte)count;
							}
							
							index++;
							if( counts[index] == 0 || (x == maxX && Y < clipLevel) ||
							   (x != maxX && (hidden[tileIdx + chunk[cIndex + 1]] & (1 << Side.Right)) != 0 ) ) {
								counts[index] = 0;
							} else {
								int count = StretchZ( zz, index, X, Y, Z, cIndex, rawBlock, Side.Right );
								AddVertices( rawBlock, count, Side.Right ); counts[index] = (byte)count;
							}
							
							index++;
							if( counts[index] == 0 || (z == 0 && Y < clipLevel) ||
							   (z != 0 && (hidden[tileIdx + chunk[cIndex - 18]] & (1 << Side.Front)) != 0 ) ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, cIndex, rawBlock, Side.Front );
								AddVertices( rawBlock, count, Side.Front ); counts[index] = (byte)count;
							}
							
							index++;
							if( counts[index] == 0 || (z == maxZ && Y < clipLevel) ||
							   (z != maxZ && (hidden[tileIdx + chunk[cIndex + 18]] & (1 << Side.Back)) != 0 ) ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, cIndex, rawBlock, Side.Back );
								AddVertices( rawBlock, count, Side.Back ); counts[index] = (byte)count;
							}
							
							index++;
							if( counts[index] == 0 || y == 0 || 
							   (hidden[tileIdx + chunk[cIndex - 324]] & (1 << Side.Bottom)) != 0 ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, cIndex, rawBlock, Side.Bottom );
								AddVertices( rawBlock, count, Side.Bottom ); counts[index] = (byte)count;
							}
							
							index++;
							if( counts[index] == 0 || 
							   (hidden[tileIdx + chunk[cIndex + 324]] & (1 << Side.Top)) != 0 ) {
								counts[index] = 0;
							} else if( rawBlock < Block.Water || rawBlock > Block.StillLava ) {
								int count = StretchX( xx, index, X, Y, Z, cIndex, rawBlock, Side.Top );
								AddVertices( rawBlock, count, Side.Top ); counts[index] = (byte)count;
							} else {
								int count = StretchXLiquid( xx, index, X, Y, Z, cIndex, rawBlock );
								if( count > 0 ) AddVertices( rawBlock, count, Side.Top );
								counts[index] = (byte)count;
							}						
						}
					}
				}
			}
		}
		
		int StretchXLiquid( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block ) {
			if( OccludedLiquid( chunkIndex ) ) return 0;			
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			int max = chunkSize - xx;			
			
			while( count < max && x < width && CanStretch( block, chunkIndex, x, y, z, Side.Top ) 
			      && !OccludedLiquid( chunkIndex ) ) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		int StretchX( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += Side.Sides;
			int max = chunkSize - xx;
			bool stretchTile = (info.CanStretch[block] & (1 << face)) != 0;
			
			while( count < max && x < width && stretchTile && CanStretch( block, chunkIndex, x, y, z, face ) ) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += Side.Sides;
			}
			return count;
		}
		
		int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
			int count = 1;
			z++;
			chunkIndex += extChunkSize;
			countIndex += chunkSize * Side.Sides;
			int max = chunkSize - zz;
			bool stretchTile = (info.CanStretch[block] & (1 << face)) != 0;
			
			while( count < max && z < length && stretchTile && CanStretch( block, chunkIndex, x, y, z, face ) ) {
				counts[countIndex] = 0;
				count++;
				z++;
				chunkIndex += extChunkSize;
				countIndex += chunkSize * Side.Sides;
			}
			return count;
		}
		
		int[] offsets = { -1, 1, -extChunkSize, extChunkSize, -extChunkSize2, extChunkSize2 };
		bool CanStretch( byte initialTile, int chunkIndex, int x, int y, int z, int face ) {
			byte rawBlock = chunk[chunkIndex];
			return rawBlock == initialTile && !info.IsFaceHidden( rawBlock, chunk[chunkIndex + offsets[face]], face )
				&& (fullBright || IsLit( X, Y, Z, face, initialTile ) == IsLit( x, y, z, face, rawBlock ) );
		}
		
		bool OccludedLiquid( int chunkIndex ) {
			return info.IsOpaque[chunk[chunkIndex + 324]] && !info.IsAir[chunk[chunkIndex + 324 - 18]] && 
				!info.IsAir[chunk[chunkIndex + 324 - 1]] && !info.IsAir[chunk[chunkIndex + 324 + 1]] && 
				!info.IsAir[chunk[chunkIndex + 324 + 18]];
		}
		
		bool IsLit( int x, int y, int z, int face, byte type ) {
			int offset = (info.LightOffset[type] >> face) & 1;
			switch( face ) {
				case Side.Left:
					return x < offset || y > map.heightmap[(z * width) + (x - offset)];
				case Side.Right:
					return x > (maxX - offset) || y > map.heightmap[(z * width) + (x + offset)];
				case Side.Front:
					return z < offset || y > map.heightmap[((z - offset) * width) + x];
				case Side.Back:
					return z > (maxZ - offset) || y > map.heightmap[((z + offset) * width) + x];
				case Side.Bottom:
					return y <= 0 || (y - 1 - offset) >= (map.heightmap[(z * width) + x]);
				case Side.Top:
					return y >= maxY || (y - offset) >= (map.heightmap[(z * width) + x]);
			}
			return true;
		}
		
		public void OnNewMapLoaded() {
			map = game.World;
			env = game.World.Env;
			width = map.Width;
			height = map.Height;
			length = map.Length;
			clipLevel = Math.Max( 0, game.World.Env.SidesHeight );
			maxX = width - 1;
			maxY = height - 1;
			maxZ = length - 1;
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