using System;

namespace OpenTK {
	
	public static class MemUtils {
		
		static MemUtils() {
			use64Bit = IntPtr.Size == 8;
		}
		static bool use64Bit;
		
		public static unsafe void memcpy( IntPtr srcPtr, IntPtr dstPtr, int bytes ) {
			byte* srcByte, dstByte;
			if( use64Bit ) {
				ulong* srcLong = (ulong*)srcPtr, dstLong = (ulong*)dstPtr;
				while( bytes >= 8 ) {
					*dstLong++ = *srcLong++;
					bytes -= 8;
				}
				srcByte = (byte*)srcLong; dstByte = (byte*)dstLong;
			} else {
				uint* srcInt = (uint*)srcPtr, dstInt = (uint*)dstPtr;
				while( bytes >= 4 ) {
					*dstInt++ = *srcInt++;
					bytes -= 4;
				}
				srcByte = (byte*)srcInt; dstByte = (byte*)dstInt;
			}
			
			for( int i = 0; i < bytes; i++ ) {
				*dstByte++ = *srcByte++;
			}
		}
		
		public static unsafe void memset( IntPtr srcPtr, byte value, int startIndex, int bytes ) {
			byte* srcByte = (byte*)srcPtr + startIndex;
			// Make sure we do an aligned write/read for the bulk copy
			while( bytes > 0 && ( startIndex & 0x7 ) != 0  ) {
				*srcByte++ = value;
				startIndex++;
				bytes--;
			}
			uint valueInt = (uint)( ( value << 24 ) | ( value << 16 ) | ( value << 8 ) | value );
			
			if( use64Bit ) {
				ulong valueLong = ( (ulong)valueInt << 32 ) | valueInt;
				ulong* srcLong = (ulong*)srcByte;
				while( bytes >= 8 ) {
					*srcLong++ = valueLong;
					bytes -= 8;
				}
				srcByte = (byte*)srcLong;
			} else {
				uint* srcInt = (uint*)srcByte;
				while( bytes >= 4 ) {
					*srcInt++ = valueInt;
					bytes -= 4;
				}
				srcByte = (byte*)srcInt;
			}
			
			for( int i = 0; i < bytes; i++ ) {
				*srcByte++ = value;
			}
		}
	}
}
