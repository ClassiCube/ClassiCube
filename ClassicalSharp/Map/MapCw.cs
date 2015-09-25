using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;
using System.Net;

namespace ClassicalSharp {

	public sealed class MapCw : IMapFile {
		
		public override bool SupportsLoading {
			get { return true; }
		}
		
		public override bool SupportsSaving {
			get { return true; }
		}

		BinaryReader reader;
		NbtTag invalid = default( NbtTag );
		public override byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			using( GZipStream wrapper = new GZipStream( stream, CompressionMode.Decompress ) ) {
				reader = new BinaryReader( wrapper );
				if( reader.ReadByte() != (byte)NbtTagType.Compound )
					throw new InvalidDataException( "Nbt file must start with Tag_Compound" );
				
				invalid.TagId = NbtTagType.Invalid;
				NbtTag root = ReadTag( (byte)NbtTagType.Compound, true );
			}
			length = width = height = 0;
			return null;
		}
		
		unsafe NbtTag ReadTag( byte typeId, bool readTagName ) {
			if( typeId == 0 ) return invalid;
			
			NbtTag tag = default( NbtTag );
			tag.Name = readTagName ? ReadString() : null;
			tag.TagId = (NbtTagType)typeId;
			
			switch( (NbtTagType)typeId ) {
				case NbtTagType.Int8:
					tag.Value = reader.ReadByte(); break;
				case NbtTagType.Int16:
					tag.Value = ReadInt16(); break;
				case NbtTagType.Int32:
					tag.Value = ReadInt32(); break;
				case NbtTagType.Int64:
					tag.Value = ReadInt64(); break;
				case NbtTagType.Real32:
					int temp32 = ReadInt32();
					tag.Value = *(float*)&temp32; break;
				case NbtTagType.Real64:
					long temp64 = ReadInt64();
					tag.Value = *(double*)&temp64; break;
				case NbtTagType.Int8Array:
					tag.Value = reader.ReadBytes( ReadInt32() ); break;
				case NbtTagType.String:
					tag.Value = ReadString(); break;
					
				case NbtTagType.List:
					NbtList list = new NbtList();
					list.ChildTagId = (NbtTagType)reader.ReadByte();
					list.ChildrenValues = new object[ReadInt32()];
					for( int i = 0; i < list.ChildrenValues.Length; i++ ) {
						list.ChildrenValues[i] = ReadTag( (byte)list.ChildTagId, false );
					}
					tag.Value = list; break;
					
				case NbtTagType.Compound:
					Dictionary<string, NbtTag> children = new Dictionary<string, NbtTag>();
					NbtTag child;
					while( (child = ReadTag( reader.ReadByte(), true )).TagId != NbtTagType.Invalid ) {
						children[child.Name] = child;
					}
					tag.Value = children; break;
					
				case NbtTagType.Int32Array:
					int[] array = new int[ReadInt32()];
					for( int i = 0; i < array.Length; i++ )
						array[i] = ReadInt32();
					tag.Value = array; break;
					
				default:
					throw new InvalidDataException( "Unrecognised tag id: " + typeId );
			}
			return tag;
		}
		
		struct NbtTag {
			public string Name;
			public object Value;
			public NbtTagType TagId;
		}
		
		class NbtList {
			public NbtTagType ChildTagId;
			public object[] ChildrenValues;
		}
		
		
		enum NbtTagType : byte {
			End = 0,
			Int8 = 1,
			Int16 = 2,
			Int32 = 3,
			Int64 = 4,
			Real32 = 5,
			Real64 = 6,
			Int8Array = 7,
			String = 8,
			List = 9,
			Compound = 10,
			Int32Array = 11,
			Invalid = 255,
		}
		
		long ReadInt64() { return IPAddress.HostToNetworkOrder( reader.ReadInt64() ); }
		int ReadInt32() { return IPAddress.HostToNetworkOrder( reader.ReadInt32() ); }
		short ReadInt16() { return IPAddress.HostToNetworkOrder( reader.ReadInt16() ); }
		string ReadString() { return Encoding.UTF8.GetString( reader.ReadBytes( (ushort)ReadInt16() ) ); }

		public override void Save( Stream stream, Game game ) {
		}
	}
}