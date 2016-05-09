// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Net.Sockets;

namespace ClassicalSharp.Network {
	
	internal class NetWriter {
		
		public byte[] buffer = new byte[131];
		public int index = 0;
		public NetworkStream Stream;
		
		public NetWriter( NetworkStream stream ) {
			Stream = stream;
		}
		
		public void WriteString( string value ) {
			int count = Math.Min( value.Length, 64 );
			for( int i = 0; i < count; i++ ) {
				char c = value[i];
				int cpIndex = 0;
				if( c == '&' ) {
					buffer[index + i] = (byte)'%'; // escape colour codes
				} else if( c >= ' ' && c <= '~' ) {
					buffer[index + i] = (byte)c;
				} else if( (cpIndex = Utils.ControlCharReplacements.IndexOf( c ) ) >= 0 ) {
					buffer[index + i] = (byte)cpIndex;
				} else if( (cpIndex = Utils.ExtendedCharReplacements.IndexOf( c ) ) >= 0 ) {
					buffer[index + i] = (byte)(cpIndex + 127);
				} else {
					buffer[index + i] = (byte)'?';
				}
			}
			for( int i = value.Length; i < 64; i++ ) {
				buffer[index + i] = (byte)' ';
			}
			index += 64;
		}
		
		public void WriteUInt8( byte value ) {
			buffer[index++] = value;
		}
		
		public void WriteInt16( short value ) {
			buffer[index++] = (byte)(value >> 8);
			buffer[index++] = (byte)(value );
		}
		
		public void WriteInt32( int value ) {
			buffer[index++] = (byte)(value >> 24);
			buffer[index++] = (byte)(value >> 16);
			buffer[index++] = (byte)(value >> 8);
			buffer[index++] = (byte)(value);
		}

		public void Send() {
			Stream.Write( buffer, 0, index );
			index = 0;
		}
	}
}