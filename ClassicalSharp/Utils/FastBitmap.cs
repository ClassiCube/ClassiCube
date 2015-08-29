using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace ClassicalSharp {

	/// <summary> Wrapper around a bitmap that allows extremely fast manipulation of 32bpp images. </summary>
	public unsafe class FastBitmap : IDisposable {
		
		public FastBitmap( Bitmap bmp, bool lockBits ) {
			Bitmap = bmp;
			if( lockBits ) {
				LockBits();
			}
		}
		
		public Bitmap Bitmap;
		BitmapData data;
		byte* scan0Byte;
		
		public bool IsLocked {
			get { return data != null; }
		}
		
		public IntPtr Scan0;
		public int Stride;		
		public int Width, Height;
		
		public void LockBits() {
			if( Bitmap == null ) throw new InvalidOperationException( "Underlying bitmap is null." );
			if( data != null ) return;
			
			PixelFormat format = Bitmap.PixelFormat;
			if( !( format == PixelFormat.Format32bppArgb || format == PixelFormat.Format32bppRgb ) ) {
				throw new NotSupportedException( "Unsupported bitmap pixel format: " + format );
			}
			
			Rectangle rec = new Rectangle( 0, 0, Bitmap.Width, Bitmap.Height );
			data = Bitmap.LockBits( rec, ImageLockMode.ReadWrite, format );
			scan0Byte = (byte*)data.Scan0;
			Scan0 = data.Scan0;
			Stride = data.Stride;
			Width = data.Width;
			Height = data.Height;
		}
		
		public void Dispose() {
			UnlockBits();
		}
		
		public void UnlockBits() {
			if( data != null ) {
				Bitmap.UnlockBits( data );
				data = null;
				scan0Byte = (byte*)IntPtr.Zero;
				Scan0 = IntPtr.Zero;
				Width = Height = Stride = 0;
			}
		}
		
		/// <summary> Returns a pointer to the start of the y'th scanline. </summary>
		public int* GetRowPtr( int y ) {
			return (int*)( scan0Byte + ( y * Stride ) );
		}
		
		internal static void MovePortion( int srcX, int srcY, int dstX, int dstY, FastBitmap src, FastBitmap dst, int size ) {
			for( int y = 0; y < size; y++ ) {
				int* srcRow = src.GetRowPtr( srcY + y );
				int* dstRow = dst.GetRowPtr( dstY + y );
				for( int x = 0; x < size; x++ ) {
					dstRow[dstX + x] = srcRow[srcX + x];
				}
			}
		}
	}
}