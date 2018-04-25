#if FALSEEE
// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;
using ClassicalSharp.Entities;
using ClassicalSharp.Network;
using OpenTK;

namespace ClassicalSharp.Map {
	
	/// <summary> Imports a world from a dat map file (original minecraft classic map) </summary>
	public sealed class MapDat2Importer : IMapFormatImporter {
		
		const byte TC_NULL = 0x70;
		const byte TC_REFERENCE = 0x71;
		const byte TC_CLASSDESC = 0x72;
		const byte TC_OBJECT = 0x73;
		const byte TC_STRING = 0x74;
		const byte TC_ARRAY = 0x75;
		const byte TC_CLASS = 0x76;
		const byte TC_BLOCKDATA = 0x77;
		const byte TC_ENDBLOCKDATA = 0x78;
		const byte TC_RESET = 0x79;
		const byte TC_BLOCKDATALONG = 0x7A;
		const byte TC_LONGSTRING = 0x7C;
		const byte TC_PROXYCLASSDESC = 0x7D;
		const byte TC_ENUM = 0x7E;
		const int baseWireHandle = 0x7E0000;

		const byte SC_WRITE_METHOD = 0x01, SC_SERIALIZABLE = 0x02;
		
		byte ReadUInt8() { return reader.ReadByte(); }
		short ReadInt16() { return IPAddress.HostToNetworkOrder(reader.ReadInt16()); }
		ushort ReadUInt16() { return (ushort)IPAddress.HostToNetworkOrder(reader.ReadInt16()); }
		int ReadInt32() { return IPAddress.HostToNetworkOrder(reader.ReadInt32()); }
		long ReadInt64() { return IPAddress.HostToNetworkOrder(reader.ReadInt64()); }
		string ReadUtf8() { return Encoding.UTF8.GetString(reader.ReadBytes(ReadUInt16())); }
		BinaryReader reader;	
		List<object> handles = new List<object>();
		
		public byte[] Load(Stream stream, Game game, out int width, out int height, out int length) {
			byte[] map = null;
			width = 0;
			height = 0;
			length = 0;
			GZipHeaderReader gsHeader = new GZipHeaderReader();
			while (!gsHeader.ReadHeader(stream)) { }
			LocalPlayer p = game.LocalPlayer;
			p.Spawn = Vector3.Zero;
			
			using (DeflateStream gs = new DeflateStream(stream, CompressionMode.Decompress)) {
				reader = new BinaryReader(gs);
				if (ReadInt32() != 0x271BB788 || reader.ReadByte() != 0x02) {
					throw new InvalidDataException("Unexpected constant in .dat file");
				}
				
				JObject obj = (JObject)ReadStream();
				JFieldDesc[] fields = obj.Desc.Info.Fields;
				object[] values = obj.ClassData[0].Values;
				
				for (int i = 0; i < fields.Length; i++) {
					JFieldDesc field = fields[i];
					object value = values[i];
					
					if (field.Name == "width")
						width = (int)value;
					else if (field.Name == "height")
						length = (int)value;
					else if (field.Name == "depth")
						height = (int)value;
					else if (field.Name == "blocks")
						map = (byte[])((JArray)value).Values;
					else if (field.Name == "xSpawn")
						p.Spawn.X = (int)value;
					else if (field.Name == "ySpawn")
						p.Spawn.Y = (int)value;
					else if (field.Name == "zSpawn")
						p.Spawn.Z = (int)value;
				}
			}
			return map;
		}
		
		object ReadStream() {
			if (ReadUInt16() != 0xACED) throw new InvalidDataException("Invalid stream magic");
			if (ReadUInt16() != 0x0005) throw new InvalidDataException("Invalid stream version");
			
			byte typeCode = ReadUInt8();
			return ReadContent(typeCode);
		}
		
		object ReadContent(byte typeCode) {
			if (typeCode == TC_BLOCKDATA) {
				return reader.ReadBytes(ReadUInt8());
			} else if (typeCode == TC_BLOCKDATALONG) {
				return reader.ReadBytes(ReadInt32());
			} else {
				return ReadObject(typeCode);
			}
		}
		
		object ReadObject() { return ReadObject(ReadUInt8()); }
		object ReadObject(byte typeCode) {
			switch (typeCode) {
				case TC_STRING:    return NewString();
				case TC_RESET: handles.Clear(); return null;
				case TC_NULL:      return null;
				case TC_REFERENCE: return PrevObject();
				case TC_CLASS:     return NewClass();
				case TC_ENUM:      return NewEnum();
				case TC_OBJECT:    return NewObject();
				case TC_ARRAY:     return NewArray();
				case TC_CLASSDESC: return NewClassDesc();
			}
			throw new InvalidDataException("Invalid typecode: " + typeCode);
		}
		
		string NewString() {
			string value = ReadUtf8();
			handles.Add(value);
			return value;
		}
		
		object PrevObject() {
			int handle = ReadInt32() - baseWireHandle;
			if (handle >= 0 && handle < handles.Count) return handles[handle];
			throw new InvalidDataException("Invalid stream handle: " + handle);
		}
		
		JClassDesc NewClass() {
			JClassDesc classDesc = ClassDesc();
			handles.Add(classDesc);
			return classDesc;
		}
		
