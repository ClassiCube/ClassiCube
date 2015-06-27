using System;
using System.IO;

namespace DefaultPlugin.Network {
	
	internal class GZipHeaderReader {
		
		enum State {
			Header1,
			Header2,
			CompressionMethod,
			Flags,
			LastModifiedTime,
			CompressionFlags,
			OperatingSystem,
			HeaderChecksum,
			Done,
		}
		
		State state = State.Header1;
		public bool done;
		int flags;
		int partsRead;
		
		public bool ReadHeader( Stream s ) {		
			switch( state ) {
					
				case State.Header1:
					if( !ReadAndCheckHeaderByte( s, 0x1F ) )
						return false;
					goto case State.Header2;
					
				case State.Header2:
					if( !ReadAndCheckHeaderByte( s, 0x8B ) )
						return false;
					goto case State.CompressionMethod;
					
				case State.CompressionMethod:
					if( !ReadAndCheckHeaderByte( s, 0x08 ) )
						return false;
					goto case State.Flags;
					
				case State.Flags:
					if( !ReadHeaderByte( s, out flags ) )
						return false;
					if( flags >= 0x04 ) // We don't support extra, comment, or original name fields
						throw new NotSupportedException( "Unsupported gzip flags: " + flags );
					goto case State.LastModifiedTime;
					
				case State.LastModifiedTime:
					for( ; partsRead < 4; partsRead++ ) {
						int part = s.ReadByte();
						if( part == -1 )
							return false;
					}
					partsRead = 0;
					state = State.CompressionFlags;
					goto case State.CompressionFlags;
					
				case State.CompressionFlags:
					int comFlags;
					if( !ReadHeaderByte( s, out comFlags ) )
						return false;
					goto case State.OperatingSystem;
					
				case State.OperatingSystem:
					int os;
					if( !ReadHeaderByte( s, out os ) )
						return false;
					goto case State.HeaderChecksum;
					
				case State.HeaderChecksum:
					if( ( flags & 0x02 ) != 0 ) {
						for( ; partsRead < 2; partsRead++ ) {
							int part = s.ReadByte();
							if( part == -1 )
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
		
		bool ReadAndCheckHeaderByte( Stream s, byte expected ) {
			int value;
			if( !ReadHeaderByte( s, out value ) )
				return false;
			
			if( value != expected )
				throw new InvalidDataException( "Unepxected constant in GZip header. (" + expected + "-" + value + ")" );
			return true;
		}
		
		bool ReadHeaderByte( Stream s, out int value ) {
			value = s.ReadByte();
			if( value == -1 )
				return false;
			
			state++;
			return true;
		}
	}
}