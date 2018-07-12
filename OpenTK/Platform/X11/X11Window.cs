#region License
//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2009 the Open Toolkit library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
#endregion

using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using OpenTK.Graphics;
using OpenTK.Input;

namespace OpenTK.Platform.X11 {
	
	public sealed class X11Window : INativeWindow, IDisposable {
		// TODO: Disable screensaver.
		// TODO: What happens if we can't disable decorations through motif?
		// TODO: Mouse/keyboard grabbing/wrapping.
		
		const int _min_width = 30, _min_height = 30;
		IntPtr wm_destroy;
		IntPtr net_wm_state;
		IntPtr net_wm_state_minimized;
		IntPtr net_wm_state_fullscreen;
		IntPtr net_wm_state_maximized_horizontal;
		IntPtr net_wm_state_maximized_vertical;
		
		IntPtr net_wm_icon, net_frame_extents;

		IntPtr xa_clipboard, xa_targets, xa_utf8_string, xa_atom, xa_data_sel;
		string clipboard_paste_text, clipboard_copy_text;

		static readonly IntPtr xa_cardinal = (IntPtr)6;
		static readonly IntPtr _remove = (IntPtr)0;
		static readonly IntPtr _add = (IntPtr)1;
		static readonly IntPtr _toggle = (IntPtr)2;
		
		Rectangle bounds, client_rectangle;
		int borderLeft, borderRight, borderTop, borderBottom;
		Icon icon;
		bool has_focus, visible;
		EventMask eventMask;
		
		public XVisualInfo VisualInfo;
		// Used for event loop.
		XEvent e = new XEvent();
		bool disposed, exists, isExiting;

		// Keyboard input
		readonly byte[] ascii = new byte[16];
		readonly char[] chars = new char[16];

		X11KeyMap keymap = new X11KeyMap();
		int firstKeyCode, lastKeyCode; // The smallest and largest KeyCode supported by the X server.
		int keysyms_per_keycode;    // The number of KeySyms for each KeyCode.
		IntPtr[] keysyms;
		
		public X11Window(int x, int y, int width, int height, string title, GraphicsMode mode, DisplayDevice device) {
			Debug.Print("Creating X11GLNative window.");
			// Open a display connection to the X server, and obtain the screen and root window.
			Debug.Print("Display: {0}, Screen {1}, Root window: {2}",
			            API.DefaultDisplay, API.DefaultScreen, API.RootWindow);
			
			RegisterAtoms();
			XVisualInfo info = new XVisualInfo();
			mode = X11GLContext.SelectGraphicsMode(mode, out info);
			VisualInfo = info;
			// Create a window on this display using the visual above
			Debug.Print("Opening render window... ");

			XSetWindowAttributes attributes = new XSetWindowAttributes();
			attributes.background_pixel = IntPtr.Zero;
			attributes.border_pixel = IntPtr.Zero;
			attributes.colormap = API.XCreateColormap(API.DefaultDisplay, API.RootWindow, VisualInfo.Visual, 0/*AllocNone*/);
			
			eventMask = EventMask.StructureNotifyMask /*| EventMask.SubstructureNotifyMask*/ | EventMask.ExposureMask |
				EventMask.KeyReleaseMask | EventMask.KeyPressMask | EventMask.KeymapStateMask |
				EventMask.PointerMotionMask | EventMask.FocusChangeMask |
				EventMask.ButtonPressMask | EventMask.ButtonReleaseMask |
				EventMask.EnterWindowMask | EventMask.LeaveWindowMask |
				EventMask.PropertyChangeMask;
			attributes.event_mask = (IntPtr)eventMask;

			uint mask =
				(uint)SetWindowValuemask.ColorMap  | (uint)SetWindowValuemask.EventMask |
				(uint)SetWindowValuemask.BackPixel | (uint)SetWindowValuemask.BorderPixel;

			WinHandle = API.XCreateWindow(API.DefaultDisplay, API.RootWindow,
			                              x, y, width, height, 0, VisualInfo.Depth/*(int)CreateWindowArgs.CopyFromParent*/,
			                              (int)CreateWindowArgs.InputOutput, VisualInfo.Visual, (IntPtr)mask, ref attributes);

			if (WinHandle == IntPtr.Zero)
				throw new ApplicationException("XCreateWindow call failed (returned 0).");

			if (title != null)
				API.XStoreName(API.DefaultDisplay, WinHandle, title);

			// Set the window hints
			SetWindowMinMax(_min_width, _min_height, -1, -1);
			
			XSizeHints hints = new XSizeHints();
			hints.base_width = width;
			hints.base_height = height;
			hints.flags = (IntPtr)(XSizeHintsFlags.PSize | XSizeHintsFlags.PPosition);
			API.XSetWMNormalHints(API.DefaultDisplay, WinHandle, ref hints);
			// Register for window destroy notification
			API.XSetWMProtocols(API.DefaultDisplay, WinHandle, new IntPtr[] { wm_destroy }, 1);

			// Set the initial window size to ensure X, Y, Width, Height and the rest
			// return the correct values inside the constructor and the Load event.
			XEvent e = new XEvent();
			e.ConfigureEvent.x = x;
			e.ConfigureEvent.y = y;
			e.ConfigureEvent.width = width;
			e.ConfigureEvent.height = height;
			RefreshWindowBounds(ref e);

			Debug.Print("X11GLNative window created successfully (id: {0}).", WinHandle);
			SetupInput();
			exists = true;
		}
		
