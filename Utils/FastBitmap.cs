using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace ClassicalSharp {

	/// <summary> Represents a wrapper around a bitmap. Provides
	/// a fast implementation for getting and setting pixels in that bitmap. </summary>
	/// <remarks> Only supports 32 bit RGBA pixel format. </remarks>
	public unsafe class FastBitmap : IDisposable {
		
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
		byte* scan0;
		int stride;
		
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
		public void LockBits() {
			if( Bitmap == null ) throw new InvalidOperationException( "Bmp is null." );
			if( data != null ) {
				Bitmap.UnlockBits( data );
			}
			
			PixelFormat format = Bitmap.PixelFormat;
			if( !( format == PixelFormat.Format32bppArgb || format == PixelFormat.Format32bppRgb ) ) {
				throw new NotSupportedException( "Unsupported bitmap pixel format: " + format );
			}
			
			Rectangle rec = new Rectangle( 0, 0, Bitmap.Width, Bitmap.Height );
			data = Bitmap.LockBits( rec, ImageLockMode.ReadWrite, format );
			scan0 = (byte*)data.Scan0;
			stride = data.Stride;
		}
		
		public void Dispose() {
			UnlockBits();
		}
		
		public void UnlockBits() {
			if( data != null ) {
				Bitmap.UnlockBits( data );
				data = null;
				scan0 = (byte*)IntPtr.Zero;
				stride = 0;
			}
		}
		
		public int GetPixel( int x, int y ) {
			// TODO: Does this work with big-endian systems?
			int* row = (int*)( scan0 + ( y * stride ) );
			return row[x]; // b g r a
		}
		
		public int* GetRowPtr( int y ) {
			return (int*)( scan0 + ( y * stride ) );
		}
		
		public void SetPixel( int x, int y, int col ) {
			int* row = (int*)( scan0 + ( y * stride ) );
			row[x] = col;
		}
	}
}