// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt
using System;
using System.IO;
using System.IO.Compression;
using System.Text;
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
			BinaryWriter writer = new BinaryWriter( stream );
			writer.Write( Identifier );
			writer.Write( Revision );
			
			writer.Write( (short)map.Width );
			writer.Write( (short)map.Height );
			writer.Write( (short)map.Length );

			writer.Write( (int)(p.Spawn.X * 32) );
			writer.Write( (int)(p.Spawn.Y * 32) );
			writer.Write( (int)(p.Spawn.Z * 32) );

			writer.Write( Utils.DegreesToPacked( p.SpawnYaw ) );
			writer.Write( Utils.DegreesToPacked( p.SpawnPitch ) );

			writer.Write( 0 ); // Date modified
			writer.Write( 0 ); // Date created

			writer.Write( map.Uuid.ToByteArray() );
			writer.Write( (byte)1 ); // layer count

			// skip over index and metacount
			long indexOffset = stream.Position;
			writer.Seek( 29, SeekOrigin.Current );
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
			writer.BaseStream.Seek( indexOffset, SeekOrigin.Begin );

			writer.Write( (byte)0 );            // data layer type (Blocks)
			writer.Write( offset );             // offset, in bytes, from start of stream
			writer.Write( compressedLength );   // compressed length, in bytes
			writer.Write( 0 );                  // general purpose field
			writer.Write( 1 );                  // element size
			writer.Write( map.mapData.Length ); // element count

			writer.Write( 0 ); // No metadata
		}
	}
}