// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using ClassicalSharp.Entities;
using ClassicalSharp.Net;

namespace ClassicalSharp.Map {

	/// <summary> Exports a world to a LVL map file (MCLawl server map) </summary>
	public sealed class MapLvlExporter : IMapFormatExporter {

		const int Version = 1874;
		const byte customTile = 163;
		
		public void Save( Stream stream, Game game ) {
			World map = game.World;
			LocalPlayer p = game.LocalPlayer;
			const int chunkSize = 128 * 128 * 128;
			
			using( DeflateStream gs = new DeflateStream( stream, CompressionMode.Compress ) ) {
				BinaryWriter writer = new BinaryWriter( gs );
				
				writer.Write( (ushort)Version );
				writer.Write( (ushort)map.Width );
				writer.Write( (ushort)map.Length );
				writer.Write( (ushort)map.Height );
				
				writer.Write( (ushort)p.Spawn.X );
				writer.Write( (ushort)p.Spawn.Z );
				writer.Write( (ushort)p.Spawn.Y );
				writer.Write( Utils.DegreesToPacked( p.SpawnYaw ) );
				writer.Write( Utils.DegreesToPacked( p.SpawnPitch ) );
				
				writer.Write( (ushort)0 ); // pervisit and perbuild perms
				WriteBlocks( map.mapData, writer );
				
				writer.Write( (byte)0xBD );			
				WriteCustomBlocks( map, writer );
			}
		}
		
		void WriteBlocks( byte[] blocks, BinaryWriter writer ) {
			const int bufferSize = 64 * 1024;
			byte[] buffer = new byte[bufferSize];
			int bIndex = 0;
			
			for( int i = 0; i < blocks.Length; i++ ) {
				byte block = blocks[i];
				buffer[bIndex] = block >= BlockInfo.CpeBlocksCount ? customTile : block;

				bIndex++;
				if( bIndex == bufferSize ) {
					writer.Write( buffer, 0, bufferSize ); bIndex = 0;
				}
			}
			if( bIndex > 0 ) writer.Write( buffer, 0, bIndex );
		}
		
		void WriteCustomBlocks( World map, BinaryWriter writer ) {
			byte[] chunk = new byte[16 * 16 * 16];
			
			for( int y = 0; y < map.Height; y += 16 )
				for( int z = 0; z < map.Length; z += 16 )
					for( int x = 0; x < map.Width; x += 16 )
			{
				writer.Write( (byte)0 );
			}
		}
	}
}