using System;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Imports a map from a dat map file (original minecraft classic map) </summary>
	public sealed class MapDat : IMapFileFormat {
		
		const byte TC_NULL = 0x70, TC_REFERENCE = 0x71;
		const byte TC_CLASSDESC = 0x72, TC_OBJECT = 0x73;
		const byte TC_STRING = 0x74, TC_ARRAY = 0x75;
		const byte TC_ENDBLOCKDATA = 0x78;
		
		public override bool SupportsLoading { get { return true; }  }
		
		BinaryReader reader;		
		public override byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			byte[] map = null;
			width = 0;		
			height = 0;
			length = 0;
			LocalPlayer p = game.LocalPlayer;
			p.SpawnPoint = Vector3.Zero;
			
			using( GZipStream gs = new GZipStream( stream, CompressionMode.Decompress ) ) {
				reader = new BinaryReader( gs );
				ClassDescription obj = ReadData();
				for( int i = 0; i < obj.Fields.Length; i++ ) {
					FieldDescription field = obj.Fields[i];
					if( field.FieldName == "width" ) 
						width = (int)field.Value;
					else if( field.FieldName == "height" )
						length = (int)field.Value;
					else if( field.FieldName == "depth" )
						height = (int)field.Value;
					else if( field.FieldName == "blocks" )
						map = (byte[])field.Value;
					else if( field.FieldName == "xSpawn" )
						p.SpawnPoint.X = (int)field.Value;
					else if( field.FieldName == "ySpawn" )
						p.SpawnPoint.Y = (int)field.Value;
					else if( field.FieldName == "zSpawn" )
						p.SpawnPoint.Z = (int)field.Value;
				}
			}
			return map;
		}
		
		ClassDescription ReadData() {
			if( ReadInt32() != 0x271BB788 || reader.ReadByte() != 0x02 ) {
				throw new InvalidDataException( "Unexpected constant in .lvl file" );
			}
			
			// Java serialization constants
			if( (ushort)ReadInt16() != 0xACED || ReadInt16() != 0x0005 ) {
				throw new InvalidDataException( "Unexpected java serialisation constant(s)." );
			}
			
			byte typeCode = reader.ReadByte();
			if( typeCode != TC_OBJECT ) ParseError( TC_OBJECT, typeCode );
			ClassDescription desc = ReadClassDescription();
			ReadClassData( desc.Fields );
			return desc;
		}	
		
		void ReadClassData( FieldDescription[] fields ) {
			for( int i = 0; i < fields.Length; i++ ) {
				switch( fields[i].Type ) {
					case FieldType.Byte:
						fields[i].Value = reader.ReadByte(); break;
					case FieldType.Float:
						fields[i].Value = ReadInt32(); break; // wrong, but we don't support it anyways
					case FieldType.Integer:
						fields[i].Value = ReadInt32(); break;
					case FieldType.Long:
						fields[i].Value = ReadInt64(); break;
					case FieldType.Boolean:
						fields[i].Value = reader.ReadByte() != 0; break;
					case FieldType.Object:
						if( fields[i].FieldName == "blockMap" ) {
							byte typeCode = reader.ReadByte();
							// Skip all blockMap data with awful hacks
							if( typeCode == TC_OBJECT ) {
								reader.ReadBytes( 315 );
								int count = ReadInt32();
								
								byte[] temp = new byte[17];
								Stream stream = reader.BaseStream;
								for( int j = 0; j < count; j++ ) {
									stream.Read( temp, 0, temp.Length );
								}
								reader.ReadBytes( 152 );
							}
						} break;
					case FieldType.Array:
						ReadArray( ref fields[i] ); break;
				}
			}
		}	
		
		void ReadArray( ref FieldDescription field ) {
			byte typeCode = reader.ReadByte();
			if( typeCode == TC_NULL ) return;
			if( typeCode != TC_ARRAY ) ParseError( TC_ARRAY, typeCode );
			
			ClassDescription desc = ReadClassDescription();
			field.Value = reader.ReadBytes( ReadInt32() );
		}
		
		ClassDescription ReadClassDescription() {
			ClassDescription desc = default( ClassDescription );
			byte typeCode = reader.ReadByte();
			if( typeCode == TC_NULL ) return desc;			
			if( typeCode != TC_CLASSDESC ) ParseError( TC_CLASSDESC, typeCode );
			
			desc.ClassName = ReadString();
			reader.ReadUInt64(); // serial version UID
			reader.ReadByte(); // flags
			FieldDescription[] fields = new FieldDescription[ReadInt16()];
			for( int i = 0; i < fields.Length; i++ ) {
				fields[i] = ReadFieldDescription();
			}
			desc.Fields = fields;
			
			typeCode = reader.ReadByte();
			if( typeCode != TC_ENDBLOCKDATA ) ParseError( TC_ENDBLOCKDATA, typeCode );
			ReadClassDescription(); // super class description
			return desc;
		}
		
		FieldDescription ReadFieldDescription() {
			FieldDescription desc = default( FieldDescription );
			byte fieldType = reader.ReadByte();
			desc.Type = (FieldType)fieldType;
			desc.FieldName = ReadString();
			
			if( desc.Type == FieldType.Array || desc.Type == FieldType.Object ) {
				byte typeCode = reader.ReadByte();
				if( typeCode == TC_STRING )
					desc.ClassName1 = ReadString();
				else if( typeCode == TC_REFERENCE )
					reader.ReadInt32(); // handle
				else
					ParseError( TC_STRING, typeCode );
			}
			return desc;
		}
		
		void ParseError( int expected, int typeCode ) {
			throw new InvalidDataException(
				String.Format( "Expected {0}, got {1}", expected.ToString( "X2" ), typeCode.ToString( "X2" ) ) );
		}
		
		struct ClassDescription {
			public string ClassName;
			public FieldDescription[] Fields;
		}
		
		struct FieldDescription {
			public FieldType Type;
			public string FieldName;
			public string ClassName1;
			public object Value;
		}
		
		enum FieldType {
			Byte = 'B', Float = 'F', Integer = 'I', Long = 'J',
			Boolean = 'Z', Array = '[', Object = 'L',
		}
		
		long ReadInt64() { return IPAddress.HostToNetworkOrder( reader.ReadInt64() ); }
		int ReadInt32() { return IPAddress.HostToNetworkOrder( reader.ReadInt32() ); }
		short ReadInt16() { return IPAddress.HostToNetworkOrder( reader.ReadInt16() ); }
		string ReadString() { return Encoding.ASCII.GetString( reader.ReadBytes( ReadInt16() ) ); }
	}
}