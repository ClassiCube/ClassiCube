// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Net.Sockets;
using OpenTK;
#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Network {
	
	public class NetWriter {
		
		public byte[] buffer = new byte[131];
		public int index = 0;
		public bool ExtendedPositions, ExtendedBlocks;
		Socket socket;
		
		public NetWriter(Socket socket) {
			this.socket = socket;
		}

		public void Send() {
			int offset = 0;
			while (offset < index) {
				offset += socket.Send(buffer, offset, index - offset, SocketFlags.None);
			}
			index = 0;
		}
		
		public void WriteUInt8(byte value) { buffer[index++] = value; }
		
		public void WriteInt16(short value) {
			buffer[index++] = (byte)(value >> 8);
			buffer[index++] = (byte)(value);
		}
		
		public void WriteInt32(int value) {
			buffer[index++] = (byte)(value >> 24);
			buffer[index++] = (byte)(value >> 16);
			buffer[index++] = (byte)(value >> 8);
			buffer[index++] = (byte)(value);
		}
		
		public void WriteString(string value) {
			int count = Math.Min(value.Length, Utils.StringLength);
			for (int i = 0; i < count; i++) {
				char c = value[i];
				if (c == '&') c = '%'; // escape colour codes
				buffer[index + i] = Utils.UnicodeToCP437(c);
			}
			
			for (int i = value.Length; i < Utils.StringLength; i++)
				buffer[index + i] = (byte)' ';
			index += Utils.StringLength;
		}
		
		public void WritePosition(Vector3 pos) {
			if (ExtendedPositions) {
				WriteInt32((int)(pos.X * 32));
				WriteInt32((int)((int)(pos.Y * 32) + 51));
				WriteInt32((int)(pos.Z * 32));
			} else {
				WriteInt16((short)(pos.X * 32));
				WriteInt16((short)((int)(pos.Y * 32) + 51));
				WriteInt16((short)(pos.Z * 32));
			}
		}
		
		public void WriteBlock(BlockID value) {
			#if USE16_BIT
			if (ExtendedBlocks) { buffer[index++] = (byte)(value >> 8); }
			buffer[index++] = (byte)value;
			#else
			buffer[index++] = value;
			#endif
		}
	}
}