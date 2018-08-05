using System;
using System.IO;
using SharpWave.Codecs;

namespace SharpWave.Containers {
	
	public abstract class IMediaContainer : Stream {
		protected Stream stream;	
		public IMediaContainer(Stream source) { stream = source; }
		
		public abstract void ReadMetadata();
		
		public abstract ICodec GetAudioCodec();
		
		#region Stream implementation
		
		public override bool CanRead { get { return true; } }	
		public override bool CanSeek { get { return false; } }	
		public override bool CanWrite { get { return false; } }
		public override void Flush() { stream.Flush(); }		
		public override long Length { get { return stream.Length; } }
		
		public override long Position {
			get { return stream.Position; }
			set { stream.Position = value; }
		}

		public override long Seek( long offset, SeekOrigin origin ) {
			return stream.Seek( offset, origin );
		}
		
		public override void SetLength( long value ) {
			throw new NotImplementedException( "SharpWave is only a decoder" );
		}
		
		public override int Read(byte[] buffer, int offset, int count) {
			return stream.Read(buffer, offset, count);
		}
		
		public override int ReadByte() { return stream.ReadByte(); }
		
		public override void Write( byte[] buffer, int offset, int count ) {
			throw new NotImplementedException( "SharpWave is only a decoder" );
		}
		
		#endregion
	}
}
