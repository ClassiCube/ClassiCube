using System;
using System.Drawing;
using System.Drawing.Imaging;

namespace ClassicalSharp {

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
		
		public int GetPixel( int x, int y ) {
			// TODO: Does this work with big-endian systems?
			int* row = (int*)( scan0Byte + ( y * Stride ) );
			return row[x]; // b g r a
		}
		
		public int* GetRowPtr( int y ) {
			return (int*)( scan0Byte + ( y * Stride ) );
		}
		
		public void SetPixel( int x, int y, int col ) {
			int* row = (int*)( scan0Byte + ( y * Stride ) );
			row[x] = col;
		}
	}
}