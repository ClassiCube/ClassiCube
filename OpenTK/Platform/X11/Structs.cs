// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software",, to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Copyright (c) 2004 Novell, Inc.
//
// Authors:
//    Peter Bartok    pbartok@novell.com
//

using System;
using System.Runtime.InteropServices;

namespace OpenTK.Platform.X11 {
	//
	// In the structures below, fields of type long are mapped to IntPtr.
	// This will work on all platforms where sizeof(long)==sizeof(void*), which
	// is almost all platforms except WIN64.
	//
	
	[StructLayout(LayoutKind.Sequential)]
	public struct XKeyEvent{	
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr window;
		public IntPtr root;
		public IntPtr subwindow;
		public IntPtr time;
		public int x;
		public int y;
		public int x_root;
		public int y_root;
		public int state;
		public int keycode;
		public bool same_screen;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XButtonEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr window;
		public IntPtr root;
		public IntPtr subwindow;
		public IntPtr time;
		public int x;
		public int y;
		public int x_root;
		public int y_root;
		public int state;
		public int button;
		public bool same_screen;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XMotionEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr window;
		public IntPtr root;
		public IntPtr subwindow;
		public IntPtr time;
		public int x;
		public int y;
		public int x_root;
		public int y_root;
		public int state;
		public byte is_hint;
		public bool same_screen;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XConfigureEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr xevent;
		public IntPtr window;
		public int x;
		public int y;
		public int width;
		public int height;
		public int border_width;
		public IntPtr above;
		public bool override_redirect;
	}
	
	[StructLayout(LayoutKind.Sequential)]
    public struct XExposeEvent {
        public XEventName type;
        public IntPtr serial;
        public bool send_event;
        public IntPtr display;
        public IntPtr window;
        public int x;
        public int y;
        public int width;
        public int height;
        public int count;
    }

