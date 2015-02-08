using System;
using ClassicalSharp.Blocks.Model;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.World;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {
		
		unsafe void CopyMainPart( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr, byte* metaPtr ) {
			Chunk chunk = map.GetChunk( x1 >> 4, z1 >> 4 );
			if( chunk == null ) {
				Utils.LogWarning( "um what? " + ( x1 >> 4 ) + " , " + ( z1 >> 4 ) + "(" + x1 + " , " + z1 + ")" );
				return;
			}
			
			for( int yy = -1; yy < 17; yy++ ) {
				int y = yy + y1;
				if( y < minY ) continue;
				if( y > maxY ) break;
				
				for( int zz = 0; zz < 16; zz++ ) {
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( 0 + 1 );
					for( int xx = 0; xx < 16; xx++ ) {
						byte block = chunk.GetBlock( xx, y, zz );
						if( block != 0 ) allAir = false;
						if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
						
						chunkPtr[chunkIndex] = block;
						metaPtr[chunkIndex] = chunk.GetBlockMetadata( xx, y, zz );
						chunkIndex++;
					}
				}
			}
		}
		
		unsafe void CopyXMinus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr, byte* metaPtr ) {
			Chunk chunk = map.GetChunk( ( x1 >> 4 ) - 1, z1 >> 4 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( 0 + 1 ) * 18 + ( -1 + 1 );
				
				for( int zz = 0; zz < 16; zz++ ) {
					
					byte block = chunk.GetBlock( 15, y, zz );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					metaPtr[chunkIndex] = chunk.GetBlockMetadata( 15, y, zz );
					chunkIndex += 18;
				}
			}
		}
		
		unsafe void CopyXPlus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr, byte* metaPtr ) {
			Chunk chunk = map.GetChunk( ( x1 >> 4 ) + 1, z1 >> 4 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( 0 + 1 ) * 18 + ( 16 + 1 );
				
				for( int zz = 0; zz < 16; zz++ ) {
					byte block = chunk.GetBlock( 0, y, zz );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					metaPtr[chunkIndex] = chunk.GetBlockMetadata( 0, y, zz );
					chunkIndex += 18;
				}
			}
		}
		
		unsafe void CopyZMinus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr, byte* metaPtr ) {
			Chunk chunk = map.GetChunk( x1 >> 4, ( z1 >> 4 ) - 1 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( -1 + 1 ) * 18 + ( 0 + 1 );
				for( int xx = 0; xx < 16; xx++ ) {
					
					byte block = chunk.GetBlock( xx, y, 15 );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					metaPtr[chunkIndex] = chunk.GetBlockMetadata( xx, y, 15 );
					chunkIndex++;
				}
			}
		}
		
		unsafe void CopyZPlus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr, byte* metaPtr ) {
			Chunk chunk = map.GetChunk( x1 >> 4, ( z1 >> 4 ) + 1 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( 16 + 1 ) * 18 + ( 0 + 1 );
				
				for( int xx = 0; xx < 16; xx++ ) {
					byte block = chunk.GetBlock( xx, y, 0 );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					metaPtr[chunkIndex] = chunk.GetBlockMetadata( xx, y, 0 );
					chunkIndex++;
				}
			}
		}
	}
}