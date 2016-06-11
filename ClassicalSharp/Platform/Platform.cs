// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
#if ANDROID
using Android.Graphics;
#else
using System.Drawing.Imaging;
#endif

namespace ClassicalSharp {

	public static class Platform {
		
		public static Bitmap CreateBmp( int width, int height ) {
			#if !ANDROID
			return new Bitmap( width, height );
			#else
			return Bitmap.CreateBitmap( width, height, Bitmap.Config.Argb8888 );
			#endif
		}
		
		public static Bitmap ReadBmp( Stream src ) {
			#if !ANDROID
			return new Bitmap( src );
			#else
			return BitmapFactory.DecodeStream( src );
			#endif
		}
		
		public static void WriteBmp( Bitmap bmp, Stream dst ) {
			#if !ANDROID
			bmp.Save( dst, ImageFormat.Png );
			#else
			bmp.Compress( Bitmap.CompressFormat.Png, 100, dst );
			#endif
		}
	}
}
