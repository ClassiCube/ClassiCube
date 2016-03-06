using System;
using System.Net.Sockets;

namespace ClassicalSharp {

	internal class NetReader {
		
		public byte[] buffer = new byte[4096 * 5];
		public int index = 0, size = 0;
		public NetworkStream Stream;
		
		public NetReader( NetworkStream stream ) {
			Stream = stream;
		}
		
		public void ReadPendingData() {
			if( !Stream.DataAvailable ) return;
			// NOTE: Always using a read call that is a multiple of 4096
			// (appears to?) improve read performance.
			int received = Stream.Read( buffer, size, 4096 * 4 );
			size += received;
		}
		
		public void Skip( int byteCount ) {
			index += byteCount;
		}
		
		public void RemoveProcessed() {			
			size -= index;
			if( size > 0 ) // only copy left over bytes
				Buffer.BlockCopy( buffer, index, buffer, 0, size );			
			index = 0;
			// We don't need to zero the old bytes, since they will be overwritten when ReadData() is called.
		}
		
		public int ReadInt32() {
			int value = buffer[index] << 24 | buffer[index + 1] << 16 | 
				buffer[index + 2] << 8 | buffer[index + 3];
			index += 4;
			return value;
		}
		
		public short ReadInt16() {
			short value = (short)(buffer[index] << 8 | buffer[index + 1]);
			index += 2;
			return value;
		}
		
		public sbyte ReadInt8() {
			sbyte value = (sbyte)buffer[index];
			index++;
			return value;
		}
		
		public ushort ReadUInt16() {
			ushort value = (ushort)(buffer[index] << 8 | buffer[index + 1]);
			index += 2;
			return value;
		}
		
		public byte ReadUInt8() {
			byte value = buffer[index];
			index++;
			return value;
		}
		
		public byte[] ReadBytes( int length ) {
			byte[] data = new byte[length];
			Buffer.BlockCopy( buffer, index, data, 0, length );
			index += length;
			return data;
		}

		public string ReadCp437String() {
			int length = GetString( false, 64 );
			index += 64;
			return new String( characters, 0, length );
		}
		
		public string ReadAsciiString() {
			int length = GetString( true, 64 );
			index += 64;
			return new String( characters, 0, length );
		}
		
		internal string ReadChatString( ref byte messageType, bool useMessageTypes ) {
			if( useMessageTypes ) 
				return ReadCp437String();
					
			messageType = (byte)MessageType.Normal;
			int length = GetString( false, 64 );		
			index += 64;
			
			int offset = 0;
			if( length >= womDetail.Length && IsWomDetailString() ) {
				length -= womDetail.Length;
				offset = womDetail.Length;
				messageType = (byte)MessageType.Status3;
			}
			return new String( characters, offset, length );
		}
		
		static char[] characters = new char[64];
		const string womDetail = "^detail.user=";		
		static bool IsWomDetailString() {
			for( int i = 0; i < womDetail.Length; i++ ) {
				if( characters[i] != womDetail[i] )
					return false;
			}
			return true;
		}
		
		int GetString( bool ascii, int bufferSize ) {
			int length = 0;
			for( int i = bufferSize - 1; i >= 0; i-- ) {
				byte code = buffer[index + i];
				if( length == 0 && !( code == 0 || code == 0x20 ) )
				   length = i + 1;
				
				if( ascii ) {
					characters[i] = code >= 0x7F ? '?' : (char)code;
					continue;
				}
				
				// Treat code as an index in code page 437
				if( code < 0x20 ) {
					characters[i] = Utils.ControlCharReplacements[code];
				} else if( code < 0x7F ) {
					characters[i] = (char)code;
				} else {
					characters[i] = Utils.ExtendedCharReplacements[code - 0x7F];
				}
			}
			return length;
		}
	}
}