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
			}
		}
		
		public byte GetBlock( int x, int y, int z ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			return Blocks[index];
		}
		
		public byte GetBlockMetadata( int x, int y, int z ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			return Metadata[index];
		}
		
		public void SetBlock( int x, int y, int z, byte value ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			Blocks[index] = value;
		}
		
		public void SetBlockMetadata( int x, int y, int z, byte value ) {
			int index = ( x << 11 ) | ( z << 7 ) | y;
			Metadata[index] = value;
		}
	}
}
