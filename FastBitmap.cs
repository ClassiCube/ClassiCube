using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace ClassicalSharp {

	/// <summary> Represents a wrapper around a bitmap. Provides
	/// a fast implementation for getting and setting pixels in that bitmap. </summary>
	public class FastBitmap : IDisposable {
		
		unsafe abstract class PixelAccessor {
			public byte* scan0;
			public int stride;
			
			public abstract int GetPixel( int x, int y );
			
			public abstract void SetPixel( int x, int y, int col );
		}
		
		unsafe class _32bppARGBAccessor : PixelAccessor {
			
			public override int GetPixel( int x, int y ) {
				// row + ( x << 2 ) points to the B part of the pixel at (x, bottomY/row)
				// casting to Int* then returning the value is a cheap way of 
				// doing the bitwise arithmetic that would otherwise be used.
				// TODO: Does this work with big-endian systems? I don't know.
				int* row = (int*)( scan0 + ( y * stride ) );
				return row[x]; // b g r a
			}
			
			public override void SetPixel( int x, int y, int col ) {
				int* row = (int*)( scan0 + ( y * stride ) );
				row[x] = col;
			}
		}
		
		/// <summary> Constructs a new FastBitmap wrapped around the specified Bitmap. </summary>
		/// <param name="bmp"> Bitmap which is wrapped. </param>
		/// <param name="lockBits"> Whether to immediately lock the bits of the bitmap,
		/// so that get and set pixel operations can be performed immediately after construction.</param>
		public FastBitmap( Bitmap bmp, bool lockBits ) {
			Bitmap = bmp;
			if( lockBits ) {
				LockBits();
			}
		}
		
		public Bitmap Bitmap;
		BitmapData data;
		PixelAccessor accessor;
		
		public bool IsLocked {
			get { return data != null; }
		}
		
		/// <summary> Gets the address of the first pixel in this bitmap. </summary>
		public IntPtr Scan0 {
			get { return data.Scan0; }
		}
		
		/// <summary> Gets the stride width/scan width of the bitmap. 
		/// (i.e. the actual size of each scanline, including padding) </summary>
		public int Stride {
			get { return data.Stride; }
		}
		
		/// <summary> Gets the width of this bitmap, in pixels. </summary>
		public int Width {
			get { return data.Width; }
		}
		
		/// <summary> Gets the height of this bitmap, in pixels. </summary>
		public int Height {
			get { return data.Height; }
		}
		
		/// <summary> Locks the wrapped bitmap into system memory, 
		/// so that fast get/set pixel operations can be performed. </summary>
		/// <remarks> Only 24 and 32 bit bitmap formats are supported. </remarks>
		public unsafe void LockBits() {
			if( Bitmap == null ) throw new InvalidOperationException( "Bmp is null." );
			if( data != null ) {
				Bitmap.UnlockBits( data );
			}
			
			PixelFormat format = Bitmap.PixelFormat;
			if( format == PixelFormat.Format32bppArgb ) {
				accessor = new _32bppARGBAccessor();
			} else {
				throw new NotSupportedException( "Unsupported bitmap pixel format:" + format );
			}
			
			Rectangle rec = new Rectangle( 0, 0, Bitmap.Width, Bitmap.Height );
			data = Bitmap.LockBits( rec, ImageLockMode.ReadWrite, format );
			accessor.scan0 = (byte*)data.Scan0;
			accessor.stride = data.Stride;
		}
		
		public void Dispose() {
			UnlockBits();
		}
		
		public void UnlockBits() {
			if( data != null ) {
				Bitmap.UnlockBits( data );
				data = null;
				accessor = null;
			}
		}
		
		/// <summary> Gets the colour of the pixel at the specified coordinates. </summary>
		/// <param name="x">X coordinate of the pixel.</param>
		/// <param name="bottomY">Y coordinate of the pixel.</param>
		/// <remarks>Does *not* perform bounds checking for performance reasons. </remarks>
		/// <returns>The colour at that pixel, in ARGB form. (A is highest 24 bits)</returns>
		public int GetPixel( int x, int y ) {
			return accessor.GetPixel( x, y );
		}
		
		/// <summary> Sets the colour of the pixel at the specified coordinates. </summary>
		/// <param name="x">X coordinate of the pixel.</param>
		/// <param name="bottomY">Y coordinate of the pixel.</param>
		/// <param name="col">The new colour of the pixel, in ARGB form. (A is highest 8 bits)</param>
		/// <remarks>Does *not* perform bounds checking for performance reasons. </remarks>
		public void SetPixel( int x, int y, int col ) {
			accessor.SetPixel( x, y, col );
		}
	}
}