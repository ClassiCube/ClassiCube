// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Net.Sockets;

namespace ClassicalSharp.Network {
	
	public class NetWriter {
		
		public byte[] buffer = new byte[131];
		public int index = 0;
		Socket socket;
		
		public NetWriter(Socket socket) {
			this.socket = socket;
		}
		
		public void WriteString(string value) {
			int count = Math.Min(value.Length, Utils.StringLength);
			for (int i = 0; i < count; i++) {
				char c = value[i];
				int cpIndex = 0;
				if (c == '&') {
					buffer[index + i] = (byte)'%'; // escape colour codes
				} else if (c >= ' ' && c <= '~') {
					buffer[index + i] = (byte)c;
				} else if ((cpIndex = Utils.ControlCharReplacements.IndexOf(c)) >= 0) {
					buffer[index + i] = (byte)cpIndex;
				} else if ((cpIndex = Utils.ExtendedCharReplacements.IndexOf(c)) >= 0) {
					buffer[index + i] = (byte)(cpIndex + 127);
				} else {
					buffer[index + i] = (byte)'?';
				}
			}
			
			for (int i = value.Length; i < Utils.StringLength; i++)
				buffer[index + i] = (byte)' ';
			index += Utils.StringLength;
		}
		
		public void WriteUInt8(byte value) {
			buffer[index++] = value;
		}
		
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

		public void Send() {
			int offset = 0;
			while (offset < index)
				offset += socket.Send(buffer, offset, index - offset, SocketFlags.None);
			index = 0;
		}
	}
}