		void SetupInput() {
			Debug.Print("Initalizing X11 input driver.");
			
			// Init keyboard
			API.XDisplayKeycodes(API.DefaultDisplay, ref firstKeyCode, ref lastKeyCode);
			Debug.Print("First keycode: {0}, last {1}", firstKeyCode, lastKeyCode);
			
			IntPtr keysym_ptr = API.XGetKeyboardMapping(API.DefaultDisplay, (byte)firstKeyCode,
			                                            lastKeyCode - firstKeyCode + 1, ref keysyms_per_keycode);
			Debug.Print("{0} keysyms per keycode.", keysyms_per_keycode);
			
			keysyms = new IntPtr[(lastKeyCode - firstKeyCode + 1) * keysyms_per_keycode];
			Marshal.PtrToStructure(keysym_ptr, keysyms);
			API.XFree(keysym_ptr);
			
			// Request that auto-repeat is only set on devices that support it physically.
			// This typically means that it's turned off for keyboards (which is what we want).
			// We prefer this method over XAutoRepeatOff/On, because the latter needs to
			// be reset before the program exits.
			bool supported;
			API.XkbSetDetectableAutoRepeat(API.DefaultDisplay, true, out supported);
		}
		
		void RegisterAtoms() {
			IntPtr display = API.DefaultDisplay;
			wm_destroy = API.XInternAtom(display, "WM_DELETE_WINDOW", true);
			net_wm_state = API.XInternAtom(display, "_NET_WM_STATE", false);
			net_wm_state_minimized = API.XInternAtom(display, "_NET_WM_STATE_MINIMIZED", false);
			net_wm_state_fullscreen = API.XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", false);
			net_wm_state_maximized_horizontal = API.XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
			net_wm_state_maximized_vertical = API.XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
			net_wm_icon = API.XInternAtom(display, "_NEW_WM_ICON", false);
			net_frame_extents = API.XInternAtom(display, "_NET_FRAME_EXTENTS", false);

			xa_clipboard = API.XInternAtom(display, "CLIPBOARD", false);
			xa_targets = API.XInternAtom(display, "TARGETS", false);
			xa_utf8_string = API.XInternAtom(display, "UTF8_STRING", false);
			xa_atom = API.XInternAtom(display, "ATOM", false);
			xa_data_sel = API.XInternAtom(display, "CS_SEL_DATA", false);
		}

