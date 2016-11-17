// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.IO;

namespace ClassicalSharp {
	
	/// <summary> Implements a non-seekable stream that can only be read from. </summary>
	internal abstract class ReadOnlyStream : Stream {
		
		static NotSupportedException ex = new NotSupportedException("Writing/Seeking not supported");
		
		public override bool CanRead { get { return true; } }
		
		public override bool CanSeek { get { return false; } }
		
		public override bool CanWrite { get { return false; } }
		
		public override long Length { get { throw ex; } }
		
		public override long Position { get { throw ex; } set { throw ex; } }
		
		public override long Seek( long offset, SeekOrigin origin ) { throw ex; }
		
		public override void SetLength( long value ) { throw ex; }
		
		public override void Write( byte[] buffer, int offset, int count ) { throw ex; }
	}
}
