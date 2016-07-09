// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	public unsafe sealed class NormalMeshBuilder : ChunkMeshBuilder {
		
		protected override int StretchXLiquid( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block ) {
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
		
		protected override int StretchX( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
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
		
		protected override int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
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
	}
}