using System;

namespace SoftwareRasterizer.Engine {
	
	public static class Utils {
		
		static bool is64bit = IntPtr.Size == 8;		
		
		public unsafe static void memzero( IntPtr src, int numBytes ) {
			if( is64bit ) {
				long address = (long)src;
				while( numBytes >= 8 ) {
					*(long*)address = 0;
					address += 8;
					numBytes -= 8;
				}
				while( numBytes >= 0 ) {
					*(byte*)address = 0;
					address++;
					numBytes--;
				}
			} else {
				int address = (int)src;
				while( numBytes >= 4 ) {
					*(int*)address = 0;
					address += 4;
					numBytes -= 4;
				}
				while( numBytes >= 0 ) {
					*(int*)address = 0;
					address++;
					numBytes--;
				}
			}
		}
		
		public static unsafe void memcpy( IntPtr src, IntPtr dst, int numBytes ) {
			if( is64bit ) {
				long srcAddress = (long)src;
				long dstAddress = (long)dst;
				while( numBytes >= 8 ) {
					*(long*)dstAddress = *(long*)srcAddress;
					srcAddress += 8;
					dstAddress += 8;
					numBytes -= 8;
				}
				while( numBytes >= 0 ) {
					*(byte*)dstAddress = *(byte*)srcAddress;
					srcAddress++;
					dstAddress++;
					numBytes--;
				}
			} else {
				int srcAddress = (int)src;
				int dstAddress = (int)dst;
				while( numBytes >= 4 ) {
					*(int*)dstAddress = *(int*)srcAddress;
					srcAddress += 4;
					dstAddress += 4;
					numBytes -= 4;
				}
				while( numBytes >= 0 ) {
					*(byte*)dstAddress = *(byte*)srcAddress;
					srcAddress++;
					dstAddress++;
					numBytes--;
				}
			}
		}
	}
}
