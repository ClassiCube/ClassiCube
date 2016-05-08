// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;
using System.IO.Compression;
using ClassicalSharp.Entities;

namespace ClassicalSharp.Map {

	/// <summary> Exports a world to a LVL map file (MCLawl server map) </summary>
	public sealed class MapLvlExporter : IMapFormatExporter {

		const int Version = 1874;
		const byte customTile = 163;
		
		public void Save( Stream stream, Game game ) {
			World map = game.World;
			LocalPlayer p = game.LocalPlayer;
			
			using( DeflateStream gs = new DeflateStream( stream, CompressionMode.Compress ) ) {
				BinaryWriter w = new BinaryWriter( gs );
				
				w.Write( (ushort)Version );
				w.Write( (ushort)map.Width );
				w.Write( (ushort)map.Length );
				w.Write( (ushort)map.Height );
				
				w.Write( (ushort)p.Spawn.X );
				w.Write( (ushort)p.Spawn.Z );
				w.Write( (ushort)p.Spawn.Y );
				w.Write( Utils.DegreesToPacked( p.SpawnYaw ) );
				w.Write( Utils.DegreesToPacked( p.SpawnPitch ) );
				
				w.Write( (ushort)0 ); // pervisit and perbuild perms
				WriteBlocks( map.mapData, w );
				
				w.Write( (byte)0xBD );			
				WriteCustomBlocks( map, w );
			}
		}
		
		void WriteBlocks( byte[] blocks, BinaryWriter w ) {
			const int bufferSize = 64 * 1024;
			byte[] buffer = new byte[bufferSize];
			int bIndex = 0;
			
			for( int i = 0; i < blocks.Length; i++ ) {
				byte block = blocks[i];
				buffer[bIndex] = block >= BlockInfo.CpeBlocksCount ? customTile : block;

				bIndex++;
				if( bIndex == bufferSize ) {
					w.Write( buffer, 0, bufferSize ); bIndex = 0;
				}
			}
			if( bIndex > 0 ) w.Write( buffer, 0, bIndex );
		}
		
		void WriteCustomBlocks( World map, BinaryWriter w ) {
			byte[] chunk = new byte[16 * 16 * 16];
			
			for( int y = 0; y < map.Height; y += 16 )
				for( int z = 0; z < map.Length; z += 16 )
					for( int x = 0; x < map.Width; x += 16 )
			{
				w.Write( (byte)0 );
			}
		}
	}
}