		void SetWindowMinMax(short min_width, short min_height, short max_width, short max_height) {
			IntPtr dummy;
			XSizeHints hints = new XSizeHints();
			API.XGetWMNormalHints(API.DefaultDisplay, WinHandle, ref hints, out dummy);

			if (min_width > 0 || min_height > 0) {
				hints.flags = (IntPtr)((int)hints.flags | (int)XSizeHintsFlags.PMinSize);
				hints.min_width = min_width;
				hints.min_height = min_height;
			} else
				hints.flags = (IntPtr)((int)hints.flags & ~(int)XSizeHintsFlags.PMinSize);

			if (max_width > 0 || max_height > 0) {
				hints.flags = (IntPtr)((int)hints.flags | (int)XSizeHintsFlags.PMaxSize);
				hints.max_width = max_width;
				hints.max_height = max_height;
			} else
				hints.flags = (IntPtr)((int)hints.flags & ~(int)XSizeHintsFlags.PMaxSize);

			if (hints.flags != IntPtr.Zero) {
				// The Metacity team has decided that they won't care about this when clicking the maximize
				// icon, will maximize the window to fill the screen/parent no matter what.
				// http://bugzilla.ximian.com/show_bug.cgi?id=80021
				API.XSetWMNormalHints(API.DefaultDisplay, WinHandle, ref hints);
			}
		}
		
		void ToggleKey(ref XKeyEvent keyEvent, bool pressed) {
			int keysym  = (int)API.XLookupKeysym(ref keyEvent, 0);
			int keysym2 = (int)API.XLookupKeysym(ref keyEvent, 1);
			Key tkKey;
			
			if (keymap.TryGetValue(keysym, out tkKey)) {
				Keyboard.Set(tkKey, pressed);
			} else if (keymap.TryGetValue(keysym2, out tkKey)) {
				Keyboard.Set(tkKey, pressed);
			} else {
				Debug.Print("KeyCode {0} (Keysym: {1}, {2}) not mapped.",
				            e.KeyEvent.keycode, keysym, keysym2);
			}
		}
		
		static void DeleteIconPixmaps(IntPtr display, IntPtr window) {
			IntPtr wmHints_ptr = API.XGetWMHints(display, window);			
			if (wmHints_ptr == IntPtr.Zero) return;
			
			XWMHints wmHints = (XWMHints)Marshal.PtrToStructure(wmHints_ptr, typeof(XWMHints));
			XWMHintsFlags flags = (XWMHintsFlags)wmHints.flags.ToInt32();
			
			if ((flags & XWMHintsFlags.IconPixmapHint) != 0) {
				wmHints.flags = new IntPtr((int)(flags & ~XWMHintsFlags.IconPixmapHint));
				API.XFreePixmap(display, wmHints.icon_pixmap);
			}			
			if ((flags & XWMHintsFlags.IconMaskHint) != 0) {
				wmHints.flags = new IntPtr((int)(flags & ~XWMHintsFlags.IconMaskHint));
				API.XFreePixmap(display, wmHints.icon_mask);
			}
			
			API.XSetWMHints(display, window, ref wmHints);
			API.XFree(wmHints_ptr);

		}
		
		void RefreshWindowBorders() {
			IntPtr atom, nitems, bytes_after, prop = IntPtr.Zero;
			int format;
			API.XGetWindowProperty(API.DefaultDisplay, WinHandle,
			                       net_frame_extents, IntPtr.Zero, new IntPtr(16), false,
			                       xa_cardinal, out atom, out format, out nitems, out bytes_after, ref prop);

			if (prop != IntPtr.Zero) {
				if ((long)nitems == 4) {
					borderLeft = Marshal.ReadIntPtr(prop, 0).ToInt32();
					borderRight = Marshal.ReadIntPtr(prop, IntPtr.Size).ToInt32();
					borderTop = Marshal.ReadIntPtr(prop, IntPtr.Size * 2).ToInt32();
					borderBottom = Marshal.ReadIntPtr(prop, IntPtr.Size * 3).ToInt32();
				}
				API.XFree(prop);
			}
		}

		void RefreshWindowBounds(ref XEvent e) {
			RefreshWindowBorders();

			Point newLoc = new Point(e.ConfigureEvent.x - borderLeft, e.ConfigureEvent.y - borderTop);
			if (Location != newLoc) {
				bounds.Location = newLoc;
				RaiseMove();
			}

			// Note: width and height denote the internal (client) size.
			// To get the external (window) size, we need to add the border size.
			Size newSize = new Size(
				e.ConfigureEvent.width + borderLeft + borderRight,
				e.ConfigureEvent.height + borderTop + borderBottom);
			
			if (bounds.Size != newSize) {
				bounds.Size = newSize;
				client_rectangle.Size = new Size(e.ConfigureEvent.width, e.ConfigureEvent.height);
				RaiseResize();
			}
		}

