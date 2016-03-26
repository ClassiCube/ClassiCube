// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Text;

namespace ClassicalSharp {

	public sealed partial class MapCw : IMapFileFormat {
		
		internal struct NbtTag {
			public string Name;
			public object Value;
			public NbtTagType TagId;
		}
		
		class NbtList {
			public NbtTagType ChildTagId;
			public object[] ChildrenValues;
		}		
		
		internal enum NbtTagType : byte {
			End, Int8, Int16, Int32, Int64, 
			Real32, Real64, Int8Array, String, 
			List, Compound, Int32Array,
			Invalid = 255,
		}
		
		
		void WriteTag( NbtTagType v ) { writer.Write( (byte)v ); }
		
		void WriteInt64( long v ) { writer.Write( IPAddress.HostToNetworkOrder( v ) ); }
		
		void WriteInt32( int v ) { writer.Write( IPAddress.HostToNetworkOrder( v ) ); }
		
		void WriteInt16( short v ) { writer.Write( IPAddress.HostToNetworkOrder( v ) ); }
		
		void WriteInt8( sbyte v ) { writer.Write( (byte)v ); }
		
		void WriteUInt8( int v ) { writer.Write( (byte)v ); }
		
		void WriteUInt8( byte v ) { writer.Write( v ); }
		
		void WriteBytes( byte[] v ) { writer.Write( v ); }
		
		void WriteString( string value ) {
			ushort len = (ushort)value.Length;
			byte[] data = Encoding.UTF8.GetBytes( value );
			WriteInt16( (short)len );
			writer.Write( data );
		}
		
		void WriteCpeExtCompound( string name, int version ) {
			WriteTag( NbtTagType.Compound ); WriteString( name );
			
			WriteTag( NbtTagType.Int32 );
			WriteString( "ExtensionVersion" ); WriteInt32( version );
		}
		
		
		long ReadInt64() { return IPAddress.HostToNetworkOrder( reader.ReadInt64() ); }
		
		int ReadInt32() { return IPAddress.HostToNetworkOrder( reader.ReadInt32() ); }
		
		short ReadInt16() { return IPAddress.HostToNetworkOrder( reader.ReadInt16() ); }
		
		string ReadString() {
			int len = (ushort)ReadInt16();
			byte[] data = reader.ReadBytes( len );
			return Encoding.UTF8.GetString( data ); 
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
					tag.Value = *((float*)&temp32); break;
				case NbtTagType.Real64:
					long temp64 = ReadInt64();
					tag.Value = *((double*)&temp64); break;
				case NbtTagType.Int8Array:
					tag.Value = reader.ReadBytes( ReadInt32() ); break;
				case NbtTagType.String:
					tag.Value = ReadString(); break;
					
				case NbtTagType.List:
					NbtList list = new NbtList();
					list.ChildTagId = (NbtTagType)reader.ReadByte();
					list.ChildrenValues = new object[ReadInt32()];
					for( int i = 0; i < list.ChildrenValues.Length; i++ )
						list.ChildrenValues[i] = ReadTag( (byte)list.ChildTagId, false ).Value;
					tag.Value = list; break;
					
				case NbtTagType.Compound:
					Dictionary<string, NbtTag> children = new Dictionary<string, NbtTag>();
					NbtTag child;
					while( (child = ReadTag( reader.ReadByte(), true )).TagId != NbtTagType.Invalid )
						children[child.Name] = child;
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
	}
}