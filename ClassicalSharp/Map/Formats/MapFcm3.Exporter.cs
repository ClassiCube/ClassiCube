// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt
using System;
using System.IO;
using System.IO.Compression;
using ClassicalSharp.Entities;

namespace ClassicalSharp.Map {
	
	/// <summary> Exports a world to a FCMv3 map file (fCraft server map) </summary>
	public sealed class MapFcm3Exporter : IMapFormatExporter {
		
		const uint Identifier = 0x0FC2AF40;
		const byte Revision = 13;
		const int chunkSize = 128 * 128 * 128;

		public void Save( Stream stream, Game game ) {
			World map = game.World;
			LocalPlayer p = game.LocalPlayer;
			BinaryWriter w = new BinaryWriter( stream );
			w.Write( Identifier );
			w.Write( Revision );
			
			w.Write( (short)map.Width );
			w.Write( (short)map.Height );
			w.Write( (short)map.Length );

			w.Write( (int)(p.Spawn.X * 32) );
			w.Write( (int)(p.Spawn.Y * 32) );
			w.Write( (int)(p.Spawn.Z * 32) );

			w.Write( Utils.DegreesToPacked( p.SpawnYaw ) );
			w.Write( Utils.DegreesToPacked( p.SpawnPitch ) );

			w.Write( 0 ); // Date modified
			w.Write( 0 ); // Date created

			w.Write( map.Uuid.ToByteArray() );
			w.Write( (byte)1 ); // layer count

			// skip over index and metacount
			long indexOffset = stream.Position;
			w.Seek( 29, SeekOrigin.Current );
			long offset = stream.Position;
			byte[] blocks = map.mapData;
			
			using( DeflateStream ds = new DeflateStream( stream, CompressionMode.Compress, true ) ) {
				for( int i = 0; i < blocks.Length; i += chunkSize ) {
					int len = Math.Min( chunkSize, blocks.Length - i );
					ds.Write( blocks, i, len );
				}
			}
			int compressedLength = (int)(stream.Position - offset);

			// come back to write the index
			w.BaseStream.Seek( indexOffset, SeekOrigin.Begin );

			w.Write( (byte)0 );            // data layer type (Blocks)
			w.Write( offset );             // offset, in bytes, from start of stream
			w.Write( compressedLength );   // compressed length, in bytes
			w.Write( 0 );                  // general purpose field
			w.Write( 1 );                  // element size
			w.Write( map.mapData.Length ); // element count

			w.Write( 0 ); // No metadata
		}
	}
}