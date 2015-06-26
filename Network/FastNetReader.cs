using System;
using System.Net.Sockets;
using System.Text;

namespace ClassicalSharp {
	
	// Basically a much faster version of List<byte>( capacity )
	internal class FastNetReader {
		public byte[] buffer = new byte[4096 * 4];
		public int size = 0;
		public NetworkStream Stream;
		
		public FastNetReader( NetworkStream stream ) {
			Stream = stream;
		}
		
		public void ReadPendingData() {
			if( !Stream.DataAvailable ) return;
			// NOTE: Always using a read call that is a multiple of 4096
			// (appears to?) improve read performance.
			int received = Stream.Read( buffer, size, 4096 * 3 );
			size += received;
		}
		
		public void Remove( int byteCount ) {
			size -= byteCount;
			Buffer.BlockCopy( buffer, byteCount, buffer, 0, size );
			// We don't need to zero the old bytes, since they will be overwritten when ReadData() is called.
		}
		
		public int ReadInt32() {
			int value = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
			Remove( 4 );
			return value;
		}
		
		public short ReadInt16() {
			short value = (short)( buffer[0] << 8 | buffer[1] );
			Remove( 2 );
			return value;
		}
		
		public sbyte ReadInt8() {
			sbyte value = (sbyte)buffer[0];
			Remove( 1 );
			return value;
		}
		
		public byte ReadUInt8() {
			byte value = buffer[0];
			Remove( 1 );
			return value;
		}
		
		public byte[] ReadBytes( int length ) {
			byte[] data = new byte[length];
			Buffer.BlockCopy( buffer, 0, data, 0, length );
			Remove( length );
			return data;
		}
		
		/// <summary> Reads a string, then converts control characters into the
		/// unicode values of their equivalent code page 437 graphical representations. </summary>
		public string ReadTextString() {
			string value = GetTextString( buffer );
			Remove( 64 );
			return value;
		}
		
		internal string ReadWoMTextString( ref byte messageType, bool useMessageTypes ) {
			if( useMessageTypes ) return ReadTextString();
			
			string value = GetWoMTextString( buffer, ref messageType );
			Remove( 64 );
			return value;
		}
		
		public string ReadString() {
			string value = GetAsciiString( buffer );
			Remove( 64 );
			return value;
		}
		
		static char[] characters = new char[64];
		const string womDetail = "^detail.user=";
		static string GetTextString( byte[] data ) {			
			int length = CopyTextStringToBuffer( data );
			return new String( characters, 0, length );
		}
		
		static string GetWoMTextString( byte[] data, ref byte messageType ) {
			messageType = (byte)CpeMessageType.Normal;
			int length = CopyTextStringToBuffer( data );
			int offset = 0;
			if( IsWomDetailString( length ) ) {
				length -= womDetail.Length;
				offset += womDetail.Length;
				messageType = (byte)CpeMessageType.Status3;
			}
			return new String( characters, offset, length );
		}
		
		static bool IsWomDetailString( int length ) {			
			if( length < womDetail.Length ) 
				return false;
			
			for( int i = 0; i < womDetail.Length; i++ ) {
				if( characters[i] != womDetail[i] )
					return false;
			}
			return true;
		}
		
		static string GetAsciiString( byte[] data ) {
		    int length = 0;
			for( int i = 63; i >= 0; i-- ) {
				byte code = data[i];
				if( length == 0 && !( code == 0 || code == 0x20 ) )
				   length = i + 1;
				
				characters[i] = code >= 0x80 ? '?' : (char)code;				
			}
			return new String( characters, 0, length );
		}
		
		static int CopyTextStringToBuffer( byte[] data ) {
			// code page 437 indices --> actual unicode characters
			int length = 0;
			for( int i = 63; i >= 0; i-- ) {
				byte code = data[i];
				if( length == 0 && !( code == 0 || code == 0x20 ) )
				   length = i + 1;
				
				if( code < 0x20 ) { // general control characters
					characters[i] = controlCharReplacements[code];
				} else if( code < 0x7F ) { // normal ascii character
					characters[i] = (char)code;
				} else if( code == 0x7F ) { // delete control character
					characters[i] = '\u2302';
				} else if( code >= 0x80 ){ // extended ascii character
					characters[i] = extendedCharReplacements[code - 0x80];
				}
			}
			return length;
		}
		
		static char[] controlCharReplacements = new char[] { // 00 -> 1F
			'\u0000', '\u263A', '\u263B', '\u2665',
			'\u2666', '\u2663', '\u2660', '\u2022',
			'\u25D8', '\u25CB', '\u25D9', '\u2642',
			'\u2640', '\u266A', '\u266B', '\u263C', 

			'\u25BA', '\u25C4', '\u2195', '\u203C',
			'\u00B6', '\u00A7', '\u25AC', '\u21A8',
			'\u2191', '\u2193', '\u2192', '\u2190',
			'\u221F', '\u2194', '\u25B2', '\u25BC',
		};
		
		static char[] extendedCharReplacements = new char[] { // 80 -> FF
			'\u00C7', '\u00FC', '\u00E9', '\u00E2',
			'\u00E4', '\u00E0', '\u00E5', '\u00E7',
			'\u00EA', '\u00EB', '\u00E8', '\u00EF',
			'\u00EE', '\u00EC', '\u00C4', '\u00C5',

			'\u00C9', '\u00E6', '\u00C6', '\u00F4',
			'\u00F6', '\u00F2', '\u00FB', '\u00F9',
			'\u00FF', '\u00D6', '\u00DC', '\u00A2',
			'\u00A3', '\u00A5', '\u20A7', '\u0192',

			'\u00E1', '\u00ED', '\u00F3', '\u00FA',
			'\u00F1', '\u00D1', '\u00AA', '\u00BA',
			'\u00BF', '\u2310', '\u00AC', '\u00BD',
			'\u00BC', '\u00A1', '\u00AB', '\u00BB',

			'\u2591', '\u2592', '\u2593', '\u2502',
			'\u2524', '\u2561', '\u2562', '\u2556',
			'\u2555', '\u2563', '\u2551', '\u2557',
			'\u255D', '\u255C', '\u255B', '\u2510',

			'\u2514', '\u2534', '\u252C', '\u251C',
			'\u2500', '\u253C', '\u255E', '\u255F',
			'\u255A', '\u2554', '\u2569', '\u2566',
			'\u2560', '\u2550', '\u256C', '\u2567',

			'\u2568', '\u2564', '\u2565', '\u2559',
			'\u2558', '\u2552', '\u2553', '\u256B',
			'\u256A', '\u2518', '\u250C', '\u2588',
			'\u2584', '\u258C', '\u2590', '\u2580',

			'\u03B1', '\u00DF', '\u0393', '\u03C0',
			'\u03A3', '\u03C3', '\u00B5', '\u03C4',
			'\u03A6', '\u0398', '\u03A9', '\u03B4',
			'\u221E', '\u03C6', '\u03B5', '\u2229',

			'\u2261', '\u00B1', '\u2265', '\u2264',
			'\u2320', '\u2321', '\u00F7', '\u2248',
			'\u00B0', '\u2219', '\u00B7', '\u221A',
			'\u207F', '\u00B2', '\u25A0', '\u00A0',
		};
	}
}