// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt
using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using ClassicalSharp.Entities;

namespace ClassicalSharp.Map {

	/// <summary> Imports a world from a FCMv3 map file (fCraft server map) </summary>
	public sealed class MapFcm3Importer : IMapFormatImporter {
		
		const uint Identifier = 0x0FC2AF40;
		const byte Revision = 13;

		public byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			BinaryReader r = new BinaryReader( stream );
			if( r.ReadInt32() != Identifier || r.ReadByte() != Revision )
				throw new InvalidDataException( "Unexpected constant in .fcm file" );

			width = r.ReadInt16();
			height = r.ReadInt16();
			length = r.ReadInt16();

			LocalPlayer p = game.LocalPlayer;
			p.Spawn.X = r.ReadInt32() / 32f;
			p.Spawn.Y = r.ReadInt32() / 32f;
			p.Spawn.Z = r.ReadInt32() / 32f;
			p.SpawnYaw = (float)Utils.PackedToDegrees( r.ReadByte() );
			p.SpawnPitch = (float)Utils.PackedToDegrees( r.ReadByte() );

			r.ReadUInt32(); // date modified
			r.ReadUInt32(); // date created
			game.World.Uuid = new Guid( r.ReadBytes( 16 ) );
			r.ReadBytes( 26 ); // layer index
			int metaSize = r.ReadInt32();

			using( DeflateStream ds = new DeflateStream( stream, CompressionMode.Decompress ) ) {
				r = new BinaryReader( ds );
				for( int i = 0; i < metaSize; i++ ) {
					string group = ReadString( r );
					string key = ReadString( r );
					string value = ReadString( r );
				}
				
				byte[] blocks = new byte[width * height * length];
				int read = ds.Read( blocks, 0, blocks.Length );
				return blocks;
			}
		}
		
		static string ReadString( BinaryReader reader ) {
			int length = reader.ReadUInt16();
			byte[] data = reader.ReadBytes( length );
			return Encoding.ASCII.GetString( data );
		}
	}
}