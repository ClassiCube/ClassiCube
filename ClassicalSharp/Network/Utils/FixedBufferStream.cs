// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;

namespace ClassicalSharp.Net {
	
	/// <summary> Similar to a memory stream except that its underlying array 
	/// cannot be resized and this class performs minimal validation checks. </summary>
	internal class FixedBufferStream : Stream {
		
		public byte[] _buffer;
		int _position, _length;		
		public int Offset;
		
		public override bool CanRead { get { return true; } }
		
		public override bool CanSeek { get { return false; } }
		
		public override bool CanWrite { get { return false; } }
		
		public override long Length { get { return _length; } }
		
		public override long Position {
			get { return _position; }
			set { _position = (int)value; }
		}

		public FixedBufferStream( byte[] buffer ) {
			_buffer = buffer;
		}
		
		public override void Flush() { }
		
		public override int Read( byte[] buffer, int offset, int count ) {
			int numBytes = _length - _position;
			if( numBytes > count ) numBytes = count;
			if( numBytes <= 0 ) return 0;
			
			Buffer.BlockCopy( _buffer, Offset + _position, buffer, offset, numBytes );
			_position += numBytes;
			return numBytes;
		}
		
		public override int ReadByte() {
			if( _position >= _length ) return -1;
			byte value = _buffer[Offset + _position];
			_position++;
			return value;
		}
		
		public override long Seek( long offset, SeekOrigin origin ) {
			throw new NotSupportedException();
		}
		
		public override void SetLength( long value ) {
			_length = (int)value;
		}
		
		public override void Write( byte[] buffer, int offset, int count ) {			
			throw new NotSupportedException();
		}
		
		public override void WriteByte( byte value ) {
			throw new NotSupportedException();
		}
	}
}
