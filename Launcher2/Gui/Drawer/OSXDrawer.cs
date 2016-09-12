// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using OSStatus = OpenTK.Platform.MacOS.OSStatus;
using OSX = OpenTK.Platform.MacOS.Carbon;

namespace Launcher {
	
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
}
