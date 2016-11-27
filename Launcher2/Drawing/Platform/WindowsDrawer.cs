// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security;

namespace Launcher.Drawing {
	public sealed class WinPlatformDrawer : PlatformDrawer {
		
		const uint SRCCOPY = 0xCC0020;
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern int BitBlt(IntPtr dcDst, int dstX, int dstY, int width, int height,
		                         IntPtr dcSrc, int srcX, int srcY, uint drawOp);
		
		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr GetDC(IntPtr hwnd);
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr CreateCompatibleDC(IntPtr dc);
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr SelectObject(IntPtr dc, IntPtr handle);
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern int DeleteObject(IntPtr handle);
		
		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		static extern int ReleaseDC(IntPtr dc, IntPtr hwnd);
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr CreateDIBSection(IntPtr hdc, [In] ref BITMAPINFO pbmi,
		                                      uint pila, out IntPtr ppvBits, IntPtr hSection, uint dwOffset);
		
		[StructLayout(LayoutKind.Sequential)]
		public struct BITMAPINFO {
			public int biSize;
			public int biWidth;
			public int biHeight;
			public short biPlanes;
			public short biBitCount;
			public int biCompression;
			public int biSizeImage;
			public int biXPelsPerMeter;
			public int biYPelsPerMeter;
			public int biClrUsed;
			public int biClrImportant;
			public uint bmiColors;
		}
		
		IntPtr dc, srcDC, srcHB;
		public override void Init() {
			dc = GetDC(info.WinHandle);
			srcDC = CreateCompatibleDC(dc);
		}
		
		public override Bitmap CreateFrameBuffer(int width, int height) {
			if (srcHB != IntPtr.Zero)
				DeleteObject(srcHB);
			
			BITMAPINFO bmp = new BITMAPINFO();
			bmp.biSize = (int)Marshal.SizeOf(typeof(BITMAPINFO));
			bmp.biWidth = width;
			bmp.biHeight = -height;
			bmp.biBitCount = 32;
			bmp.biPlanes = 1;

			IntPtr pointer;
			srcHB = CreateDIBSection(srcDC, ref bmp, 0, out pointer, IntPtr.Zero, 0);
			return new Bitmap(width, height, width * 4, 
			                  System.Drawing.Imaging.PixelFormat.Format32bppArgb, pointer);
		}
		
		public override void Resize() {
			if (dc != IntPtr.Zero) {
				ReleaseDC(info.WinHandle, dc);
				DeleteObject(srcDC);
			}
			dc = GetDC(info.WinHandle);
			srcDC = CreateCompatibleDC(dc);
		}
		
		public override void Redraw(Bitmap framebuffer, Rectangle r) {
			IntPtr oldSrc = SelectObject(srcDC, srcHB);
			int success = BitBlt(dc, r.X, r.Y, r.Width, r.Height, srcDC, r.X, r.Y, SRCCOPY);
			SelectObject(srcDC, oldSrc);
		}
	}
	
	public sealed class WinOldPlatformDrawer : PlatformDrawer {
		
		Graphics g;
		public override void Init() {
			g = Graphics.FromHwnd(info.WinHandle);
		}
		
		public override void Resize() {
			if (g != null)
				g.Dispose();
			g = Graphics.FromHwnd(info.WinHandle);
		}
		
		public override void Redraw(Bitmap framebuffer, Rectangle r) {
			g.DrawImage(framebuffer, r, r, GraphicsUnit.Pixel);
		}
	}
}