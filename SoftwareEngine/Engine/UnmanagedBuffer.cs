using System;
using Marshal = System.Runtime.InteropServices.Marshal;

namespace SoftwareRasterizer.Engine {
	
	public unsafe class UnmanagedBuffer<T> : IDisposable
		where T : struct {
		
		public IntPtr ptr;
		
		/// <summary> Number of bytes in each scanline. </summary>
		public int stride;
		int bufferSize;
		
		public UnmanagedBuffer( int width, int height ) {
			bufferSize = width * height * Marshal.SizeOf( new T() );
			stride = width * Marshal.SizeOf( new T() );
			AllocateMemory();
		}

		public void Resize( int width, int height ) {
			FreeMemory();
			stride = width * Marshal.SizeOf( new T() );
			bufferSize = width * height * Marshal.SizeOf( new T() );
			AllocateMemory();
		}
		
		public void Dispose() {
			FreeMemory();
		}
		
		void AllocateMemory() {
			ptr = Marshal.AllocHGlobal( bufferSize );
			Utils.memzero( ptr, bufferSize );
		}
		
		void FreeMemory() {
			Marshal.FreeHGlobal( ptr );
			ptr = IntPtr.Zero;
		}
	}
}
