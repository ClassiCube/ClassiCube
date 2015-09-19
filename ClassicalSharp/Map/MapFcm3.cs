// Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt
using System;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace ClassicalSharp {

	public sealed class MapFcm3 : IMapFile {
		
		const uint Identifier = 0x0FC2AF40;
		const byte Revision = 13;
		
		public override bool SupportsLoading { 
			get { return true; } 
		}
		
		public override bool SupportsSaving { 
			get { return true; } 
		}

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
			byte yaw = reader.ReadByte();
			byte pitch = reader.ReadByte();

			reader.ReadUInt32(); // date modified
			reader.ReadUInt32(); // date created
			reader.ReadBytes( 16 ); // guid
			reader.ReadBytes( 26 ); // layer index
			int metaSize = reader.ReadInt32();

			using( DeflateStream ds = new DeflateStream( stream, CompressionMode.Decompress ) ) {
				reader = new BinaryReader( ds );
				for( int i = 0; i < metaSize; i++ ) {
					string group = ReadString( reader );
					string key = ReadString( reader );
					string value = ReadString( reader );
					if( group == "CS_Client" ) {
						int valueInt = Int32.Parse( value );
						switch( key ) {
							case "SkyCol":
								game.Map.SetSkyColour( new FastColour( valueInt ) ); break;
							case "CloudsCol":
								game.Map.SetCloudsColour( new FastColour( valueInt ) ); break;	
							case "FogCol":
								game.Map.SetFogColour( new FastColour( valueInt ) ); break;	
								
							case "ClickDist":
								game.LocalPlayer.ReachDistance = valueInt / 32f; break;
							case "SunLight":
								game.Map.SetSunlight( new FastColour( valueInt ) ); break;
							case "ShadowLight":
								game.Map.SetShadowlight( new FastColour( valueInt ) ); break;
								
							case "Weather":
								game.Map.SetWeather( (Weather)valueInt ); break;
							case "SidesBlock":
								game.Map.SetSidesBlock( (Block)valueInt ); break;
							case "EdgeBlock":
								game.Map.SetEdgeBlock( (Block)valueInt ); break;
						}
					}
					Console.WriteLine( group + "," + key + "," + value );
				}
				byte[] blocks = new byte[width * height * length];
				int read = ds.Read( blocks, 0, blocks.Length );
				return blocks;
			}
		}

		public override void Save( Stream stream, Game game ) {
			BinaryWriter writer = new BinaryWriter( stream );
			LocalPlayer p = game.LocalPlayer;
			Map map = game.Map;
			
			writer.Write( Identifier );
			writer.Write( Revision );

			writer.Write( (short)map.Width );
			writer.Write( (short)map.Height );
			writer.Write( (short)map.Length );

			writer.Write( (int)( p.SpawnPoint.X * 32 ) );
			writer.Write( (int)( p.SpawnPoint.Y * 32 ) );
			writer.Write( (int)( p.SpawnPoint.Z * 32 ) );

			writer.Write( (byte)0 ); // spawn R
			writer.Write( (byte)0 ); // spawn L

			writer.Write( (uint)0 ); // date modified
			writer.Write( (uint)0 ); // date created

			writer.Write( new byte[16] ); // map guid
			writer.Write( (byte)1 ); // layer count

			long indexOffset = stream.Position;
			writer.Seek( 25, SeekOrigin.Current );
			writer.Write( 9 ); // meta count

			int compressedLength;
			long offset;
			using( DeflateStream ds = new DeflateStream( stream, CompressionMode.Compress, true ) ) {
				WriteMetadata( ds, game );
				offset = stream.Position;
				
				ds.Write( map.mapData, 0, map.mapData.Length );
				compressedLength = (int)( stream.Position - offset );
			}

			writer.BaseStream.Seek( indexOffset, SeekOrigin.Begin );
			writer.Write( (byte)0 );
			writer.Write( offset );
			writer.Write( compressedLength );
			writer.Write( 0 );
			writer.Write( 1 );
			writer.Write( map.mapData.Length );
		}
		
		static void WriteMetadata( Stream stream, Game game ) {
			BinaryWriter writer = new BinaryWriter( stream );
			LocalPlayer p = game.LocalPlayer;
			Map map = game.Map;
			
			WriteMetadataEntry( writer, "SkyCol", map.SkyCol.ToArgb() );
			WriteMetadataEntry( writer, "CloudsCol", map.CloudsCol.ToArgb() );
			WriteMetadataEntry( writer, "FogCol", map.FogCol.ToArgb() );
			
			WriteMetadataEntry( writer, "ClickDist", (int)( p.ReachDistance * 32 ) );
			WriteMetadataEntry( writer, "SunLight", map.Sunlight.ToArgb() );
			WriteMetadataEntry( writer, "ShadowLight", map.Shadowlight.ToArgb() );
			
			WriteMetadataEntry( writer, "Weather", (int)map.Weather );
			WriteMetadataEntry( writer, "SidesBlock", (int)map.SidesBlock );
			WriteMetadataEntry( writer, "EdgeBlock", (int)map.EdgeBlock );
		}

		static string ReadString( BinaryReader reader ) {
			int length = reader.ReadUInt16();
			byte[] data = reader.ReadBytes( length );
			return Encoding.ASCII.GetString( data );
		}

		static void WriteString( BinaryWriter writer, string str ) {
			byte[] data = Encoding.ASCII.GetBytes( str );
			writer.Write( (ushort)data.Length );
			writer.Write( data );
		}

		static void WriteMetadataEntry( BinaryWriter writer, string key, int value ) {
			WriteString( writer, "CS_Client" );
			WriteString( writer, key );
			WriteString( writer, value.ToString() );
		}
	}
}