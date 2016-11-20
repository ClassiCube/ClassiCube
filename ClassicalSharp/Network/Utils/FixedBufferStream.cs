// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;

namespace ClassicalSharp.Network {
	
	/// <summary> Similar to a memory stream except that its underlying array 
	/// cannot be resized and this class performs minimal validation checks. </summary>
	internal class FixedBufferStream : ReadOnlyStream {
		
		public byte[] _buffer;
		public int pos, len, bufferPos;

		public FixedBufferStream( byte[] buffer ) {
			_buffer = buffer;
		}
		
		public override void Flush() { }
		
		public override int Read( byte[] buffer, int offset, int count ) {
			int numBytes = len - pos;
			if( numBytes > count ) numBytes = count;
			if( numBytes <= 0 ) return 0;
			
			Buffer.BlockCopy( _buffer, bufferPos + pos, buffer, offset, numBytes );
			pos += numBytes;
			return numBytes;
		}
		
		public override int ReadByte() {
			if( pos >= len ) return -1;
			byte value = _buffer[bufferPos + pos];
			pos++;
			return value;
		}
	}
}
