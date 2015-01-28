using System;
using ClassicalSharp.Util;

namespace ClassicalSharp.World {
	
	public partial class Chunk {
		
		public int ChunkX;
		
		public int ChunkZ;
		
		public byte[] Blocks;
		public NibbleArray Metadata;
		public NibbleArray BlockLight;
		public NibbleArray SkyLight;
		sbyte[] heightmap;
		Game game;
		
		public const int Width = 16, Height = 128, Length = 16;
		
		public Chunk( int chunkX, int chunkZ, bool initData, Game game ) {
			ChunkX = chunkX;
			ChunkZ = chunkZ;
			this.game = game;
			
			if( initData ) {
				Blocks = new byte[Width * Height * Length];
				Metadata = new NibbleArray( Width * Height * Length );
				BlockLight = new NibbleArray( Width * Height * Length );
				SkyLight = new NibbleArray( Width * Height * Length );
				heightmap = new sbyte[Width * Length];
				for( int i = 0; i < heightmap.Length; i++ ) {
					heightmap[i] = sbyte.MinValue;
				}
			}
		}
		
		public byte GetBlock( int x, int y, int z ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			return Blocks[index];
		}
		
		public byte GetBlockMetadata( int x, int y, int z ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			return Blocks[index];
		}
		
		public void SetBlock( int x, int y, int z, byte value ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			Blocks[index] = value;
		}
		
		
		public int GetLightHeight( int x,  int z ) {
			int index = ( x * Length ) + z;
			int height = heightmap[index];
			
			if( height == sbyte.MinValue ) {
				height = CalcHeightAt( x, Height - 1, z, index );
			}
			return height;
		}
		
		int CalcHeightAt( int x, int maxY, int z, int index ) {
			heightmap[index] = -1;
			for( int y = maxY; y >= 0; y-- ) {
				byte block = GetBlock( x, y, z );
				if( game.BlockInfo.BlocksLight( block ) ) {
					heightmap[index] = (sbyte)y;
					return y;
				}
			}
			return -1;
		}
		
		internal void UpdateHeight( int x, int y, int z, byte oldBlock, byte newBlock ) {
			bool didBlock = game.BlockInfo.BlocksLight( oldBlock );
			bool nowBlocks = game.BlockInfo.BlocksLight( newBlock );
			if( didBlock && nowBlocks || !didBlock && !nowBlocks ) return;
			
			int index = ( x * Length ) + z;
			if( nowBlocks ) {
				if( y > GetLightHeight( x, z ) ) {
					heightmap[index] = (sbyte)y;
				}
			} else {
				if( y >= GetLightHeight( x, z ) ) {
					CalcHeightAt( x, y, z, index );
				}
			}
		}
	}
}