		bool GetPendingEvent() {
			return API.XCheckWindowEvent(API.DefaultDisplay, WinHandle, eventMask, ref e) ||
				API.XCheckTypedWindowEvent(API.DefaultDisplay, WinHandle, XEventName.ClientMessage, ref e) ||
				API.XCheckTypedWindowEvent(API.DefaultDisplay, WinHandle, XEventName.SelectionNotify, ref e) ||
				API.XCheckTypedWindowEvent(API.DefaultDisplay, WinHandle, XEventName.SelectionRequest, ref e);
		}
		
		public override unsafe void ProcessEvents() {
			// Process all pending events
			while (Exists) {
				if (!GetPendingEvent ()) break;
				
				// Respond to the event e
				switch (e.type) {
					case XEventName.MapNotify:
					case XEventName.UnmapNotify:
						{
							bool previous_visible = visible;
							visible = e.type == XEventName.MapNotify;
							if (visible != previous_visible)
								RaiseVisibleChanged();
						} break;

					case XEventName.CreateNotify:
						// A child was was created - nothing to do
						break;

					case XEventName.ClientMessage:
						if (!isExiting && e.ClientMessageEvent.ptr1 == wm_destroy) {
							Debug.Print("Exit message received.");
							RaiseClosing();
							
							isExiting = true;
							DestroyWindow();
							RaiseClosed();
						} break;

					case XEventName.DestroyNotify:
						Debug.Print("Window destroyed");
						exists = false;

						break;

					case XEventName.ConfigureNotify:
						RefreshWindowBounds(ref e);
						break;
						
					case XEventName.KeyPress:
						ToggleKey(ref e.KeyEvent, true);
						int status = API.XLookupString(ref e.KeyEvent, ascii, ascii.Length, null, IntPtr.Zero);
						Encoding.Default.GetChars(ascii, 0, status, chars, 0);

						for (int i = 0; i < status; i++) {
							// ignore NULL char after non-ASCII input char, like ä or å on Finnish keyboard layout
							if (chars[i] == '\0') continue;
							RaiseKeyPress(chars[i]);
						}
						break;
						
					case XEventName.KeyRelease:
						// Todo: raise KeyPress event. Use code from
						// http://anonsvn.mono-project.com/viewvc/trunk/mcs/class/Managed.Windows.Forms/System.Windows.Forms/X11Keyboard.cs?view=markup
						ToggleKey(ref e.KeyEvent, false);
						break;
						
					case XEventName.ButtonPress:
						if      (e.ButtonEvent.button == 1) Mouse.Set(MouseButton.Left, true);
						else if (e.ButtonEvent.button == 2) Mouse.Set(MouseButton.Middle, true);
						else if (e.ButtonEvent.button == 3) Mouse.Set(MouseButton.Right, true);
						else if (e.ButtonEvent.button == 4) Mouse.SetWheel(Mouse.Wheel + 1);
						else if (e.ButtonEvent.button == 5) Mouse.SetWheel(Mouse.Wheel - 1);
						else if (e.ButtonEvent.button == 6) Keyboard.Set(Key.XButton1, true);
						else if (e.ButtonEvent.button == 7) Keyboard.Set(Key.XButton2, true);
						break;

					case XEventName.ButtonRelease:
						if      (e.ButtonEvent.button == 1) Mouse.Set(MouseButton.Left, false);
						else if (e.ButtonEvent.button == 2) Mouse.Set(MouseButton.Middle, false);
						else if (e.ButtonEvent.button == 3) Mouse.Set(MouseButton.Right, false);
						else if (e.ButtonEvent.button == 6) Keyboard.Set(Key.XButton1, false);
						else if (e.ButtonEvent.button == 7) Keyboard.Set(Key.XButton2, false);
						break;

					case XEventName.MotionNotify:
						Mouse.SetPos(e.MotionEvent.x, e.MotionEvent.y);
						break;

					case XEventName.FocusIn:
					case XEventName.FocusOut:
						{
							bool previous_focus = has_focus;
							has_focus = e.type == XEventName.FocusIn;
							if (has_focus != previous_focus)
								RaiseFocusedChanged();
						} break;

					case XEventName.MappingNotify:
						// 0 == MappingModifier, 1 == MappingKeyboard
						if (e.MappingEvent.request == 0 || e.MappingEvent.request == 1) {
							Debug.Print("keybard mapping refreshed");
							API.XRefreshKeyboardMapping(ref e.MappingEvent);
						}
						break;

					case XEventName.PropertyNotify:
						if (e.PropertyEvent.atom == net_wm_state) {
							RaiseWindowStateChanged();
						}

						//if (e.PropertyEvent.atom == net_frame_extents) {
						//    RefreshWindowBorders();
						//}
						break;

					case XEventName.SelectionNotify:
						clipboard_paste_text = "";

						if (e.SelectionEvent.selection == xa_clipboard && e.SelectionEvent.target == xa_utf8_string && e.SelectionEvent.property == xa_data_sel) {
							IntPtr prop_type, num_items, bytes_after, data = IntPtr.Zero;
							int prop_format;

							API.XGetWindowProperty(API.DefaultDisplay, WinHandle, xa_data_sel, IntPtr.Zero, new IntPtr (1024), false, IntPtr.Zero,
							                       out prop_type, out prop_format, out num_items, out bytes_after, ref data);

							API.XDeleteProperty(API.DefaultDisplay, WinHandle, xa_data_sel);
							if (num_items == IntPtr.Zero) break;

							if (prop_type == xa_utf8_string) {
								byte[] dst = new byte[(int)num_items];
								byte* src = (byte*)data;
								for (int i = 0; i < dst.Length; i++) { dst[i] = src[i]; }

								clipboard_paste_text = Encoding.UTF8.GetString(dst);
							}
							API.XFree (data);
						}
						break;

					case XEventName.SelectionRequest:
						XEvent reply = default(XEvent);
						reply.SelectionEvent.type = XEventName.SelectionNotify;
						reply.SelectionEvent.send_event = true;
						reply.SelectionEvent.display = API.DefaultDisplay;
						reply.SelectionEvent.requestor = e.SelectionRequestEvent.requestor;
						reply.SelectionEvent.selection = e.SelectionRequestEvent.selection;
						reply.SelectionEvent.target = e.SelectionRequestEvent.target;
						reply.SelectionEvent.property = IntPtr.Zero;
						reply.SelectionEvent.time = e.SelectionRequestEvent.time;

						if (e.SelectionRequestEvent.selection == xa_clipboard && e.SelectionRequestEvent.target == xa_utf8_string && clipboard_copy_text != null) {
							reply.SelectionEvent.property = GetSelectionProperty(ref e);

							byte[] utf8_data = Encoding.UTF8.GetBytes(clipboard_copy_text);
							fixed (byte* utf8_ptr = utf8_data) {
								API.XChangeProperty(API.DefaultDisplay, reply.SelectionEvent.requestor, reply.SelectionEvent.property, xa_utf8_string, 8,
								                    PropertyMode.Replace, (IntPtr)utf8_ptr, utf8_data.Length);
							}
						} else if (e.SelectionRequestEvent.selection == xa_clipboard && e.SelectionRequestEvent.target == xa_targets) {
							reply.SelectionEvent.property = GetSelectionProperty(ref e);

							IntPtr[] data = new IntPtr[] { xa_utf8_string, xa_targets };
							API.XChangeProperty(API.DefaultDisplay, reply.SelectionEvent.requestor, reply.SelectionEvent.property, xa_atom, 32,
							                    PropertyMode.Replace, data, data.Length);
						}
						
						API.XSendEvent(API.DefaultDisplay, e.SelectionRequestEvent.requestor, true, EventMask.NoEventMask, ref reply);
						break;
						
					default:
						//Debug.WriteLine(String.Format("{0} event was not handled", e.type));
						break;
				}
			}
		}

