// Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt
using System;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace ClassicalSharp {

	/// <summary> Imports a map from a FCMv3 map file (fCraft server map) </summary>
	public sealed class MapFcm3 : IMapFileFormat {
		
		const uint Identifier = 0x0FC2AF40;
		const byte Revision = 13;
		
		public override bool SupportsLoading { get { return true; }  }

		public override byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			BinaryReader reader = new BinaryReader( stream );
			if( reader.ReadInt32() != Identifier || reader.ReadByte() != Revision ) {
				throw new InvalidDataException( "Unexpected constant in .fcm file" );
			}

			width = reader.ReadInt16();
			height = reader.ReadInt16();
			length = reader.ReadInt16();

			game.LocalPlayer.SpawnPoint.X = reader.ReadInt32() / 32f;
			game.LocalPlayer.SpawnPoint.Y = reader.ReadInt32() / 32f;
			game.LocalPlayer.SpawnPoint.Z = reader.ReadInt32() / 32f;
			game.LocalPlayer.SpawnYaw = (float)Utils.PackedToDegrees( reader.ReadByte() );
			game.LocalPlayer.SpawnPitch = (float)Utils.PackedToDegrees( reader.ReadByte() );

			reader.ReadUInt32(); // date modified
			reader.ReadUInt32(); // date created
			game.Map.Uuid = new Guid( reader.ReadBytes( 16 ) );
			reader.ReadBytes( 26 ); // layer index
			int metaSize = reader.ReadInt32();

			using( DeflateStream ds = new DeflateStream( stream, CompressionMode.Decompress ) ) {
				reader = new BinaryReader( ds );
				for( int i = 0; i < metaSize; i++ ) {
					string group = ReadString( reader );
					string key = ReadString( reader );
					string value = ReadString( reader );
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