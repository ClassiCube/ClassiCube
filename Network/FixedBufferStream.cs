using System;
using System.IO;

namespace ClassicalSharp {
	
	internal class FixedBufferStream : Stream {
		
		public byte[] _buffer;
		private int _position;
		private int _length;
		
		public override bool CanRead {
			get { return true; }
		}
		
		public override bool CanSeek {
			get { return true; }
		}
		
		public override bool CanWrite {
			get { return true; }
		}
		
		public override long Length {
			get { return _length; }
		}
		
		public override long Position {
			get { return _position; }
			set { _position = (int)value; }
		}

		public FixedBufferStream( int capacity ) {
			_buffer = new byte[capacity];
			_length = capacity;
		}
		
		public override void Flush() {
		}
		
		public override int Read( byte[] buffer, int offset, int count ) {
			int numBytes = _length - _position;
			if( numBytes > count ) {
				numBytes = count;
			}
			if( numBytes <= 0 ) {
				return 0;
			}
			
			Buffer.BlockCopy( _buffer, _position, buffer, offset, numBytes );
			_position += numBytes;
			return numBytes;
		}
		
		public override int ReadByte() {
			if( _position >= _length ) {
				return -1;
			}
			return _buffer[_position++];
		}
		
		public override long Seek( long offset, SeekOrigin loc ) {
			throw new NotImplementedException();
		}
		
		public override void SetLength( long value ) {
			_length = (int)value;
		}
		
		public override void Write( byte[] buffer, int offset, int count ) {			
			Buffer.BlockCopy( buffer, offset, _buffer, _position, count );
			_position += count;
		}
		
		public override void WriteByte( byte value ) {
			_buffer[_position++] = value;
		}
	}
}
