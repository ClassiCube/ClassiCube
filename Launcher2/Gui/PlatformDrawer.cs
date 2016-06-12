// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security;
using ClassicalSharp;
using OpenTK.Platform;
using OpenTK.Platform.X11;
using OSStatus = OpenTK.Platform.MacOS.OSStatus;
using OSX = OpenTK.Platform.MacOS.Carbon;

namespace Launcher {
	
	/// <summary> Platform specific class used to transfer a bitmap directly to the screen. </summary>
	public abstract class PlatformDrawer {
		
		internal IWindowInfo info;
		public abstract void Init();
		
		public virtual Bitmap CreateFrameBuffer( int width, int height ) { 
			return new Bitmap( width, height );
		}
		
		public abstract void Resize();
		
		public abstract void Redraw( Bitmap framebuffer );
		
		public virtual void Redraw( Bitmap framebuffer, Rectangle rec ) { 
			Redraw( framebuffer ); 
		}
	}
	
	public sealed class WinPlatformDrawer : PlatformDrawer {
		
		const uint SRCCOPY = 0xCC0020;
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern int BitBlt( IntPtr dcDst, int dstX, int dstY, int width, int height,
		                         IntPtr dcSrc, int srcX, int srcY, uint drawOp );
		
		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr GetDC( IntPtr hwnd );
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr CreateCompatibleDC(IntPtr dc);
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr SelectObject( IntPtr dc, IntPtr handle );
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern int DeleteObject( IntPtr handle );
		
		[DllImport("user32.dll"), SuppressUnmanagedCodeSecurity]
		static extern int ReleaseDC( IntPtr dc, IntPtr hwnd );
		
		[DllImport("gdi32.dll"), SuppressUnmanagedCodeSecurity]
		static extern IntPtr CreateDIBSection( IntPtr hdc, [In] ref BITMAPINFO pbmi,
		                                      uint pila, out IntPtr ppvBits, IntPtr hSection, uint dwOffset );
		
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
			dc = GetDC( info.WinHandle );
			srcDC = CreateCompatibleDC( dc );
		}
		
		public override Bitmap CreateFrameBuffer( int width, int height ) {
			if( srcHB != IntPtr.Zero )
				DeleteObject( srcHB );
			
			BITMAPINFO bmp = new BITMAPINFO();
			bmp.biSize = (int)Marshal.SizeOf(typeof(BITMAPINFO));
			bmp.biWidth = width;
			bmp.biHeight = -height;
			bmp.biBitCount = 32;
			bmp.biPlanes = 1;

			IntPtr pointer;
			srcHB = CreateDIBSection( srcDC, ref bmp, 0, out pointer, IntPtr.Zero, 0 );
			return new Bitmap( width, height, width * 4, 
			                  System.Drawing.Imaging.PixelFormat.Format32bppArgb, pointer );
		}
		
		public override void Resize() {
			if( dc != IntPtr.Zero ) {
				ReleaseDC( info.WinHandle, dc );
				DeleteObject( srcDC );
			}
			dc = GetDC( info.WinHandle );
			srcDC = CreateCompatibleDC( dc );
		}
		
		public override void Redraw( Bitmap framebuffer ) {
			IntPtr oldSrc = SelectObject( srcDC, srcHB );
			int success = BitBlt( dc, 0, 0, framebuffer.Width, framebuffer.Height, srcDC, 0, 0, SRCCOPY );
			SelectObject( srcDC, oldSrc );
		}
		