		IntPtr GetSelectionProperty(ref XEvent e) {
			IntPtr property  = e.SelectionRequestEvent.property;
			if (property != IntPtr.Zero) return property;

			/* For obsolete clients. See ICCCM spec, selections chapter for reasoning. */
			return e.SelectionRequestEvent.target;
		}
		
		public override string GetClipboardText() {
			IntPtr owner = API.XGetSelectionOwner(API.DefaultDisplay, xa_clipboard);
			if (owner == IntPtr.Zero) return ""; // no window owner

			API.XConvertSelection(API.DefaultDisplay, xa_clipboard, xa_utf8_string, xa_data_sel, WinHandle, IntPtr.Zero);
			clipboard_paste_text = null;

			// wait up to 1 second for SelectionNotify event to arrive
			for (int i = 0; i < 10; i++) {
				ProcessEvents ();
				if (clipboard_paste_text != null) return clipboard_paste_text;
				Thread.Sleep(100);
			}
			return "";
		}
		
		public override void SetClipboardText(string value) {
			clipboard_copy_text = value;
			API.XSetSelectionOwner(API.DefaultDisplay, xa_clipboard, WinHandle, IntPtr.Zero);
		}
		
		public override Rectangle Bounds {
			get { return bounds; }
			set {
				API.XMoveResizeWindow(
					API.DefaultDisplay, WinHandle, value.X, value.Y,
					value.Width - borderLeft - borderRight, value.Height - borderTop - borderBottom);
				ProcessEvents();
			}
		}
		
