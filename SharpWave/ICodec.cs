using System;
using System.Collections.Generic;
using System.IO;

namespace SharpWave.Codecs {
	
	public interface ICodec {		
		AudioFormat ReadHeader(Stream source);
		IEnumerable<AudioChunk> StreamData(Stream source);
		string Name { get; }
	}
	
	public sealed class AudioChunk {
		public byte[] Data;
		public int Length;
	}
}
