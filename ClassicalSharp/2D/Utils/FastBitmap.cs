// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
#if ANDROID
using Android.Graphics;
using Java.Nio;
#else
using System.Drawing.Imaging;
#endif

namespace ClassicalSharp {

	/// <summary> Wrapper around a bitmap that allows extremely fast manipulation of 32bpp images. </summary>
	public unsafe class FastBitmap : IDisposable {
		
		public FastBitmap() { }
		
		[Obsolete( "You should always specify whether the bitmap is readonly or not." )]
		public FastBitmap( Bitmap bmp, bool lockBits ) : this( bmp, lockBits, false ) {
		}
		
		public FastBitmap( Bitmap bmp, bool lockBits, bool readOnly ) {
			SetData( bmp, lockBits, readOnly );
		}
		
		public void SetData( Bitmap bmp, bool lockBits, bool readOnly ) {
			Bitmap = bmp;
			if( lockBits ) LockBits();
			ReadOnly = readOnly;
		}
		
		public void SetData( int width, int height, int stride, IntPtr scan0, bool readOnly ) {
			Width = width;
			Height = height;
			Stride = stride;
			Scan0 = scan0;
			scan0Byte = (byte*)scan0;
			ReadOnly = readOnly;
		}
		
		public Bitmap Bitmap;
		public bool ReadOnly;
		#if !ANDROID
		BitmapData data;
		#else
		ByteBuffer data;
		#endif
		byte* scan0Byte;
		
		public bool IsLocked { get { return data != null; } }
		
		public IntPtr Scan0;
		public int Stride;
		public int Width, Height;
		
		/// <summary> Returns a pointer to the start of the y'th scanline. </summary>
		public int* GetRowPtr( int y ) {
			return (int*)(scan0Byte + (y * Stride));
		}
		
		public static void MovePortion( int srcX, int srcY, int dstX, int dstY, FastBitmap src, FastBitmap dst, int size ) {
			for( int y = 0; y < size; y++ ) {
				int* srcRow = src.GetRowPtr( srcY + y );
				int* dstRow = dst.GetRowPtr( dstY + y );
				for( int x = 0; x < size; x++ )
					dstRow[dstX + x] = srcRow[srcX + x];
			}
		}
		
		public static void CopyRow( int srcY, int dstY, FastBitmap src, FastBitmap dst, int width ) {
			int* srcRow = src.GetRowPtr( srcY ), dstRow = dst.GetRowPtr( dstY);
			for( int x = 0; x < width; x++ )
				dstRow[x] = srcRow[x];
		}
		
		public void Dispose() { UnlockBits(); }
		
		#if !ANDROID
		
		public void LockBits() {
			if( Bitmap == null ) throw new InvalidOperationException( "Underlying bitmap is null." );
			if( data != null ) return;
			
			PixelFormat format = Bitmap.PixelFormat;
			if( !Platform.Is32Bpp( Bitmap ) )
				throw new NotSupportedException( "Unsupported bitmap pixel format: " + format );
			
			Rectangle rec = new Rectangle( 0, 0, Bitmap.Width, Bitmap.Height );
			data = Bitmap.LockBits( rec, ImageLockMode.ReadWrite, format );
			Scan0 = data.Scan0;
			scan0Byte = (byte*)Scan0;
			Stride = data.Stride;
			Width = data.Width;
			Height = data.Height;
		}
		
		public void UnlockBits() {
			if( Bitmap == null || data == null )
				return;
			Bitmap.UnlockBits( data );
			data = null;
			scan0Byte = (byte*)IntPtr.Zero;
			Scan0 = IntPtr.Zero;
			Width = Height = Stride = 0;
		}
		
		#else
		
		public void LockBits() {
			// ====
			// TODO: Use Bitmap.LockPixels, need to cast the pointer though
			// ====
			if( Bitmap == null ) throw new InvalidOperationException( "Underlying bitmap is null." );
			if( data != null ) return;

			data = ByteBuffer.AllocateDirect( Bitmap.Width * Bitmap.Height * 4 );
			Bitmap.CopyPixelsToBuffer( data );
			Scan0 = data.GetDirectBufferAddress();
			scan0Byte = (byte*)Scan0;			
			Stride = Bitmap.Width * 4;
			Width = Bitmap.Width;
			Height = Bitmap.Height;
		}
		
		public void UnlockBits() {
			if( Bitmap == null || data == null )
				return;
			data.Rewind();
			if( !ReadOnly )
				Bitmap.CopyPixelsFromBuffer( data );
			data.Dispose();

			data = null;
			scan0Byte = (byte*)IntPtr.Zero;
			Scan0 = IntPtr.Zero;
			Width = Height = Stride = 0;
		}
		#endif
	}
}