		public override Point Location {
			get { return Bounds.Location; }
			set {
				API.XMoveWindow(API.DefaultDisplay, WinHandle, value.X, value.Y);
				ProcessEvents();
			}
		}
		
		public override Size Size {
			get { return Bounds.Size; }
			set {
				int width = value.Width - borderLeft - borderRight;
				int height = value.Height - borderTop - borderBottom;
				width = width <= 0 ? 1 : width;
				height = height <= 0 ? 1 : height;
				API.XResizeWindow(API.DefaultDisplay, WinHandle, width, height);
				ProcessEvents();
			}
		}
		
		public override Rectangle ClientRectangle {
			get {
				if (client_rectangle.Width == 0)
					client_rectangle.Width = 1;
				if (client_rectangle.Height == 0)
					client_rectangle.Height = 1;
				return client_rectangle;
			}
			set {
				API.XResizeWindow(API.DefaultDisplay, WinHandle, value.Width, value.Height);
				ProcessEvents();
			}
		}
		
		public override Size ClientSize {
			get { return ClientRectangle.Size; }
			set { ClientRectangle = new Rectangle(Point.Empty, value); }
		}
		
		public override Icon Icon {
			get { return icon; }
			set {
				if (value == icon)
					return;

				// Note: it seems that Gnome/Metacity does not respect the _NET_WM_ICON hint.
				// For this reason, we'll also set the icon using XSetWMHints.
				if (value == null) {
					API.XDeleteProperty(API.DefaultDisplay, WinHandle, net_wm_icon);
					DeleteIconPixmaps(API.DefaultDisplay, WinHandle);
				} else {
					// Set _NET_WM_ICON
					System.Drawing.Bitmap bitmap = value.ToBitmap();
					int size = bitmap.Width * bitmap.Height + 2;
					IntPtr[] data = new IntPtr[size];
					int index = 0;
					
					data[index++] = (IntPtr)bitmap.Width;
					data[index++] = (IntPtr)bitmap.Height;
					
					for (int y = 0; y < bitmap.Height; y++)
						for (int x = 0; x < bitmap.Width; x++)
							data[index++] = (IntPtr)bitmap.GetPixel(x, y).ToArgb();
					
					API.XChangeProperty(API.DefaultDisplay, WinHandle,
					                    net_wm_icon, xa_cardinal, 32,
					                    PropertyMode.Replace, data, size);

					// Set XWMHints
					DeleteIconPixmaps(API.DefaultDisplay, WinHandle);
					IntPtr wmHints_ptr = API.XGetWMHints(API.DefaultDisplay, WinHandle);
					
					if (wmHints_ptr == IntPtr.Zero)
						wmHints_ptr = API.XAllocWMHints();
					
					XWMHints wmHints = (XWMHints)Marshal.PtrToStructure(wmHints_ptr, typeof(XWMHints));
					
					wmHints.flags = new IntPtr(wmHints.flags.ToInt32() | (int)(XWMHintsFlags.IconPixmapHint | XWMHintsFlags.IconMaskHint));
					wmHints.icon_pixmap = API.CreatePixmapFromImage(API.DefaultDisplay, bitmap);
					wmHints.icon_mask = API.CreateMaskFromImage(API.DefaultDisplay, bitmap);
					
					API.XSetWMHints(API.DefaultDisplay, WinHandle, ref wmHints);
					API.XFree(wmHints_ptr);
					API.XSync(API.DefaultDisplay, false);
				}

				icon = value;
			}
		}
		
