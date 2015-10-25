using System;
using System.Drawing;
using OpenTK;
using OpenTK.Platform;
using OpenTK.Platform.X11;
using ClassicalSharp;

namespace Launcher2 {
	
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
	
	public sealed class X11PlatformDrawer : PlatformDrawer {
		
		IntPtr gc;	
		public override void Init( IWindowInfo info ) {
			gc = API.XCreateGC( API.DefaultDisplay, info.WinHandle, IntPtr.Zero, null );
		}
		
		public override void Resize( IWindowInfo info ) {
			if( gc != IntPtr.Zero )
				API.XFreeGC( API.DefaultDisplay, gc );
			gc = API.XCreateGC( API.DefaultDisplay, info.WinHandle, IntPtr.Zero, null );
		}
		
		public override void Draw( IWindowInfo info, Bitmap framebuffer ) {
			X11WindowInfo x11Info = info as X11WindowInfo;
			using( FastBitmap fastBmp = new FastBitmap( framebuffer, true ) ) {
				IntPtr image = API.XCreateImage( API.DefaultDisplay, x11Info.VisualInfo.Visual,
				                                24, ImageFormat.ZPixmap, 0, fastBmp.Scan0,
				                                fastBmp.Width, fastBmp.Height, 32, 0 );
				API.XPutImage( API.DefaultDisplay, x11Info.WindowHandle, gc, image,
				              0, 0, 0, 0, fastBmp.Width, fastBmp.Height );
				API.XFree( image );
			}
		}
	}
}
