// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using System.IO;
#if ANDROID
using Android.Graphics;
#else
using System.Drawing.Imaging;
#endif

namespace ClassicalSharp {

	/// <summary> Abstracts away platform specific operations. </summary>
	public static class Platform {
		
		/// <summary> Reads a bitmap from the given byte array. </summary>
		/// <returns> Bitmap with pixel depth of 32 bits. </returns>
		public static Bitmap ReadBmp32Bpp(IDrawer2D drawer, byte[] data) {
			return ReadBmp32Bpp(drawer, new MemoryStream(data));
		}

		/// <summary> Reads a bitmap from the given stream. </summary>
		/// <returns> Bitmap with pixel depth of 32 bits. </returns>
		public static Bitmap ReadBmp32Bpp(IDrawer2D drawer, Stream src) {
			Bitmap bmp = ReadBmp(src);
			if (!Is32Bpp(bmp)) drawer.ConvertTo32Bpp(ref bmp);
			return bmp;
		}

		/// <summary> Creates a bitmap of the given dimensions. </summary>		
		public static Bitmap CreateBmp(int width, int height) {
			#if !ANDROID
			return new Bitmap(width, height);
			#else
			return Bitmap.CreateBitmap(width, height, Bitmap.Config.Argb8888);
			#endif
		}

		/// <summary> Reads a bitmap from the given stream. </summary>		
		public static Bitmap ReadBmp(Stream src) {
			#if !ANDROID
			return new Bitmap(src);
			#else
			return BitmapFactory.DecodeStream(src);
			#endif
		}

		/// <summary> Writes a bitmap to the given stream in PNG format. </summary>		
		public static void WriteBmp(Bitmap bmp, Stream dst) {
			#if !ANDROID
			bmp.Save(dst, ImageFormat.Png);
			#else
			bmp.Compress(Bitmap.CompressFormat.Png, 100, dst);
			#endif
		}
		
		/// <returns> Whether the given bitmap has a pixel depth of 32 bits. </returns>
		public static bool Is32Bpp(Bitmap bmp) {
			#if !ANDROID
			PixelFormat format = bmp.PixelFormat;
			return format == PixelFormat.Format32bppRgb || format == PixelFormat.Format32bppArgb;
			#else
			Bitmap.Config config = bmp.GetConfig();
			return config != null && config == Bitmap.Config.Argb8888;
			#endif
		}
	}
}
