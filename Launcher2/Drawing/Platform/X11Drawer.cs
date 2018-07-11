// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK.Platform.X11;

namespace Launcher.Drawing {
	public unsafe sealed class X11PlatformDrawer : PlatformDrawer {
		
		IntPtr gc;
		public override void Init() {
			gc = API.XCreateGC(API.DefaultDisplay, info.WinHandle, IntPtr.Zero, IntPtr.Zero);
		}
		
		public override void Resize() {
			if (gc != IntPtr.Zero) API.XFreeGC(API.DefaultDisplay, gc);
			gc = API.XCreateGC(API.DefaultDisplay, info.WinHandle, IntPtr.Zero, IntPtr.Zero);
		}
		
		public override void Redraw(Bitmap framebuffer, Rectangle r) {
			X11WindowInfo x11Info = (X11WindowInfo)info;
			using (FastBitmap bmp = new FastBitmap(framebuffer, true, true)) {
				switch(x11Info.VisualInfo.Depth) {
					case 32: DrawDirect(bmp, 32, x11Info, r); break;
					case 24: DrawDirect(bmp, 24, x11Info, r); break;
					//case 16: Draw16Bits(fastBmp, x11Info); break;
					//case 15: Draw15Bits(fastBmp, x11Info); break;
					default: throw new NotSupportedException("Unsupported bits per pixel: " + x11Info.VisualInfo.Depth);
				}
			}
		}
		
		void DrawDirect(FastBitmap bmp, uint bits, X11WindowInfo x11Info, Rectangle r) {
			IntPtr image = API.XCreateImage(API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                bits, ImageFormat.ZPixmap, 0, bmp.Scan0,
			                                bmp.Width, bmp.Height, 32, 0);
			API.XPutImage(API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              r.X, r.Y, r.X, r.Y, r.Width, r.Height);
			API.XFree(image);
		}
		
		// These bits per pixel are less common but are much slower
		// TODO: find a platform that actually creates 16bpp windows
		/*void Draw16Bits(FastBitmap bmp, X11WindowInfo x11Info) {
			int bytes = bmp.Width * bmp.Height * 2;
			IntPtr ptr = Marshal.AllocHGlobal(bytes + 16); // ensure we allocate aligned num bytes
			ushort* dst = (ushort*)ptr;
			
			for (int y = 0; y < bmp.Height; y++) {
				int* src = bmp.GetRowPtr(y);
				for (int x = 0; x < bmp.Width; x++) {
					int value = *src; src++;
					int pixel =
						(((value & 0xFF0000) >> (16 + 3)) << 11) // R
						| (((value & 0xFF00) >> (8 + 2)) << 5)   // G
						| ((value & 0xFF) >> (0 + 3));           // B
		 			*dst = (ushort)pixel; dst++;
				}
			}
			
			IntPtr image = API.XCreateImage(API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                16, ImageFormat.ZPixmap, 0, ptr,
			                                bmp.Width, bmp.Height, 16, 0);
			API.XPutImage(API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              0, 0, 0, 0, bmp.Width, bmp.Height);
			API.XFree(image);
			Marshal.FreeHGlobal(ptr);
		}
		
		void Draw15Bits(FastBitmap bmp, X11WindowInfo x11Info) {
			int bytes = bmp.Width * bmp.Height * 2;
			IntPtr ptr = Marshal.AllocHGlobal(bytes + 16); // ensure we allocate aligned num bytes
			ushort* dst = (ushort*)ptr;
			
			for (int y = 0; y < bmp.Height; y++) {
				int* src = bmp.GetRowPtr(y);
				for (int x = 0; x < bmp.Width; x++) {
					int value = *src; src++;
					int pixel =
						(((value & 0xFF0000) >> (16 + 3)) << 10) // R
						| (((value & 0xFF00) >> (8 + 3)) << 5)   // G
						| ((value & 0xFF) >> (0 + 3));           // B
		 			*dst = (ushort)pixel; dst++;
				}
			}
			
			IntPtr image = API.XCreateImage(API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                15, ImageFormat.ZPixmap, 0, ptr,
			                                bmp.Width, bmp.Height, 16, 0);
			API.XPutImage(API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              0, 0, 0, 0, bmp.Width, bmp.Height);
			API.XFree(image);
			Marshal.FreeHGlobal(ptr);
		}*/
	}
}