	[StructLayout(LayoutKind.Sequential)]
	public struct XPropertyEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr window;
		public IntPtr atom;
		public IntPtr time;
		public int state;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XSelectionRequestEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr owner;
		public IntPtr requestor;
		public IntPtr selection;
		public IntPtr target;
		public IntPtr property;
		public IntPtr time;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XSelectionEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr requestor;
		public IntPtr selection;
		public IntPtr target;
		public IntPtr property;
		public IntPtr time;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XClientMessageEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr window;
		public IntPtr message_type;
		public int format;
		public IntPtr ptr1;
		public IntPtr ptr2;
		public IntPtr ptr3;
		public IntPtr ptr4;
		public IntPtr ptr5;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XMappingEvent {
		public XEventName type;
		public IntPtr serial;
		public bool send_event;
		public IntPtr display;
		public IntPtr window;
		public int request;
		public int first_keycode;
		public int count;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XEventPad {
		public IntPtr pad0;
		public IntPtr pad1;
		public IntPtr pad2;
		public IntPtr pad3;
		public IntPtr pad4;
		public IntPtr pad5;
		public IntPtr pad6;
		public IntPtr pad7;
		public IntPtr pad8;
		public IntPtr pad9;
		public IntPtr pad10;
		public IntPtr pad11;
		public IntPtr pad12;
		public IntPtr pad13;
		public IntPtr pad14;
		public IntPtr pad15;
		public IntPtr pad16;
		public IntPtr pad17;
		public IntPtr pad18;
		public IntPtr pad19;
		public IntPtr pad20;
		public IntPtr pad21;
		public IntPtr pad22;
		public IntPtr pad23;
	}

	[StructLayout(LayoutKind.Explicit)]
	public struct XEvent {
		[FieldOffset(0)]
		public XEventName type;
		[FieldOffset(0)]
		public XKeyEvent KeyEvent;
		[FieldOffset(0)]
		public XButtonEvent ButtonEvent;
		[FieldOffset(0)]
		public XMotionEvent MotionEvent;
		[FieldOffset(0)]
		public XConfigureEvent ConfigureEvent;
		[FieldOffset(0)]
		public XExposeEvent ExposeEvent;
		[FieldOffset(0)]
		public XPropertyEvent PropertyEvent;
		[FieldOffset(0)]
		public XSelectionRequestEvent SelectionRequestEvent;
		[FieldOffset(0)]
		public XSelectionEvent SelectionEvent;
		[FieldOffset(0)]
		public XClientMessageEvent ClientMessageEvent;
		[FieldOffset(0)]
		public XMappingEvent MappingEvent;

		//[MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValArray, SizeConst=24)]
		//[ FieldOffset(0) ] public int[] pad;
		[FieldOffset(0)]
		public XEventPad Pad;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XSetWindowAttributes {
		public IntPtr background_pixmap;
		public IntPtr background_pixel;
		public IntPtr border_pixmap;
		public IntPtr border_pixel;
		public Gravity bit_gravity;
		public Gravity win_gravity;
		public int backing_store;
		public IntPtr backing_planes;
		public IntPtr backing_pixel;
		public bool save_under;
		public IntPtr event_mask;
		public IntPtr do_not_propagate_mask;
		public bool override_redirect;
		public IntPtr colormap;
		public IntPtr cursor;
	}

	public enum XEventName {
		KeyPress = 2,
		KeyRelease = 3,
		ButtonPress = 4,
		ButtonRelease = 5,
		MotionNotify = 6,
		EnterNotify = 7,
		LeaveNotify = 8,
		FocusIn = 9,
		FocusOut = 10,
		KeymapNotify = 11,
		Expose = 12,
		GraphicsExpose = 13,
		NoExpose = 14,
		VisibilityNotify = 15,
		CreateNotify = 16,
		DestroyNotify = 17,
		UnmapNotify = 18,
		MapNotify = 19,
		MapRequest = 20,
		ReparentNotify = 21,
		ConfigureNotify = 22,
		ConfigureRequest = 23,
		GravityNotify = 24,
		ResizeRequest = 25,
		CirculateNotify = 26,
		CirculateRequest = 27,
		PropertyNotify = 28,
		SelectionClear = 29,
		SelectionRequest = 30,
		SelectionNotify = 31,
		ColormapNotify = 32,
		ClientMessage = 33,
		MappingNotify = 34,

		LASTEvent
	}

	[Flags]
	public enum SetWindowValuemask
	{
		Nothing = 0,
		BackPixmap = 1,
		BackPixel = 2,
		BorderPixmap = 4,
		BorderPixel = 8,
		BitGravity = 16,
		WinGravity = 32,
		BackingStore = 64,
		BackingPlanes = 128,
		BackingPixel = 256,
		OverrideRedirect = 512,
		SaveUnder = 1024,
		EventMask = 2048,
		DontPropagate = 4096,
		ColorMap = 8192,
		Cursor = 16384
	}

	public enum CreateWindowArgs {
		CopyFromParent = 0,
		ParentRelative = 1,
		InputOutput = 1,
		InputOnly = 2
	}

	public enum Gravity {
		ForgetGravity = 0,
		NorthWestGravity = 1,
		NorthGravity = 2,
		NorthEastGravity = 3,
		WestGravity = 4,
		CenterGravity = 5,
		EastGravity = 6,
		SouthWestGravity = 7,
		SouthGravity = 8,
		SouthEastGravity = 9,
		StaticGravity = 10
	}

	#pragma warning disable 1591

	[Flags]
	public enum EventMask {
		NoEventMask = 0,
		KeyPressMask = 1 << 0,
		KeyReleaseMask = 1 << 1,
		ButtonPressMask = 1 << 2,
		ButtonReleaseMask = 1 << 3,
		EnterWindowMask = 1 << 4,
		LeaveWindowMask = 1 << 5,
		PointerMotionMask = 1 << 6,
		PointerMotionHintMask = 1 << 7,
		Button1MotionMask = 1 << 8,
		Button2MotionMask = 1 << 9,
		Button3MotionMask = 1 << 10,
		Button4MotionMask = 1 << 11,
		Button5MotionMask = 1 << 12,
		ButtonMotionMask = 1 << 13,
		KeymapStateMask = 1 << 14,
		ExposureMask = 1 << 15,
		VisibilityChangeMask = 1 << 16,
		StructureNotifyMask = 1 << 17,
		ResizeRedirectMask = 1 << 18,
		SubstructureNotifyMask = 1 << 19,
		SubstructureRedirectMask = 1 << 20,
		FocusChangeMask = 1 << 21,
		PropertyChangeMask = 1 << 22,
		ColormapChangeMask = 1 << 23,
		OwnerGrabButtonMask = 1 << 24
	}

	#pragma warning restore 1591

	[StructLayout(LayoutKind.Sequential, Pack = 2)]
	public struct XColor {
		public IntPtr pixel;
		public ushort red;
		public ushort green;
		public ushort blue;
		public byte flags;
		public byte pad;
	}

	public enum PropertyMode {
		Replace = 0,
		Prepend = 1,
		Append = 2
	}

	[Flags]
	public enum XSizeHintsFlags {
		PPosition = (1 << 2),
		PSize = (1 << 3),
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XSizeHints {
		public IntPtr flags;
		public int x;
		public int y;
		public int width;
		public int height;
		public int min_width;
		public int min_height;
		public int max_width;
		public int max_height;
		public int width_inc;
		public int height_inc;
		public int min_aspect_x;
		public int min_aspect_y;
		public int max_aspect_x;
		public int max_aspect_y;
		public int base_width;
		public int base_height;
		public int win_gravity;
	}

	[Flags]
	public enum XWMHintsFlags {
		IconPixmapHint = (1 << 2),
		IconMaskHint = (1 << 5),
	}

	public enum XInitialState {
		DontCareState = 0,
		NormalState = 1,
		ZoomState = 2,
		IconicState = 3,
		InactiveState = 4
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct XWMHints
	{
		public IntPtr flags;
		public bool input;
		public XInitialState initial_state;
		public IntPtr icon_pixmap;
		public IntPtr icon_window;
		public int icon_x;
		public int icon_y;
		public IntPtr icon_mask;
		public IntPtr window_group;
	}

	public enum ImageFormat {
		XYPixmap = 1,
		ZPixmap
	}
}
