// Originally copyright (c) 2009 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// See license.txt, section Ionic.Zlib license
#if __MonoCS__
using System;

namespace Ionic.Zlib {
	
	public sealed class ZlibCodec {
		
		public byte[] InputBuffer;
		public int NextIn;
		public int AvailableBytesIn;

		public byte[] OutputBuffer;
		public int NextOut;
		public int AvailableBytesOut;
		
		InflateManager istate;

		public ZlibCodec() {
			istate = new InflateManager();
			const int windowBits = 15; // 32K LZ77 window (max value 15, min value 8)
			istate.Initialize(this, windowBits);
		}
		
		public int Inflate() {
			return istate.Inflate();
		}

		public void EndInflate() {
			istate.End();
			istate = null;
		}
	}
	
	public static class RCode {
		public const int Okay = 0;
		public const int StreamEnd = 1;
		public const int DataError = -3;
		public const int BufferError = -5;
	}
}
#endif