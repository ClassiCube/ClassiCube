using System;

namespace SharpWave {
	
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
	}
}