		public override bool Focused {
			get { return has_focus; }
		}
		
		public override WindowState WindowState {
			get {
				IntPtr actual_atom, nitems, bytes_after, prop = IntPtr.Zero;
				int actual_format;
				bool fullscreen = false, minimised = false;
				int maximised = 0;
				API.XGetWindowProperty(API.DefaultDisplay, WinHandle,
				                       net_wm_state, IntPtr.Zero, new IntPtr(256), false,
				                       new IntPtr(4) /*XA_ATOM*/, out actual_atom, out actual_format,
				                       out nitems, out bytes_after, ref prop);

				if ((long)nitems > 0 && prop != IntPtr.Zero) {
					for (int i = 0; i < (long)nitems; i++) {
						IntPtr atom = (IntPtr)Marshal.ReadIntPtr(prop, i * IntPtr.Size);

						if (atom == net_wm_state_maximized_horizontal ||
						    atom == net_wm_state_maximized_vertical)
							maximised++;
						else if (atom == net_wm_state_minimized)
							minimised = true;
						else if (atom == net_wm_state_fullscreen)
							fullscreen = true;
					}
					API.XFree(prop);
				}

				if (minimised)
					return OpenTK.WindowState.Minimized;
				else if (maximised == 2)
					return OpenTK.WindowState.Maximized;
				else if (fullscreen)
					return OpenTK.WindowState.Fullscreen;
				/*
                                attributes = new XWindowAttributes();
                                Functions.XGetWindowAttributes(API.DefaultDisplay, window.WindowHandle, ref attributes);
                                if (attributes.map_state == MapState.IsUnmapped)
                                    return (OpenTK.WindowState)(-1);
				 */
				return WindowState.Normal;
			}
			set {
				WindowState current_state = this.WindowState;

				if (current_state == value)
					return;

				Debug.Print("GameWindow {0} changing WindowState from {1} to {2}.", WinHandle.ToString(),
				            current_state.ToString(), value.ToString());

				// Reset the current window state
				if (current_state == OpenTK.WindowState.Minimized)
					API.XMapWindow(API.DefaultDisplay, WinHandle);
				else if (current_state == OpenTK.WindowState.Fullscreen)
					API.SendNetWMMessage(this, net_wm_state, _remove,
					                     net_wm_state_fullscreen,
					                     IntPtr.Zero);
				else if (current_state == OpenTK.WindowState.Maximized)
					API.SendNetWMMessage(this, net_wm_state, _toggle,
					                     net_wm_state_maximized_horizontal,
					                     net_wm_state_maximized_vertical);
				
				API.XSync(API.DefaultDisplay, false);

				switch (value) {
					case OpenTK.WindowState.Normal:
						API.XRaiseWindow(API.DefaultDisplay, WinHandle);
						break;
						
					case OpenTK.WindowState.Maximized:
						API.SendNetWMMessage(this, net_wm_state, _add,
						                     net_wm_state_maximized_horizontal,
						                     net_wm_state_maximized_vertical);
						API.XRaiseWindow(API.DefaultDisplay, WinHandle);
						break;
						
					case OpenTK.WindowState.Minimized:
						// Todo: multiscreen support
						API.XIconifyWindow(API.DefaultDisplay, WinHandle, API.DefaultScreen);
						break;
						
					case OpenTK.WindowState.Fullscreen:
						API.SendNetWMMessage(this, net_wm_state, _add,
						                     net_wm_state_fullscreen, IntPtr.Zero);
						API.XRaiseWindow(API.DefaultDisplay, WinHandle);
						break;
				}
				ProcessEvents();
			}
		}

