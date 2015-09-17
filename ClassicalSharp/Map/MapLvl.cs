using System;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace ClassicalSharp {

	public sealed class MapLvl {
		const uint Identifier = 0x88B71B27;
		const byte Revision = 2;
		const ushort streamMagic = 0xEDAC;
		const ushort streamVersion = 0x0500;

		public byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			using( GZipStream decompressed = new GZipStream( stream, CompressionMode.Decompress ) ) {
				BinaryReader reader = new BinaryReader( decompressed );
				if( reader.ReadUInt32() != Identifier || reader.ReadByte() != Revision ) {
					throw new InvalidDataException( "Unexpected constant in .lvl file" );
				}
				
				// Java serialization constants
				if( reader.ReadUInt16() != streamMagic || reader.ReadUInt16() != streamVersion ) {
					throw new InvalidDataException( "Unexpected java serialisation constant(s)." );
				}
				
				int typeCode = 0;
				while( ( typeCode = decompressed.ReadByte() ) != -1 ) {
					HandleTypeCode( typeCode, reader );
				}
			}
			width = 0;
			length = 0;
			height = 0;
			return null;
		}
		
		void HandleTypeCode( int typeCode, BinaryReader reader ) {
			switch( typeCode ) {
					
				case 0x42:
					ReadPrimField( reader, FieldType.Byte ); break;
				case 0x43:
					ReadPrimField( reader, FieldType.Char ); break;
				case 0x44:
					ReadPrimField( reader, FieldType.Double ); break;
				case 0x46:
					ReadPrimField( reader, FieldType.Float ); break;
				case 0x49:
					ReadPrimField( reader, FieldType.Integer ); break;
				case 0x4A:
					ReadPrimField( reader, FieldType.Long ); break;
				case 0x53:
					ReadPrimField( reader, FieldType.Short ); break;
				case 0x5A:
					ReadPrimField( reader, FieldType.Boolean ); break;
					
				case 0x4C:
					ReadPrimField( reader, FieldType.Object ); break;
				case 0x5B:
					ReadPrimField( reader, FieldType.Array ); break;					
					
				case 0x70: // TC_NULL
					break;
					
				case 0x71: // TC_REFERENCE
					reader.ReadInt32(); // handle
					break;
					
				case 0x72: // TC_CLASSDESC
					{
						string className = ReadString( reader );
						reader.ReadUInt64(); // serial identifier
						reader.ReadByte(); // various flags
						fields = new Field[(reader.ReadByte() << 8) | reader.ReadByte()];
					} break;
					
				case 0x73: // TC_OBJECT
					break;
					
				case 0x74: // TC_STRING
					string value = ReadString( reader );
					break;
					
				case 0x75: // TC_ARRAY
					break;
					
				case 0x76: // TC_CLASS
					break;
					
				case 0x77: // TC_BLOCKDATA
					break;
					
				case 0x78: // TC_ENDBLOCKDATA
					break;
					
				case 0x79: // TC_RESET
					break;
					
				case 0x7A: // TC_BLOCKDATALONG
					break;
					
				case 0x7B: // TC_EXCEPTION
					break;
					
				case 0x7C: // TC_LONGSTRING
					break;
					
				case 0x7D: // TC_PROXYCLASSDESC
					break;
					
				case 0x7E: // TC_ENUM
					break;
					
			}
		}
		
		enum FieldType {
			Byte, Char, Double, Float, Integer, Long,
			Short, Boolean, Object, Array,
		}
		
		struct Field {
			public string Name;
			public FieldType Type;
		}
		
		static string ReadString( BinaryReader reader ) {
			int len = ( reader.ReadByte() << 8 ) | reader.ReadByte();
			byte[] data = reader.ReadBytes( len );
			return Encoding.ASCII.GetString( data );
		}
		
		Field[] fields;
		int fieldIndex;
		void ReadPrimField( BinaryReader reader, FieldType type ) {
			Field field;
			field.Name = ReadString( reader );
			field.Type = type;
			fields[fieldIndex++] = field;
		}
	}
}