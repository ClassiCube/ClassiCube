// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Text;

namespace ClassicalSharp.Map {
	
	public struct NbtTag {
		public string Name;
		public object Value;
		public NbtTagType TagId;
	}
	
	public class NbtList {
		public NbtTagType ChildTagId;
		public object[] ChildrenValues;
	}
	
	public enum NbtTagType : byte {
		End, Int8, Int16, Int32, Int64,
		Real32, Real64, Int8Array, String,
		List, Compound, Int32Array,
		Invalid = 255,
	}

	public sealed class NbtFile {
		
		readonly BinaryReader reader;
		readonly BinaryWriter writer;
		
		public NbtFile(BinaryReader reader) {
			this.reader = reader;
		}
		
		public NbtFile(BinaryWriter writer) {
			this.writer = writer;
		}
		
		
		public void Write(NbtTagType v) { writer.Write((byte)v); }
		
		public void WriteInt32(int v) { writer.Write(IPAddress.HostToNetworkOrder(v)); }
		
		public void WriteInt16(short v) { writer.Write(IPAddress.HostToNetworkOrder(v)); }
		
		public void WriteUInt8(int v) { writer.Write((byte)v); }
		
		public void WriteUInt8(byte v) { writer.Write(v); }
		
		public void WriteBytes(byte[] v) { writer.Write(v); }
		
		public void WriteBytes(byte[] v, int index, int count) { writer.Write(v, index, count); }
		
		public void Write(string value) {
			ushort len = (ushort)value.Length;
			byte[] data = Encoding.UTF8.GetBytes(value);
			WriteInt16((short)len);
			writer.Write(data);
		}
		
		public void WriteCpeExtCompound(string name, int version) {
			Write(NbtTagType.Compound); Write(name);
			
			Write(NbtTagType.Int32);
			Write("ExtensionVersion"); WriteInt32(version);
		}
		
		
		public long ReadInt64() { return IPAddress.HostToNetworkOrder(reader.ReadInt64()); }
		
		public int ReadInt32() { return IPAddress.HostToNetworkOrder(reader.ReadInt32()); }
		
		public short ReadInt16() { return IPAddress.HostToNetworkOrder(reader.ReadInt16()); }
		
		public string ReadString() {
			int len = (ushort)ReadInt16();
			byte[] data = reader.ReadBytes(len);
			return Encoding.UTF8.GetString(data);
		}
		
		public unsafe NbtTag ReadTag(byte typeId, bool readTagName) {
			NbtTag tag = default(NbtTag);
			if (typeId == 0) {
				tag.TagId = NbtTagType.Invalid; return tag;
			}
			
			tag.Name = readTagName ? ReadString() : null;
			tag.TagId = (NbtTagType)typeId;			
			switch ((NbtTagType)typeId) {
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
					tag.Value = reader.ReadBytes(ReadInt32()); break;
				case NbtTagType.String:
					tag.Value = ReadString(); break;
					
				case NbtTagType.List:
					NbtList list = new NbtList();
					list.ChildTagId = (NbtTagType)reader.ReadByte();
					list.ChildrenValues = new object[ReadInt32()];
					for (int i = 0; i < list.ChildrenValues.Length; i++) {
						list.ChildrenValues[i] = ReadTag((byte)list.ChildTagId, false).Value;
					}
					tag.Value = list; break;
					
				case NbtTagType.Compound:
					Dictionary<string, NbtTag> children = new Dictionary<string, NbtTag>();
					NbtTag child;
					while ((child = ReadTag(reader.ReadByte(), true)).TagId != NbtTagType.Invalid) {
						children[child.Name] = child;
					}
					tag.Value = children; break;
					
				case NbtTagType.Int32Array:
					int[] array = new int[ReadInt32()];
					for (int i = 0; i < array.Length; i++) {
						array[i] = ReadInt32();
					}
					tag.Value = array; break;
					
				default:
					throw new InvalidDataException("Unrecognised tag id: " + typeId);
			}
			return tag;
		}
	}
}