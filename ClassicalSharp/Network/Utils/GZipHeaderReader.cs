// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;

namespace ClassicalSharp.Network {
	
	internal class GZipHeaderReader {
		
		enum State {
			Header1, Header2, CompressionMethod, Flags,
			LastModifiedTime, CompressionFlags, OperatingSystem,
			HeaderChecksum, Filename, Comment, Done,
		}
		
		State state = State.Header1;
		public bool done;
		int flags;
		int partsRead;
		
		public bool ReadHeader(Stream s) {
			int temp;
			switch (state) {
					
				case State.Header1:
					if (!ReadHeaderByte(s, out temp)) return false;
					if (temp != 0x1F) { throw new NotSupportedException("Byte 1 of GZIP header must be 1F"); }
					goto case State.Header2;
					
				case State.Header2:
					if (!ReadHeaderByte(s, out temp)) return false;
					if (temp != 0x8B) { throw new NotSupportedException("Byte 2 of GZIP header must be 8B"); }
					goto case State.CompressionMethod;
					
				case State.CompressionMethod:
					if (!ReadHeaderByte(s, out temp)) return false;
					if (temp != 0x08) { throw new NotSupportedException("Only DEFLATE compression supported"); }
					goto case State.Flags;
					
				case State.Flags:
					if (!ReadHeaderByte(s, out flags)) return false;
					if ((flags & 0x04) != 0) { throw new NotSupportedException("Unsupported gzip flags"); }
					goto case State.LastModifiedTime;
					
				case State.LastModifiedTime:
					for (; partsRead < 4; partsRead++) {
						int part = s.ReadByte();
						if (part == -1)
							return false;
					}
					partsRead = 0;
					state = State.CompressionFlags;
					goto case State.CompressionFlags;
					
				case State.CompressionFlags:
					if (!ReadHeaderByte(s, out temp)) return false;
					goto case State.OperatingSystem;
					
				case State.OperatingSystem:
					if (!ReadHeaderByte(s, out temp)) return false;
					goto case State.Filename;
					
				case State.Filename:
					if ((flags & 0x08) != 0) {
						for (; ;) {
							int part = s.ReadByte();
							if (part == -1) return false;
							if (part == 0) break;
						}
					}
					state = State.Comment;
					goto case State.Comment;
					
				case State.Comment:
					if ((flags & 0x10) != 0) {
						for (; ;) {
							int part = s.ReadByte();
							if (part == -1) return false;
							if (part == 0) break;
						}
					}
					state = State.HeaderChecksum;
					goto case State.HeaderChecksum;					
					
				case State.HeaderChecksum:
					if ((flags & 0x02) != 0) {
						for (; partsRead < 2; partsRead++) {
							int part = s.ReadByte();
							if (part == -1)
								return false;
						}
					}
					partsRead = 0;
					state = State.Done;
					done = true;
					return true;
			}
			return true;
		}
		
		bool ReadHeaderByte(Stream s, out int value) {
			value = s.ReadByte();
			if (value == -1) return false;
			
			state++;
			return true;
		}
	}
}