		public override void Redraw( Bitmap framebuffer, Rectangle rec ) {
			IntPtr oldSrc = SelectObject( srcDC, srcHB );
			int success = BitBlt( dc, rec.X, rec.Y, rec.Width, rec.Height, srcDC, rec.X, rec.Y, SRCCOPY );
			SelectObject( srcDC, oldSrc );
		}
	}
	
	public sealed class WinOldPlatformDrawer : PlatformDrawer {
		
		Graphics g;
		public override void Init() {
			g = Graphics.FromHwnd( info.WinHandle );
		}
		
		public override void Resize() {
			if( g != null )
				g.Dispose();
			g = Graphics.FromHwnd( info.WinHandle );
		}
		
		public override void Redraw( Bitmap framebuffer ) {
			g.DrawImage( framebuffer, 0, 0, framebuffer.Width, framebuffer.Height );
		}
		
		public override void Redraw( Bitmap framebuffer, Rectangle rec ) {
			g.DrawImage( framebuffer, rec, rec, GraphicsUnit.Pixel );
		}
	}
	
	public sealed class OSXPlatformDrawer : PlatformDrawer {
		
		IntPtr windowPort;
		public override void Init() {
			windowPort = OSX.API.GetWindowPort( info.WinHandle );
		}
		
		public override void Resize() {
			windowPort = OSX.API.GetWindowPort( info.WinHandle );
		}
		
		public override void Redraw( Bitmap framebuffer ) {			
			using( FastBitmap bmp = new FastBitmap( framebuffer, true, true ) ) {
				IntPtr scan0 = bmp.Scan0;
				int size = bmp.Width * bmp.Height * 4;
				
				IntPtr colorSpace = OSX.API.CGColorSpaceCreateDeviceRGB();
				IntPtr provider = OSX.API.CGDataProviderCreateWithData( IntPtr.Zero, scan0, size, IntPtr.Zero );
				const uint flags = 4 | (2 << 12);
				IntPtr image = OSX.API.CGImageCreate( bmp.Width, bmp.Height, 8, 8 * 4, bmp.Stride,
				                                     colorSpace, flags, provider, IntPtr.Zero, 0, 0 );
				IntPtr context = IntPtr.Zero;
				OSStatus err = OSX.API.QDBeginCGContext( windowPort, ref context );
				OSX.API.CheckReturn( err );
				
				OSX.HIRect rect = new OSX.HIRect();
				rect.Origin.X = 0; rect.Origin.Y = 0;
				rect.Size.X = bmp.Width; rect.Size.Y = bmp.Height;
				
				OSX.API.CGContextDrawImage( context, rect, image );
				OSX.API.CGContextSynchronize( context );
				err = OSX.API.QDEndCGContext( windowPort, ref context );
				OSX.API.CheckReturn( err );
				
				OSX.API.CGImageRelease( image );
				OSX.API.CGDataProviderRelease( provider );
				OSX.API.CGColorSpaceRelease( colorSpace );
			}
		}
	}
	
	public unsafe sealed class X11PlatformDrawer : PlatformDrawer {
		
		IntPtr gc;
		public override void Init() {
			gc = API.XCreateGC( API.DefaultDisplay, info.WinHandle, IntPtr.Zero, null );
		}
		
		public override void Resize() {
			if( gc != IntPtr.Zero ) API.XFreeGC( API.DefaultDisplay, gc );
			gc = API.XCreateGC( API.DefaultDisplay, info.WinHandle, IntPtr.Zero, null );
		}
		
		public override void Redraw( Bitmap framebuffer ) {
			X11WindowInfo x11Info = (X11WindowInfo)info;
			using( FastBitmap fastBmp = new FastBitmap( framebuffer, true, true ) ) {
				switch( x11Info.VisualInfo.Depth ) {
					case 32: DrawDirect( fastBmp, 32, x11Info ); break;
					case 24: DrawDirect( fastBmp, 24, x11Info ); break;
					//case 16: Draw16Bits( fastBmp, x11Info ); break;
					//case 15: Draw15Bits( fastBmp, x11Info ); break;
					default: throw new NotSupportedException("Unsupported bits per pixel: " + x11Info.VisualInfo.Depth );
				}
			}
		}
		
		void DrawDirect( FastBitmap bmp, uint bits, X11WindowInfo x11Info ) {
			IntPtr image = API.XCreateImage( API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                bits, ImageFormat.ZPixmap, 0, bmp.Scan0,
			                                bmp.Width, bmp.Height, 32, 0 );
			API.XPutImage( API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              0, 0, 0, 0, bmp.Width, bmp.Height );
			API.XFree( image );
		}
		
		public override void Redraw( Bitmap framebuffer, Rectangle rec ) {
			X11WindowInfo x11Info = (X11WindowInfo)info;
			using( FastBitmap fastBmp = new FastBitmap( framebuffer, true, true ) ) {
				switch( x11Info.VisualInfo.Depth ) {
					case 32: DrawDirect( fastBmp, 32, x11Info, rec ); break;
					case 24: DrawDirect( fastBmp, 24, x11Info, rec ); break;
					default: throw new NotSupportedException("Unsupported bits per pixel: " + x11Info.VisualInfo.Depth );
				}
			}
		}
		
		void DrawDirect( FastBitmap bmp, uint bits, X11WindowInfo x11Info, Rectangle rec ) {
			IntPtr image = API.XCreateImage( API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                bits, ImageFormat.ZPixmap, 0, bmp.Scan0,
			                                bmp.Width, bmp.Height, 32, 0 );
			API.XPutImage( API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              rec.X, rec.Y, rec.X, rec.Y, bmp.Width, bmp.Height );
			API.XFree( image );
		}
		
		// These bits per pixel are less common but are much slower
		// TODO: find a platform that actually creates 16bpp windows
		/*void Draw16Bits( FastBitmap bmp, X11WindowInfo x11Info ) {
			int bytes = bmp.Width * bmp.Height * 2;
			IntPtr ptr = Marshal.AllocHGlobal( bytes + 16 ); // ensure we allocate aligned num bytes
			ushort* dst = (ushort*)ptr;
			
			for( int y = 0; y < bmp.Height; y++ ) {
				int* src = bmp.GetRowPtr( y );
				for( int x = 0; x < bmp.Width; x++ ) {
					int value = *src; src++;
					int pixel =
						(((value & 0xFF0000) >> (16 + 3)) << 11) // R
						| (((value & 0xFF00) >> (8 + 2)) << 5)   // G
						| ((value & 0xFF) >> (0 + 3));           // B
		 			*dst = (ushort)pixel; dst++;
				}
			}
			
			IntPtr image = API.XCreateImage( API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                16, ImageFormat.ZPixmap, 0, ptr,
			                                bmp.Width, bmp.Height, 16, 0 );
			API.XPutImage( API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              0, 0, 0, 0, bmp.Width, bmp.Height );
			API.XFree( image );
			Marshal.FreeHGlobal( ptr );
		}
		
		void Draw15Bits( FastBitmap bmp, X11WindowInfo x11Info ) {
			int bytes = bmp.Width * bmp.Height * 2;
			IntPtr ptr = Marshal.AllocHGlobal( bytes + 16 ); // ensure we allocate aligned num bytes
			ushort* dst = (ushort*)ptr;
			
			for( int y = 0; y < bmp.Height; y++ ) {
				int* src = bmp.GetRowPtr( y );
				for( int x = 0; x < bmp.Width; x++ ) {
					int value = *src; src++;
					int pixel =
						(((value & 0xFF0000) >> (16 + 3)) << 10) // R
						| (((value & 0xFF00) >> (8 + 3)) << 5)   // G
						| ((value & 0xFF) >> (0 + 3));           // B
		 			*dst = (ushort)pixel; dst++;
				}
			}
			
			IntPtr image = API.XCreateImage( API.DefaultDisplay, x11Info.VisualInfo.Visual,
			                                15, ImageFormat.ZPixmap, 0, ptr,
			                                bmp.Width, bmp.Height, 16, 0 );
			API.XPutImage( API.DefaultDisplay, x11Info.WindowHandle, gc, image,
			              0, 0, 0, 0, bmp.Width, bmp.Height );
			API.XFree( image );
			Marshal.FreeHGlobal( ptr );
		}*/
	}
}
