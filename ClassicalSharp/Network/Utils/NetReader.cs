// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Net.Sockets;
using ClassicalSharp.Entities;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp.Network {

	public class NetReader {
		
		public byte[] buffer = new byte[4096 * 5];
		public int index = 0, size = 0;
		public bool ExtendedPositions, ExtendedBlocks;
		Socket socket;
		
		public NetReader(Socket socket) {
			this.socket = socket;
		}
		
		public void ReadPendingData() {
			if (socket.Available == 0) return;
			// NOTE: Always using a read call that is a multiple of 4096
			// (appears to?) improve read performance.
			int recv = socket.Receive(buffer, size, 4096 * 4, SocketFlags.None);
			size += recv;
		}
		
		public void Skip(int byteCount) {
			index += byteCount;
		}
		
		public void RemoveProcessed() {
			size -= index;
			if (size > 0) // only copy left over bytes
				Buffer.BlockCopy(buffer, index, buffer, 0, size);			
			index = 0;
			// We don't need to zero the old bytes, since they will be overwritten when ReadData() is called.
		}
		
		public byte ReadUInt8() { return buffer[index++]; }
		
		public sbyte ReadInt8() { return (sbyte)buffer[index++]; }

		public ushort ReadUInt16() {
			ushort value = (ushort)(buffer[index] << 8 | buffer[index + 1]);
			index += 2;
			return value;
		}
		
		public short ReadInt16() {
			short value = (short)(buffer[index] << 8 | buffer[index + 1]);
			index += 2;
			return value;
		}

		public int ReadInt32() {
			int value = buffer[index] << 24 | buffer[index + 1] << 16 | 
				buffer[index + 2] << 8 | buffer[index + 3];
			index += 4;
			return value;
		}

		public string ReadString() {
			int length = GetString(Utils.StringLength);
			return new String(characters, 0, length);
		}

		public byte[] ReadBytes(int length) {
			byte[] data = new byte[length];
			Buffer.BlockCopy(buffer, index, data, 0, length);
			index += length;
			return data;
		}
		
		public Vector3 ReadPosition(byte id) {
			int x = 0, y = 0, z = 0;
			if (ExtendedPositions) {
				x = ReadInt32(); y = ReadInt32(); z = ReadInt32();
			} else {
				x = ReadInt16(); y = ReadInt16(); z = ReadInt16();
			}
			
			float yAdj = (y - 51) / 32f; // We have to do this.
			if (id == EntityList.SelfID) yAdj += 22/32f;
			return new Vector3(x / 32f, yAdj, z / 32f);
		}
		
		public BlockID ReadBlock() {
			if (ExtendedBlocks) return ReadUInt16();
			return buffer[index++];			
		}
		
		internal string ReadChatString(ref byte messageType) {
			int length = GetString(Utils.StringLength);
			
			int offset = 0;
			if (length >= womDetail.Length && IsWomDetailString()) {
				length -= womDetail.Length;
				offset = womDetail.Length;
				messageType = (byte)MessageType.Status3;
			}
			return new String(characters, offset, length);
		}
		
		static char[] characters = new char[Utils.StringLength];
		const string womDetail = "^detail.user=";		
		static bool IsWomDetailString() {
			for (int i = 0; i < womDetail.Length; i++) {
				if (characters[i] != womDetail[i])
					return false;
			}
			return true;
		}
		
		int GetString(int maxLength) {
			int length = 0;
			
			for (int i = maxLength - 1; i >= 0; i--) {
				byte code = buffer[index + i];
				if (length == 0 && !(code == 0 || code == 0x20))
				   length = i + 1;
				
				characters[i] = Utils.CP437ToUnicode(code);
			}
			index += maxLength;
			return length;
		}
	}
}