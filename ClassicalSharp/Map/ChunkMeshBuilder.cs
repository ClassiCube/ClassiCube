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
		byte tile;
		BlockInfo info;
		World map;
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
			byte* countsPtr = stackalloc byte[chunkSize3 * TileSide.Sides]; counts = countsPtr;
			MemUtils.memset( (IntPtr)chunkPtr, 0, 0, extChunkSize3 );
			if( ReadChunkData( x1, y1, z1 ) ) return false;
			MemUtils.memset( (IntPtr)countsPtr, 1, 0, chunkSize3 * TileSide.Sides );
			
			Stretch( x1, y1, z1 );
			PostStretchTiles( x1, y1, z1 );
			
			int xMax = Math.Min( width, x1 + chunkSize );
			int yMax = Math.Min( height, y1 + chunkSize );
			int zMax = Math.Min( length, z1 + chunkSize );
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (0 + 1);
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						tile = chunk[chunkIndex];
						if( !info.IsAir[tile] ) {
							int index = ((yy << 8) | (zz << 4) | xx) * TileSide.Sides;
							RenderTile( chunkIndex, index, x, y, z );
						}
						chunkIndex++;
					}
				}
			}
			return true;
		}
		
		bool ReadChunkData( int x1, int y1, int z1 ) {
			bool allAir = true, allSolid = true;
			fixed( byte* mapPtr = map.mapData ) {
				
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
							
							byte block = mapPtr[index];
							if( block == 9 ) block = 8; // Still water --> Water
							if( block == 11 ) block = 10; // Still lava --> Lava
							
							if( allAir && block != 0 ) allAir = false;
							if( allSolid && !info.IsOpaque[block] ) allSolid = false;
							chunk[chunkIndex] = block;
						}
					}
				}
				
				if( x1 == 0 || y1 == 0 || z1 == 0 || x1 + chunkSize >= width ||
				   y1 + chunkSize >= height || z1 + chunkSize >= length ) allSolid = false;
				if( !( allAir || allSolid ) ) {
					map.HeightmapHint( x1 - 1, z1 - 1, mapPtr );
				}
			}
			return allAir || allSolid;
		}
		
		public void GetDrawInfo( int x, int y, int z, ref ChunkPartInfo[] normalParts,
		                        ref ChunkPartInfo[] translucentParts ) {
			if( !BuildChunk( x, y, z ) ) return;
			
			for( int i = 0; i < arraysCount; i++ ) {
				SetPartInfo( drawInfoNormal[i], i, ref normalParts );
				SetPartInfo( drawInfoTranslucent[i], i, ref translucentParts );
			}
			#if OCCLUSION
			//  , ref byte occlusionFlags
			if( normalParts != null || translucentParts != null )
				occlusionFlags = (byte)ComputeOcclusion();
			#endif
		}

		Vector3 minBB, maxBB;
		public void RenderTile( int chunkIndex, int index, int x, int y, int z ) {
			X = x; Y = y; Z = z;
			
			if( info.IsSprite[tile] ) {
				fullBright = info.FullBright[tile];
				int count = counts[index + TileSide.Top];
				if( count != 0 )
					DrawSprite( count );
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if( leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			   backCount == 0 && bottomCount == 0 && topCount == 0 ) return;
			
			fullBright = info.FullBright[tile];
			isTranslucent = info.IsTranslucent[tile];
			lightFlags = info.LightOffset[tile];
			
			Vector3 min = info.MinBB[tile], max = info.MaxBB[tile];
			x1 = x + min.X; y1 = y + min.Y; z1 = z + min.Z;
			x2 = x + max.X; y2 = y + max.Y; z2 = z + max.Z;
			if( isTranslucent && info.Collide[tile] != CollideType.Solid ) {
				x1 -= 0.1f/16; x2 -= 0.1f/16f; z1 -= 0.1f/16f; z2 -= 0.1f/16f;
				y1 -= 0.1f/16; y2 -= 0.1f/16f;
			}
			this.minBB = min; this.maxBB = max;
			minBB.Y = 1 - minBB.Y; maxBB.Y = 1 - maxBB.Y;
			
			if( leftCount != 0 ) DrawLeftFace( leftCount );
			if( rightCount != 0 ) DrawRightFace( rightCount );
			if( frontCount != 0 ) DrawFrontFace( frontCount );
			if( backCount != 0 ) DrawBackFace( backCount );
			if( bottomCount != 0 ) DrawBottomFace( bottomCount );
			if( topCount != 0 ) DrawTopFace( topCount );
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
			bool[] hidden = game.BlockInfo.hidden;
			
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (-1 + 1);
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						chunkIndex++;
						byte tile = chunk[chunkIndex];
						if( info.IsAir[tile] ) continue;
						int countIndex = ((yy << 8) + (zz << 4) + xx) * TileSide.Sides;
						
						// Sprites only use one face to indicate stretching count, so we can take a shortcut here.
						// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
						if( info.IsSprite[tile] ) {
							countIndex += TileSide.Top;
							if( counts[countIndex] != 0 ) {
								X = x; Y = y; Z = z;
								AddSpriteVertices( tile );
								counts[countIndex] = 1;
							}
						} else {
							X = x; Y = y; Z = z;
							fullBright = info.FullBright[tile];
							int tileAdj = tile * BlockInfo.BlocksCount;
							// All of these function calls are inlined as they can be called tens of millions to hundreds of millions of times.
							
							int index = countIndex + TileSide.Left;
							if( counts[index] == 0 || (x == 0 && Y < clipLevel) ||
							   (x != 0 && hidden[(tileAdj + chunk[chunkIndex - 1]) * TileSide.Sides + TileSide.Left]) ) {
								counts[index] = 0;
							} else {
								int count = StretchZ( zz, index, X, Y, Z, chunkIndex, tile, TileSide.Left );
								AddVertices( tile, count, TileSide.Left ); counts[index] = (byte)count;
							}
							
							index = countIndex + TileSide.Right;
							if( counts[index] == 0 || (x == maxX && Y < clipLevel) ||
							   (x != maxX && hidden[(tileAdj + chunk[chunkIndex + 1]) * TileSide.Sides + TileSide.Right]) ) {
								counts[index] = 0;
							} else {
								int count = StretchZ( zz, index, X, Y, Z, chunkIndex, tile, TileSide.Right );
								AddVertices( tile, count, TileSide.Right ); counts[index] = (byte)count;
							}
							
							index = countIndex + TileSide.Front;
							if( counts[index] == 0 || (z == 0 && Y < clipLevel) ||
							   (z != 0 && hidden[(tileAdj + chunk[chunkIndex - 18]) * TileSide.Sides + TileSide.Front]) ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, chunkIndex, tile, TileSide.Front );
								AddVertices( tile, count, TileSide.Front ); counts[index] = (byte)count;
							}
							
							index = countIndex + TileSide.Back;
							if( counts[index] == 0 || (z == maxZ && Y < clipLevel) ||
							   (z != maxZ && hidden[(tileAdj + chunk[chunkIndex + 18]) * TileSide.Sides + TileSide.Back]) ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, chunkIndex, tile, TileSide.Back );
								AddVertices( tile, count, TileSide.Back ); counts[index] = (byte)count;
							}
							
							index = countIndex + TileSide.Bottom;
							if( counts[index] == 0 || y == 0 || 
							   hidden[(tileAdj + chunk[chunkIndex - 324]) * TileSide.Sides + TileSide.Bottom] ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, chunkIndex, tile, TileSide.Bottom );
								AddVertices( tile, count, TileSide.Bottom ); counts[index] = (byte)count;
							}
							
							index = countIndex + TileSide.Top;
							if( counts[index] == 0 || 
							   hidden[(tileAdj + chunk[chunkIndex + 324]) * TileSide.Sides + TileSide.Top] ) {
								counts[index] = 0;
							} else {
								int count = StretchX( xx, index, X, Y, Z, chunkIndex, tile, TileSide.Top );
								AddVertices( tile, count, TileSide.Top ); counts[index] = (byte)count;
							}
							
						}
					}
				}
			}
		}
		
		int StretchX( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte tile, int face ) {
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += TileSide.Sides;
			int max = chunkSize - xx;
			bool stretchTile = info.CanStretch[tile * TileSide.Sides + face];
			
			while( count < max && x < width && stretchTile && CanStretch( tile, chunkIndex, x, y, z, face ) ) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += TileSide.Sides;
			}
			return count;
		}
		
		int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte tile, int face ) {
			int count = 1;
			z++;
			chunkIndex += extChunkSize;
			countIndex += chunkSize * TileSide.Sides;
			int max = chunkSize - zz;
			bool stretchTile = info.CanStretch[tile * TileSide.Sides + face];
			
			while( count < max && z < length && stretchTile && CanStretch( tile, chunkIndex, x, y, z, face ) ) {
				counts[countIndex] = 0;
				count++;
				z++;
				chunkIndex += extChunkSize;
				countIndex += chunkSize * TileSide.Sides;
			}
			return count;
		}
		
		public void OnNewMap() {
		}
		
		public void OnNewMapLoaded() {
			map = game.World;
			width = map.Width;
			height = map.Height;
			length = map.Length;
			clipLevel = Math.Max( 0, game.World.SidesHeight );
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