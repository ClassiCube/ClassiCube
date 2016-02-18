using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {

		unsafe void CastShadows( int X, int Y, int Z ) {
			for( int yy = 0; yy < 16; yy++ )
				for( int zz = 0; zz < 16; zz++ )
					for( int xx = 0; xx < 16; xx++ )
			{
				int index = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (xx + 1);
				byte block = chunk[index];
				if( block == 0 || !info.BlocksLight[block] ) continue;
				
				RayCast( X + xx + 1, Y + yy - 1, Z + zz + 0, 1 );
				RayCast( X + xx + 0, Y + yy - 1, Z + zz + 1, 2 );
				RayCast( X + xx + 1, Y + yy - 1, Z + zz + 1, 3 );
			}
		}
		
		void RayCast( int x, int y, int z, byte flags ) {
			bool debug = y == 14;
			if( debug ) Console.WriteLine( "CAST SRC " + x + "," + z );
			while( x < width && z < length && y >= 0 && y < height ) {
				byte block = y == (height - 1) ? (byte)0 : map.GetBlock( x, y + 1, z );
				if( info.BlocksLight[block] && block != 0 ) return;
				if( debug ) Console.WriteLine( "Doing: " + x + "," + y + "," + z + " : " + flags );
				
				block = map.GetBlock( x, y, z );
				if( info.BlocksLight[block] && block != 0 ) {
					int index = x + width * (z + y * length);
					map.mapShadows[index] |= flags;
					if( debug ) Console.WriteLine( "=== fin ===" + ":" + index + "," + width + "," + length );
					return;
				}
				x++; z++; y--;
			}
			if( debug ) Console.WriteLine( "=== xxx ===" );
		}
	}
}