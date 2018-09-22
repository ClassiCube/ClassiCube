// Originally copyright (c) 2009 Dino Chiesa and Microsoft Corporation.
// All rights reserved.
// See license.txt, section Ionic.Zlib license
using System;
using System.IO;
using ClassicalSharp;

namespace Ionic.Zlib {
	
	internal class DeflateStream : ReadOnlyStream {
		
		InflateBlocks z;
		bool _leaveOpen;
		byte[] workBuffer;
		Stream _stream;

		public DeflateStream(Stream stream, bool leaveOpen) {
			_stream = stream;
			_leaveOpen = leaveOpen;
			workBuffer = new byte[16384]; // TODO: 1024 bytes?
			z = new InflateBlocks();
		}		

		public override void Close() {
			z.Free();
			z = null;
			
			if (!_leaveOpen)
				_stream.Dispose();
			_stream = null;
		}
		
		public override void Flush() { }

		public override int Read(byte[] buffer, int offset, int count) {
			// According to MS documentation, any implementation of the IO.Stream.Read function must:
			// (a) throw an exception if offset & count reference an invalid part of the buffer,
			//     or if count < 0, or if buffer is null
			// (b) return 0 only upon EOF, or if count = 0
			// (c) if not EOF, then return at least 1 byte, up to <count> bytes

			if (count == 0) return 0;
			int rc = 0;

			// set up the output of the deflate/inflate codec:
			z.OutputBuffer = buffer;
			z._NextOut = offset;
			z._AvailOut = count;
			z.InputBuffer = workBuffer;
			bool endOfInput = false;

			do {
				// need data in _workingBuffer in order to deflate/inflate.  Here, we check if we have any.
				if (z._AvailIn == 0 && !endOfInput) {
					// No data available, so try to Read data from the captive stream.
					z._NextIn = 0;
					z._AvailIn = _stream.Read(workBuffer, 0, workBuffer.Length);
					if (z._AvailIn == 0)
						endOfInput = true;
				}
				rc = z.Inflate();

				if (endOfInput && rc == RCode.BufferError)
					return 0;

				if (rc != RCode.Okay && rc != RCode.StreamEnd)
					throw new InvalidDataException("inflating: rc=" + rc);

				if ((endOfInput || rc == RCode.StreamEnd) && z._AvailOut == count)
					break; // nothing more to read
			} while (z._AvailOut > 0 && !endOfInput && rc == RCode.Okay);

			return count - z._AvailOut;
		}
	}
}