		class JEnum { public JClassDesc Desc; public string ConstName; }
		JEnum NewEnum() {
			JEnum value = new JEnum();
			value.Desc = ClassDesc();
			handles.Add(value);
			value.ConstName = (string)ReadObject();
			return value;
		}
		
		class JObject { public JClassDesc Desc; public List<JClassData> ClassData; }
		JObject NewObject() {
			JObject obj = new JObject();
			obj.Desc = ClassDesc();
			handles.Add(obj);
			
			List<JClassDesc> classDescs = new List<JClassDesc>();
			List<JClassData> classDatas = new List<JClassData>();
			JClassDesc tmp = obj.Desc;
			
			// most superclass data is first
			while (tmp != null) { 
				classDescs.Add(tmp);
				tmp = tmp.Info.SuperClass;
			}
			classDescs.Reverse();
			
			for (int i = 0; i < classDescs.Count; i++) {
				JClassData classData = ClassData(classDescs[i].Info);
				classDatas.Add(classData);
			}
			
			// reverse order so least superclass is first
			classDatas.Reverse();
			obj.ClassData = classDatas;
			return obj;
		}
		
		class JArray { public JClassDesc Desc; public object Values; }
		JArray NewArray() {
			JArray array = new JArray();
			array.Desc = ClassDesc();
			handles.Add(array);
			char type = array.Desc.Name[1];
			int size = ReadInt32();
			
			if (type == 'B') {
				array.Values = reader.ReadBytes(size);
			} else {
				object[] values = new object[size];
				for (int i = 0; i < values.Length; i++) {
					values[i] = Value(type);
				}
				array.Values = values;
			}
			return array;
		}
		
		class JClassDesc { public string Name; public long SerialUID; public JClassDescInfo Info; }
		JClassDesc NewClassDesc() {
			JClassDesc desc = new JClassDesc();
			desc.Name = ReadUtf8();
			desc.SerialUID = ReadInt64();
			handles.Add(desc);
			desc.Info = ClassDescInfo();
			return desc;
		}
		
		JClassDesc ClassDesc() {
			byte typeCode = ReadUInt8();
			if (typeCode == TC_CLASSDESC) return NewClassDesc();
			if (typeCode == TC_NULL) return null;
			if (typeCode == TC_REFERENCE) return (JClassDesc)PrevObject();
			
			throw new InvalidDataException("Invalid type code: " + typeCode);
		}
		
		class JClassData { public object[] Values; public object Annotation; }
		JClassData ClassData(JClassDescInfo info) {
			if ((info.Flags & SC_SERIALIZABLE) == 0) {
				throw new InvalidDataException("Invalid class data flags: " + info.Flags);
			}
			
			JClassData data = new JClassData();
			data.Values = new object[info.Fields.Length];
			for (int i = 0; i < data.Values.Length; i++) {
				data.Values[i] = Value(info.Fields[i].Type);
			}
			
			if ((info.Flags & SC_WRITE_METHOD) != 0) {
				data.Annotation = Annotation();
			}
			return data;
		}
		
		class JClassDescInfo { public byte Flags; public JFieldDesc[] Fields; public object Annotation; public JClassDesc SuperClass; }
		JClassDescInfo ClassDescInfo() {
			JClassDescInfo info = new JClassDescInfo();
			info.Flags = ReadUInt8();
			info.Fields = new JFieldDesc[ReadUInt16()];
			for (int i = 0; i < info.Fields.Length; i++) {
				info.Fields[i] = FieldDesc();
			}
			info.Annotation = Annotation();
			info.SuperClass = ClassDesc();
			return info;
		}
		
		unsafe object Value(char type) {
			if (type == 'B') return ReadUInt8();
			if (type == 'C') return (char)ReadUInt16();
			if (type == 'D') { long tmp = ReadInt64(); return *(double*)(&tmp); }
			if (type == 'F') { int tmp = ReadInt32(); return *(float*)(&tmp); }
			if (type == 'I') return ReadInt32();
			if (type == 'J') return ReadInt64();
			if (type == 'S') return ReadInt16();
			if (type == 'Z') return ReadUInt8() != 0;
			if (type == 'L') return ReadObject();
			if (type == '[') return ReadObject();
			
			throw new InvalidDataException("Invalid value code: " + type);
		}
		
		class JFieldDesc { public char Type; public string Name; public string ClassName; public override string ToString() { return Name; } }
		JFieldDesc FieldDesc() {
			JFieldDesc desc = new JFieldDesc();
			byte type = ReadUInt8();
			desc.Type = (char)type;
			
			if (type == 'B' || type == 'C' || type == 'D' || type == 'F' || type == 'I' || type == 'J' || type == 'S' || type == 'Z') {
				desc.Name = ReadUtf8();
			} else if (type == '[' || type == 'L') {
				desc.Name = ReadUtf8();
				desc.ClassName = (string)ReadObject();
			} else {
				throw new InvalidDataException("Invalid field type: " + type);
			}
			return desc;
		}
		
		object Annotation() {
			List<object> parts = new List<object>();
			byte typeCode;
			while ((typeCode = ReadUInt8()) != TC_ENDBLOCKDATA) {
				parts.Add(ReadContent(typeCode));
			}
			if (parts.Count > 0) System.Diagnostics.Debugger.Break();
			return parts;
		}
	}
}
#endif