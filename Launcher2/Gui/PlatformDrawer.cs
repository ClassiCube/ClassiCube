// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using ClassicalSharp;
using OpenTK.Platform;
using OpenTK.Platform.X11;
using OSX = OpenTK.Platform.MacOS.Carbon;
using OSStatus = OpenTK.Platform.MacOS.OSStatus;

namespace Launcher {
	
	/// <summary> Platform specific class used to transfer a bitmap directly to the screen. </summary>
	public abstract class PlatformDrawer {
		
		public abstract void Init( IWindowInfo info );
		
		public abstract void Resize( IWindowInfo info );
		
		public abstract void Draw( IWindowInfo info , Bitmap framebuffer );
	}
	
	public sealed class WinPlatformDrawer : PlatformDrawer {
		
		Graphics g;
		public override void Init( IWindowInfo info ) {
			g = Graphics.FromHwnd( info.WinHandle );
		}
		
		public override void Resize( IWindowInfo info ) {
			if( g != null )
				g.Dispose();
			g = Graphics.FromHwnd( info.WinHandle );
		}
		
		public override void Draw( IWindowInfo info, Bitmap framebuffer ) {
			g.DrawImage( framebuffer, 0, 0, framebuffer.Width, framebuffer.Height );
		}
	}
	
	public sealed class OSXPlatformDrawer : PlatformDrawer {
		
		IntPtr windowPort;
		public override void Init( IWindowInfo info ) {
			windowPort = OSX.API.GetWindowPort( info.WinHandle );
		}
		
		public override void Resize( IWindowInfo info ) {
			windowPort = OSX.API.GetWindowPort( info.WinHandle );
		}
		
		public override void Draw( IWindowInfo info, Bitmap framebuffer ) {
			
			using( FastBitmap bmp = new FastBitmap( framebuffer, true ) ) {
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
		int depth;
		public override void Init( IWindowInfo info ) {
			gc = API.XCreateGC( API.DefaultDisplay, info.WinHandle, IntPtr.Zero, null );
			depth = ((X11WindowInfo)info).VisualInfo.Depth;
		}
		
		public override void Resize( IWindowInfo info ) {
			if( gc != IntPtr.Zero ) API.XFreeGC( API.DefaultDisplay, gc );
			gc = API.XCreateGC( API.DefaultDisplay, info.WinHandle, IntPtr.Zero, null );
			depth = ((X11WindowInfo)info).VisualInfo.Depth;
		}
		
		public override void Draw( IWindowInfo info, Bitmap framebuffer ) {
			X11WindowInfo x11Info = (X11WindowInfo)info;
			using( FastBitmap fastBmp = new FastBitmap( framebuffer, true ) ) {
				switch( depth ) {
					case 32: DrawDirect( fastBmp, 32, x11Info ); break;
					case 24: DrawDirect( fastBmp, 24, x11Info ); break;
					//case 16: Draw16Bits( fastBmp, x11Info ); break;
					//case 15: Draw15Bits( fastBmp, x11Info ); break;
					default: throw new NotSupportedException("Unsupported bits per pixel: " + depth );
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
