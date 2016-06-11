#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Runtime.InteropServices;
using System.Security;
using Window = System.IntPtr;
using KeySym = System.IntPtr;
using Display = System.IntPtr;
using Bool = System.Boolean;
using Status = System.Int32;
using Drawable = System.IntPtr;
using Time = System.IntPtr;
// Randr and Xrandr
using XRRScreenConfiguration = System.IntPtr; // opaque datatype
using KeyCode = System.Byte;    // Or maybe ushort?

namespace OpenTK.Platform.X11 {
	
	public static class API {

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XOpenDisplay(IntPtr display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XCloseDisplay(IntPtr display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XCreateWindow(IntPtr display, IntPtr parent, int x, int y, int width, int height, int border_width, int depth, int xclass, IntPtr visual, IntPtr valuemask, ref XSetWindowAttributes attributes);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XMapWindow(IntPtr display, IntPtr window);
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XUnmapWindow(IntPtr display, IntPtr window);
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XRootWindow(IntPtr display, int screen_number);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static Bool XCheckWindowEvent(Display display, Window w, EventMask event_mask, ref XEvent event_return);
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static Bool XCheckTypedWindowEvent(Display display, Window w, XEventName event_type, ref XEvent event_return);
		
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XDestroyWindow(IntPtr display, IntPtr window);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XMoveResizeWindow(IntPtr display, IntPtr window, int x, int y, int width, int height);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XMoveWindow(IntPtr display, IntPtr w, int x, int y);
		
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XResizeWindow(IntPtr display, IntPtr window, int width, int height);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XFlush(IntPtr display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XStoreName(IntPtr display, IntPtr window, string window_name);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XFetchName(IntPtr display, IntPtr window, ref IntPtr window_name);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XSendEvent(IntPtr display, IntPtr window, bool propagate, IntPtr event_mask, ref XEvent send_event);

		public static int XSendEvent(IntPtr display, IntPtr window, bool propagate, EventMask event_mask, ref XEvent send_event) {
			return XSendEvent(display, window, propagate, new IntPtr((int)event_mask), ref send_event);
		}
		
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
        public extern static bool XQueryPointer(IntPtr display, IntPtr window, out IntPtr root, out IntPtr child, out int root_x, out int root_y, out int win_x, out int win_y, out int keys_buttons);
        
        [DllImport("libX11"), SuppressUnmanagedCodeSecurity]
        public extern static uint XWarpPointer(IntPtr display, IntPtr src_w, IntPtr dest_w, int src_x, int src_y, uint src_width, uint src_height, int dest_x, int dest_y);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XFree(IntPtr data);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XRaiseWindow(IntPtr display, IntPtr window);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XInternAtom(IntPtr display, string atom_name, bool only_if_exists);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XSetWMProtocols(IntPtr display, IntPtr window, IntPtr[] protocols, int count);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static bool XTranslateCoordinates(IntPtr display, IntPtr src_w, IntPtr dest_w, int src_x, int src_y, out int intdest_x_return, out int dest_y_return, out IntPtr child_return);

		// Colormaps
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XDefaultDepth(IntPtr display, int screen_number);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XDefaultScreen(IntPtr display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XSetTransientForHint(IntPtr display, IntPtr window, IntPtr prop_window);
		
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XChangeProperty(IntPtr display, IntPtr window, IntPtr property, IntPtr type, int format, PropertyMode mode, IntPtr[] data, int nelements);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XDeleteProperty(IntPtr display, IntPtr window, IntPtr property);
		
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
        public extern static int XDefineCursor(IntPtr display, IntPtr window, IntPtr cursor);

        [DllImport("libX11"), SuppressUnmanagedCodeSecurity]
        public extern static int XUndefineCursor(IntPtr display, IntPtr window);

        [DllImport("libX11"), SuppressUnmanagedCodeSecurity]
        public extern static int XFreeCursor(IntPtr display, IntPtr cursor);

		// Drawing
		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XCreateGC(IntPtr display, IntPtr window, IntPtr valuemask, XGCValues[] values);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XFreeGC(IntPtr display, IntPtr gc);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XGetWindowProperty(IntPtr display, IntPtr window, IntPtr atom, IntPtr long_offset, IntPtr long_length, bool delete, IntPtr req_type, out IntPtr actual_type, out int actual_format, out IntPtr nitems, out IntPtr bytes_after, ref IntPtr prop);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XIconifyWindow(IntPtr display, IntPtr window, int screen_number);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XCreatePixmapFromBitmapData(IntPtr display, IntPtr drawable, byte[] data, int width, int height, IntPtr fg, IntPtr bg, int depth);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XCreatePixmap(IntPtr display, IntPtr d, int width, int height, int depth);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
        public extern static IntPtr XCreatePixmapCursor(IntPtr display, IntPtr source, IntPtr mask, ref XColor foregroundCol, ref XColor backgroundCol, int x_hot, int y_hot);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static IntPtr XFreePixmap(IntPtr display, IntPtr pixmap);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static int XGetWMNormalHints(IntPtr display, IntPtr window, ref XSizeHints hints, out IntPtr supplied_return);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static void XSetWMNormalHints(IntPtr display, IntPtr window, ref XSizeHints hints);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr XGetWMHints(Display display, Window w); // returns XWMHints*

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern void XSetWMHints(Display display, Window w, ref XWMHints wmhints);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr XAllocWMHints();

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public extern static bool XkbSetDetectableAutoRepeat(IntPtr display, bool detectable, out bool supported);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr XCreateColormap(Display display, Window window, IntPtr visual, int alloc);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern Status XGetTransientForHint(Display display, Window w, out Window prop_window_return);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern void XSync(Display display, bool discard);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr XDefaultRootWindow(IntPtr display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern int XBitmapBitOrder(Display display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern IntPtr XCreateImage(Display display, IntPtr visual,
		                                         uint depth, ImageFormat format, int offset, IntPtr data, int width, int height,
		                                         int bitmap_pad, int bytes_per_line);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern void XPutImage(Display display, IntPtr drawable,
		                                    IntPtr gc, IntPtr image, int src_x, int src_y, int dest_x, int dest_y, int width, int height);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern int XLookupString(ref XKeyEvent event_struct, [Out] byte[] buffer_return,
		                                       int bytes_buffer, [Out] KeySym[] keysym_return, IntPtr status_in_out);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern int XRefreshKeyboardMapping(ref XMappingEvent event_map);

		static readonly IntPtr CopyFromParent = IntPtr.Zero;

		public static void SendNetWMMessage(X11WindowInfo window, IntPtr message_type, IntPtr l0, IntPtr l1, IntPtr l2)
		{
			XEvent xev = new XEvent();
			xev.ClientMessageEvent.type = XEventName.ClientMessage;
			xev.ClientMessageEvent.send_event = true;
			xev.ClientMessageEvent.window = window.WindowHandle;
			xev.ClientMessageEvent.message_type = message_type;
			xev.ClientMessageEvent.format = 32;
			xev.ClientMessageEvent.ptr1 = l0;
			xev.ClientMessageEvent.ptr2 = l1;
			xev.ClientMessageEvent.ptr3 = l2;

			XSendEvent(window.Display, window.RootWindow, false, 
			           EventMask.SubstructureRedirectMask | EventMask.SubstructureNotifyMask, ref xev);
		}

		public static IntPtr CreatePixmapFromImage(Display display, System.Drawing.Bitmap image)
		{
			int width = image.Width, height = image.Height;

			System.Drawing.Imaging.BitmapData data = image.LockBits(new System.Drawing.Rectangle(0, 0, width, height),
			                                                        System.Drawing.Imaging.ImageLockMode.ReadOnly,
			                                                        System.Drawing.Imaging.PixelFormat.Format32bppArgb);
			
			IntPtr ximage = XCreateImage(display, CopyFromParent, 24, ImageFormat.ZPixmap,
			                             0, data.Scan0, width, height, 32, 0);
			IntPtr pixmap = XCreatePixmap(display, XDefaultRootWindow(display),
			                              width, height, 24);
			IntPtr gc = XCreateGC(display, pixmap, IntPtr.Zero, null);
			
			XPutImage(display, pixmap, gc, ximage, 0, 0, 0, 0, width, height);
			
			XFreeGC(display, gc);
			image.UnlockBits(data);
			
			return pixmap;
		}
		
		public static IntPtr CreateMaskFromImage(Display display, System.Drawing.Bitmap image)
		{
			int width = image.Width;
			int height = image.Height;
			int stride = (width + 7) >> 3;
			byte[] mask = new byte[stride * height];
			bool msbfirst = (XBitmapBitOrder(display) == 1); // 1 = MSBFirst
			
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					byte bit = (byte) (1 << (msbfirst ? (7 - (x & 7)) : (x & 7)));
					int offset = y * stride + (x >> 3);
					
					if (image.GetPixel(x, y).A >= 128)
						mask[offset] |= bit;
				}
			}
			
			return XCreatePixmapFromBitmapData(display, XDefaultRootWindow(display),
			                                   mask, width, height, new IntPtr(1), IntPtr.Zero, 1);
		}
		
		const string XrandrLibrary = "libXrandr.so.2";

		[DllImport(XrandrLibrary)]
		public static extern XRRScreenConfiguration XRRGetScreenInfo(Display dpy, Drawable draw);

		[DllImport(XrandrLibrary)]
		public static extern void XRRFreeScreenConfigInfo(XRRScreenConfiguration config);

		[DllImport(XrandrLibrary)]
		public static extern Status XRRSetScreenConfig(Display dpy, XRRScreenConfiguration config,
		                                               Drawable draw, int size_index, ref ushort rotation, Time timestamp);

		[DllImport(XrandrLibrary)]
		public static extern Status XRRSetScreenConfigAndRate(Display dpy, XRRScreenConfiguration config,
		                                                      Drawable draw, int size_index, ushort rotation, short rate, Time timestamp);

		[DllImport(XrandrLibrary)]
		public static extern ushort XRRConfigCurrentConfiguration(XRRScreenConfiguration config, out ushort rotation);

		[DllImport(XrandrLibrary)]
		public static extern short XRRConfigCurrentRate(XRRScreenConfiguration config);

		[DllImport(XrandrLibrary)]
		public static extern int XRRRootToScreen(Display dpy, Window root);

		// the following are always safe to call, even if RandR is not implemented on a screen
		[DllImport(XrandrLibrary)]
		unsafe static extern XRRScreenSize* XRRSizes(Display dpy, int screen, int* nsizes);

		public unsafe static XRRScreenSize[] XRRSizes(Display dpy, int screen) {
			int count;
			XRRScreenSize* data = XRRSizes(dpy, screen, &count);
			if (count == 0) return null;
			
			XRRScreenSize[] sizes = new XRRScreenSize[count];
			for (int i = 0; i < count; i++)
				sizes[i] = *data++;
			return sizes;
		}

		[DllImport(XrandrLibrary)]
		unsafe static extern short* XRRRates(Display dpy, int screen, int size_index, int* nrates);

		public unsafe static short[] XRRRates(Display dpy, int screen, int size_index) {
			int count;
			short* data = XRRRates(dpy, screen, size_index, &count);
			if (count == 0) return null;
			
			short[] rates = new short[count];
			for (int i = 0; i < count; i++)
				rates[i] = *data++;
			return rates;
		}

		[DllImport(XrandrLibrary)]
		public static extern Time XRRTimes(Display dpy, int screen, out Time config_timestamp);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern int XScreenCount(Display display);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		unsafe static extern int* XListDepths(Display display, int screen_number, int* count_return);

		public unsafe static int[] XListDepths(Display display, int screen_number) {
			int count;
			int* data = XListDepths(display, screen_number, &count);
			if (count == 0) return null;
			
			int[] depths = new int[count];
			for (int i = 0; i < count; i++)
				depths[i] = *data++;
			return depths;
		}

		public static Display DefaultDisplay;
		internal static int ScreenCount;

		static API() {
			DefaultDisplay = API.XOpenDisplay(IntPtr.Zero);
			if (DefaultDisplay == IntPtr.Zero)
				throw new PlatformException("Could not establish connection to the X-Server.");

			ScreenCount = API.XScreenCount(DefaultDisplay);
			Debug.Print("Display connection: {0}, Screen count: {1}", DefaultDisplay, ScreenCount);

			//AppDomain.CurrentDomain.ProcessExit += new EventHandler(CurrentDomain_ProcessExit);
		}

		static void CurrentDomain_ProcessExit(object sender, EventArgs e) {
			if (DefaultDisplay != IntPtr.Zero) {
				API.XCloseDisplay(DefaultDisplay);
				DefaultDisplay = IntPtr.Zero;
			}
		}

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern KeySym XGetKeyboardMapping(Display display, KeyCode first_keycode, int keycode_count,
		                                                ref int keysyms_per_keycode_return);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern void XDisplayKeycodes(Display display, ref int min_keycodes_return, ref int max_keycodes_return);

		[DllImport("libX11"), SuppressUnmanagedCodeSecurity]
		public static extern KeySym XLookupKeysym(ref XKeyEvent key_event, int index);
	}
}
