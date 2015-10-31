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
		
		public FastBitmap( int width, int height, int stride, IntPtr scan0 ) {
			Width = width;
			Height = height;
			Stride = stride;
			Scan0 = scan0;
			scan0Byte = (byte*)scan0;
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
		
		public static bool CheckFormat( PixelFormat format ) {
			return format == PixelFormat.Format32bppRgb || format == PixelFormat.Format32bppArgb;
		}
		
		public void LockBits() {
			if( Bitmap == null ) throw new InvalidOperationException( "Underlying bitmap is null." );
			if( data != null ) return;
			
			PixelFormat format = Bitmap.PixelFormat;
			if( !CheckFormat( format ) )
				throw new NotSupportedException( "Unsupported bitmap pixel format: " + format );
			
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
			if( Bitmap != null && data != null ) {
				Bitmap.UnlockBits( data );
				data = null;
				scan0Byte = (byte*)IntPtr.Zero;
				Scan0 = IntPtr.Zero;
				Width = Height = Stride = 0;
			}
		}
		
		/// <summary> Returns a pointer to the start of the y'th scanline. </summary>
		public int* GetRowPtr( int y ) {
			return (int*)(scan0Byte + (y * Stride));
		}
		
		public static void MovePortion( int srcX, int srcY, int dstX, int dstY, FastBitmap src, FastBitmap dst, int size ) {
			for( int y = 0; y < size; y++ ) {
				int* srcRow = src.GetRowPtr( srcY + y );
				int* dstRow = dst.GetRowPtr( dstY + y );
				for( int x = 0; x < size; x++ ) {
					dstRow[dstX + x] = srcRow[srcX + x];
				}
			}
		}
		
		public static void CopyScaledPixels( FastBitmap src, FastBitmap dst, Size scale,
		                                    Rectangle srcRect, Rectangle dstRect, byte rgbScale ) {
			int srcWidth = srcRect.Width, dstWidth = dstRect.Width;
			int srcHeight = srcRect.Height, dstHeight = dstRect.Height;
			int srcX = srcRect.X, dstX = dstRect.X;
			int srcY = srcRect.Y, dstY = dstRect.Y;
			int scaleWidth = scale.Width, scaleHeight = scale.Height;
			
			for( int yy = 0; yy < dstHeight; yy++ ) {
				int scaledY = yy * srcHeight / scaleHeight;
				int* srcRow = src.GetRowPtr( srcY + scaledY );
				int* dstRow = dst.GetRowPtr( dstY + yy );
				
				for( int xx = 0; xx < dstWidth; xx++ ) {
					int scaledX = xx * srcWidth / scaleWidth;
					int pixel = srcRow[srcX + scaledX];
					
					int col = pixel & ~0xFFFFFF; // keep a but clear rgb
					col |= ((pixel & 0xFF) * rgbScale / 255);
					col |= (((pixel >> 8) & 0xFF) * rgbScale / 255) << 8;
					col |= (((pixel >> 16) & 0xFF) * rgbScale / 255) << 16;
					dstRow[dstX + xx] = col;
				}
			}
		}
	}
}