using System;
using System.IO;
using System.Text;

namespace Injector {
	
	public sealed class StreamInjector : IDisposable {
		public Stream inStream, outStream;
		private byte[] buff;

		public StreamInjector( Stream input, Stream output ) {
			inStream = input;
			outStream = output;
			buff = new byte[8];
		}
		
		public void Close() {
			Dispose();
		}

		public void Dispose() {
			if( inStream != null ) {
				inStream.Close();
			}
			inStream = null;
			buff = null;
		}

		public bool ReadBool() {
			return ReadUInt8() != 0;
		}
		
		public byte ReadUInt8() {
			int value = inStream.ReadByte();
			if( value == -1 ) throw new EndOfStreamException();
			outStream.WriteByte( (byte)value );
			return (byte)value;
		}
		
		public sbyte ReadInt8() {
			return (sbyte)ReadUInt8();
		}
		
		public short ReadInt16() {
			return (short)ReadUInt16();
		}
		
		public ushort ReadUInt16() {
			FillBuffer( buff, 2 );
			return (ushort)( buff[1] | buff[0] << 8 );
		}

		public int ReadInt32() {
			return (int)ReadUInt32();
		}

		public uint ReadUInt32() {
			FillBuffer( buff, 4 );
			return (uint)( buff[3] | buff[2] << 8 | buff[1] << 16 | buff[0] << 24 );
		}
		
		public long ReadInt64() {
			return (long)ReadUInt64();
		}

		public ulong ReadUInt64() {
			FillBuffer( buff, 8 );
			uint low = (uint)( buff[7] | buff[6] << 8 | buff[5] << 16 | buff[4] << 24 );
			uint high = (uint)( buff[3] | buff[2] << 8 | buff[1] << 16 | buff[0] << 24 );
			return (ulong)high << 32 | (ulong)low;
		}

		public unsafe float ReadFloat32() {
			uint value = ReadUInt32();
			return *(float*)&value;
		}

		public unsafe double ReadFloat64() {
			ulong value = ReadUInt64();
			return *(double*)&value;
		}
		
		public string ReadString() {
			int length = ReadInt16() * 2;
			byte[] text = ReadRawBytes( length );
			return Encoding.BigEndianUnicode.GetString( text );
		}

		public byte[] ReadRawBytes( int count ) {
			byte[] buffer = new byte[count];
			FillBuffer( buffer, count );
			return buffer;
		}

		void FillBuffer( byte[] buffer, int numBytes ) {
			if( numBytes == 0 ) return;
			
			int totalRead = 0;
			do {
				int read = inStream.Read( buffer, totalRead, numBytes - totalRead );
				if( read == 0 ) throw new EndOfStreamException();
				totalRead += read;
			} while( totalRead < numBytes );
			outStream.Write( buffer, 0, numBytes );
		}
		
		public Slot ReadSlot() {
			Slot slot = new Slot();
			slot.Id = ReadInt16();
			if( slot.Id >= 0 ) {
				slot.Count = ReadUInt8();
				slot.Damage = ReadInt16();
			}
			return slot;
		}
	}
}