		public override Point DesktopCursorPos {
			get {
				IntPtr root, child;
				int rootX, rootY, childX, childY, mask;
				API.XQueryPointer(API.DefaultDisplay, API.RootWindow, out root, out child, out rootX, out rootY, out childX, out childY, out mask);
				return new Point(rootX, rootY);
			}
			set {
				API.XWarpPointer(API.DefaultDisplay, IntPtr.Zero, API.RootWindow, 0, 0, 0, 0, value.X, value.Y);
				API.XFlush(API.DefaultDisplay); // TODO: not sure if XFlush call is necessary
			}
		}
		
		bool cursorVisible = true;
		public override bool CursorVisible {
			get { return cursorVisible; }
			set {
				cursorVisible = value;
				if (value) {
					API.XUndefineCursor(API.DefaultDisplay, WinHandle);
				} else {
					if (blankCursor == IntPtr.Zero)
						MakeBlankCursor();
					API.XDefineCursor(API.DefaultDisplay, WinHandle, blankCursor);
				}
			}
		}
		
		IntPtr blankCursor;
		void MakeBlankCursor() {
			XColor color = default(XColor);
			IntPtr pixmap = API.XCreatePixmap(API.DefaultDisplay, API.RootWindow, 1, 1, 1);
			blankCursor = API.XCreatePixmapCursor(API.DefaultDisplay, pixmap, pixmap, ref color, ref color, 0, 0);
			API.XFreePixmap(API.DefaultDisplay, pixmap);
		}
		
		/// <summary> Returns true if a render window/context exists. </summary>
		public override bool Exists {
			get { return exists; }
		}
		
		public override bool Visible {
			get { return visible; }
			set {
				if (value && !visible) {
					API.XMapWindow(API.DefaultDisplay, WinHandle);
				} else if (!value && visible) {
					API.XUnmapWindow(API.DefaultDisplay, WinHandle);
				}
			}
		}

		public override void Close() {
			XEvent ev = new XEvent();
			ev.type = XEventName.ClientMessage;
			ev.ClientMessageEvent.format = 32;
			ev.ClientMessageEvent.display = API.DefaultDisplay;
			ev.ClientMessageEvent.window = WinHandle;
			ev.ClientMessageEvent.ptr1 = wm_destroy;
			API.XSendEvent(API.DefaultDisplay, WinHandle, false,
			               EventMask.NoEventMask, ref ev);
			API.XFlush(API.DefaultDisplay);
		}

		void DestroyWindow() {
			Debug.Print("X11GLNative shutdown sequence initiated.");
			API.XSync(API.DefaultDisplay, true);
			API.XDestroyWindow(API.DefaultDisplay, WinHandle);
			exists = false;
		}

		public override Point PointToClient(Point point) {
			int ox, oy;
			IntPtr child;
			API.XTranslateCoordinates(API.DefaultDisplay, API.RootWindow, WinHandle, point.X, point.Y, out ox, out oy, out child);
			return new Point(ox, oy);
		}

		public override Point PointToScreen(Point point) {
			int ox, oy;
			IntPtr child;
			API.XTranslateCoordinates(API.DefaultDisplay, WinHandle, API.RootWindow, point.X, point.Y, out ox, out oy, out child);
			return new Point(ox, oy);
		}

		protected override void Dispose(bool manuallyCalled) {
			if (disposed) return;
			
			if (manuallyCalled) {
				if (WinHandle != IntPtr.Zero) {
					if (Exists) DestroyWindow();
					WinHandle = IntPtr.Zero;
				}
			} else {
				Debug.Print("=== [Warning] INativeWindow leaked ===");
			}
			disposed = true;
		}
	}
}