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
using System.ComponentModel;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;
using OpenTK.Graphics;
using OpenTK.Input;

namespace OpenTK.Platform.X11 {
	
	/// \internal
	/// <summary> Drives GameWindow on X11.
	/// This class supports OpenTK, and is not intended for use by OpenTK programs. </summary>
	internal sealed class X11GLNative : INativeWindow, IDisposable {
		// TODO: Disable screensaver.
		// TODO: What happens if we can't disable decorations through motif?
		// TODO: Mouse/keyboard grabbing/wrapping.
		
		const int _min_width = 30, _min_height = 30;
		X11WindowInfo window = new X11WindowInfo();
		// The Atom class from Mono might be useful to avoid calling XInternAtom by hand (somewhat error prone).
		IntPtr wm_destroy;
		
		IntPtr net_wm_state;
		IntPtr net_wm_state_minimized;
		IntPtr net_wm_state_fullscreen;
		IntPtr net_wm_state_maximized_horizontal;
		IntPtr net_wm_state_maximized_vertical;

		IntPtr net_wm_icon;
		IntPtr net_frame_extents;

		static readonly IntPtr xa_cardinal = (IntPtr)6;
		static readonly IntPtr _remove = (IntPtr)0;
		static readonly IntPtr _add = (IntPtr)1;
		static readonly IntPtr _toggle = (IntPtr)2;
		
		Rectangle bounds, client_rectangle;
		int borderLeft, borderRight, borderTop, borderBottom;
		Icon icon;
		bool has_focus;
		bool visible;

		// Used for event loop.
		XEvent e = new XEvent();
		bool disposed, exists, isExiting;

		// Keyboard input
		readonly byte[] ascii = new byte[16];
		readonly char[] chars = new char[16];
		readonly KeyPressEventArgs KPEventArgs = new KeyPressEventArgs();
		
		KeyboardDevice keyboard = new KeyboardDevice();
		MouseDevice mouse = new MouseDevice();

		X11KeyMap keymap = new X11KeyMap();
		int firstKeyCode, lastKeyCode; // The smallest and largest KeyCode supported by the X server.
		int keysyms_per_keycode;    // The number of KeySyms for each KeyCode.
		IntPtr[] keysyms;
		
		public X11GLNative(int x, int y, int width, int height, string title,
		                   GraphicsMode mode, GameWindowFlags options, DisplayDevice device) {
			if (width <= 0)
				throw new ArgumentOutOfRangeException("width", "Must be higher than zero.");
			if (height <= 0)
				throw new ArgumentOutOfRangeException("height", "Must be higher than zero.");

			Debug.Print("Creating X11GLNative window.");
			// Open a display connection to the X server, and obtain the screen and root window.
			window.Display = API.DefaultDisplay;
			window.Screen = API.XDefaultScreen(window.Display); //API.DefaultScreen;
			window.RootWindow = API.XRootWindow(window.Display, window.Screen); // API.RootWindow;

			Debug.Print("Display: {0}, Screen {1}, Root window: {2}", window.Display, window.Screen, window.RootWindow);
			RegisterAtoms(window);
			XVisualInfo info = new XVisualInfo();	
			mode = X11GLContext.SelectGraphicsMode( mode, out info );
			window.VisualInfo = info;
			// Create a window on this display using the visual above
			Debug.Print("Opening render window... ");

			XSetWindowAttributes attributes = new XSetWindowAttributes();
			attributes.background_pixel = IntPtr.Zero;
			attributes.border_pixel = IntPtr.Zero;
			attributes.colormap = API.XCreateColormap(window.Display, window.RootWindow, window.VisualInfo.Visual, 0/*AllocNone*/);
			window.EventMask = EventMask.StructureNotifyMask /*| EventMask.SubstructureNotifyMask*/ | EventMask.ExposureMask |
				EventMask.KeyReleaseMask | EventMask.KeyPressMask | EventMask.KeymapStateMask |
				EventMask.PointerMotionMask | EventMask.FocusChangeMask |
				EventMask.ButtonPressMask | EventMask.ButtonReleaseMask |
				EventMask.EnterWindowMask | EventMask.LeaveWindowMask |
				EventMask.PropertyChangeMask;
			attributes.event_mask = (IntPtr)window.EventMask;

			uint mask = (uint)SetWindowValuemask.ColorMap | (uint)SetWindowValuemask.EventMask |
				(uint)SetWindowValuemask.BackPixel | (uint)SetWindowValuemask.BorderPixel;

			window.WindowHandle = API.XCreateWindow(window.Display, window.RootWindow,
			                                        x, y, width, height, 0, window.VisualInfo.Depth/*(int)CreateWindowArgs.CopyFromParent*/,
			                                        (int)CreateWindowArgs.InputOutput, window.VisualInfo.Visual, (IntPtr)mask, ref attributes);

			if (window.WindowHandle == IntPtr.Zero)
				throw new ApplicationException("XCreateWindow call failed (returned 0).");

			if (title != null)
				API.XStoreName(window.Display, window.WindowHandle, title);

			// Set the window hints
			SetWindowMinMax(_min_width, _min_height, -1, -1);
			
			XSizeHints hints = new XSizeHints();
			hints.base_width = width;
			hints.base_height = height;
			hints.flags = (IntPtr)(XSizeHintsFlags.PSize | XSizeHintsFlags.PPosition);
			API.XSetWMNormalHints(window.Display, window.WindowHandle, ref hints);
			// Register for window destroy notification
			API.XSetWMProtocols(window.Display, window.WindowHandle, new IntPtr[] { wm_destroy }, 1);

			// Set the initial window size to ensure X, Y, Width, Height and the rest
			// return the correct values inside the constructor and the Load event.
			XEvent e = new XEvent();
			e.ConfigureEvent.x = x;
			e.ConfigureEvent.y = y;
			e.ConfigureEvent.width = width;
			e.ConfigureEvent.height = height;
			RefreshWindowBounds(ref e);

			Debug.Print("X11GLNative window created successfully (id: {0}).", Handle);
			SetupInput();
			exists = true;
		}
		
		void SetupInput() {
			Debug.Print("Initalizing X11 input driver.");
			
			// Init keyboard
			API.XDisplayKeycodes(window.Display, ref firstKeyCode, ref lastKeyCode);
			Debug.Print("First keycode: {0}, last {1}", firstKeyCode, lastKeyCode);
			
			IntPtr keysym_ptr = API.XGetKeyboardMapping(window.Display, (byte)firstKeyCode,
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
			API.XkbSetDetectableAutoRepeat(window.Display, true, out supported);
		}
		
		/// <summary> Registers the necessary atoms for GameWindow. </summary>
		void RegisterAtoms(X11WindowInfo window) {
			wm_destroy = API.XInternAtom(window.Display, "WM_DELETE_WINDOW", true);
			net_wm_state = API.XInternAtom(window.Display, "_NET_WM_STATE", false);
			net_wm_state_minimized = API.XInternAtom(window.Display, "_NET_WM_STATE_MINIMIZED", false);
			net_wm_state_fullscreen = API.XInternAtom(window.Display, "_NET_WM_STATE_FULLSCREEN", false);
			net_wm_state_maximized_horizontal = API.XInternAtom(window.Display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
			net_wm_state_maximized_vertical = API.XInternAtom(window.Display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
			net_wm_icon = API.XInternAtom(window.Display, "_NEW_WM_ICON", false);
			net_frame_extents = API.XInternAtom(window.Display, "_NET_FRAME_EXTENTS", false);
		}

		void SetWindowMinMax(short min_width, short min_height, short max_width, short max_height) {
			IntPtr dummy;
			XSizeHints hints = new XSizeHints();
			API.XGetWMNormalHints(window.Display, window.WindowHandle, ref hints, out dummy);

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
				API.XSetWMNormalHints(window.Display, window.WindowHandle, ref hints);
			}
		}
		
		void ToggleKey( ref XKeyEvent keyEvent, bool pressed ) {
			IntPtr keysym = API.XLookupKeysym(ref keyEvent, 0);
			IntPtr keysym2 = API.XLookupKeysym(ref keyEvent, 1);
			if (keymap.ContainsKey((XKey)keysym))
				keyboard[keymap[(XKey)keysym]] = pressed;
			else if (keymap.ContainsKey((XKey)keysym2))
				keyboard[keymap[(XKey)keysym2]] = pressed;
			else
				Debug.Print("KeyCode {0} (Keysym: {1}, {2}) not mapped.", e.KeyEvent.keycode, (XKey)keysym, (XKey)keysym2);
		}
		
		static void DeleteIconPixmaps(IntPtr display, IntPtr window) {
			IntPtr wmHints_ptr = API.XGetWMHints(display, window);
			
			if (wmHints_ptr != IntPtr.Zero) {
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
		}
		
		void RefreshWindowBorders() {
			IntPtr atom, nitems, bytes_after, prop = IntPtr.Zero;
			int format;
			API.XGetWindowProperty(window.Display, window.WindowHandle,
			                       net_frame_extents, IntPtr.Zero, new IntPtr(16), false,
			                       (IntPtr)Atom.XA_CARDINAL, out atom, out format, out nitems, out bytes_after, ref prop);

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
				if (Move != null)
					Move(this, EventArgs.Empty);
			}

			// Note: width and height denote the internal (client) size.
			// To get the external (window) size, we need to add the border size.
			Size newSize = new Size(
				e.ConfigureEvent.width + borderLeft + borderRight,
				e.ConfigureEvent.height + borderTop + borderBottom);
			if (bounds.Size != newSize) {
				bounds.Size = newSize;
				client_rectangle.Size = new Size(e.ConfigureEvent.width, e.ConfigureEvent.height);

				if (this.Resize != null) {
					Resize(this, EventArgs.Empty);
				}
			}
		}
		
		public void ProcessEvents() {
			// Process all pending events
			while (Exists && window != null) {
				if (!API.XCheckWindowEvent(window.Display, window.WindowHandle, window.EventMask, ref e) &&
				    !API.XCheckTypedWindowEvent(window.Display, window.WindowHandle, XEventName.ClientMessage, ref e))
					break;
				
				// Respond to the event e
				switch (e.type) {
					case XEventName.MapNotify:
					case XEventName.UnmapNotify:
						{
							bool previous_visible = visible;
							visible = e.type == XEventName.MapNotify;
							if (visible != previous_visible)
								if (VisibleChanged != null)
									VisibleChanged(this, EventArgs.Empty);
						} break;

					case XEventName.CreateNotify:
						// A child was was created - nothing to do
						break;

					case XEventName.ClientMessage:
						if (!isExiting && e.ClientMessageEvent.ptr1 == wm_destroy) {
							Debug.Print("Exit message received.");
							CancelEventArgs ce = new CancelEventArgs();
							if (Closing != null)
								Closing(this, ce);

							if (!ce.Cancel) {
								isExiting = true;

								DestroyWindow();
								if (Closed != null)
									Closed(this, EventArgs.Empty);
							}
						} break;

					case XEventName.DestroyNotify:
						Debug.Print("Window destroyed");
						exists = false;

						break;

					case XEventName.ConfigureNotify:
						RefreshWindowBounds(ref e);
						break;
						
					case XEventName.KeyPress:
						ToggleKey( ref e.KeyEvent, true );
						int status = API.XLookupString(ref e.KeyEvent, ascii, ascii.Length, null, IntPtr.Zero);
						Encoding.Default.GetChars(ascii, 0, status, chars, 0);

						EventHandler<KeyPressEventArgs> key_press = KeyPress;
						if (key_press != null) {
							for (int i = 0; i < status; i++) {
								KPEventArgs.KeyChar = chars[i];
								key_press(this, KPEventArgs);
							}
						}
						break;
						
					case XEventName.KeyRelease:
						// Todo: raise KeyPress event. Use code from
						// http://anonsvn.mono-project.com/viewvc/trunk/mcs/class/Managed.Windows.Forms/System.Windows.Forms/X11Keyboard.cs?view=markup
						ToggleKey( ref e.KeyEvent, false );
						break;
						
					case XEventName.ButtonPress:
						if      (e.ButtonEvent.button == 1) mouse[OpenTK.Input.MouseButton.Left] = true;
						else if (e.ButtonEvent.button == 2) mouse[OpenTK.Input.MouseButton.Middle] = true;
						else if (e.ButtonEvent.button == 3) mouse[OpenTK.Input.MouseButton.Right] = true;
						else if (e.ButtonEvent.button == 4) mouse.Wheel++;
						else if (e.ButtonEvent.button == 5) mouse.Wheel--;
						else if (e.ButtonEvent.button == 6) mouse[OpenTK.Input.MouseButton.Button1] = true;
						else if (e.ButtonEvent.button == 7) mouse[OpenTK.Input.MouseButton.Button2] = true;
						break;

					case XEventName.ButtonRelease:
						if      (e.ButtonEvent.button == 1) mouse[OpenTK.Input.MouseButton.Left] = false;
						else if (e.ButtonEvent.button == 2) mouse[OpenTK.Input.MouseButton.Middle] = false;
						else if (e.ButtonEvent.button == 3) mouse[OpenTK.Input.MouseButton.Right] = false;
						else if (e.ButtonEvent.button == 6) mouse[OpenTK.Input.MouseButton.Button1] = false;
						else if (e.ButtonEvent.button == 7) mouse[OpenTK.Input.MouseButton.Button2] = false;
						break;

					case XEventName.MotionNotify:
						mouse.Position = new Point(e.MotionEvent.x, e.MotionEvent.y);
						break;

					case XEventName.FocusIn:
					case XEventName.FocusOut:
						{
							bool previous_focus = has_focus;
							has_focus = e.type == XEventName.FocusIn;
							if (has_focus != previous_focus && FocusedChanged != null)
								FocusedChanged(this, EventArgs.Empty);
						} break;

					case XEventName.LeaveNotify:
						if (MouseLeave != null)
							MouseLeave(this, EventArgs.Empty);
						break;

					case XEventName.EnterNotify:
						if (MouseEnter != null)
							MouseEnter(this, EventArgs.Empty);
						break;

					case XEventName.MappingNotify:
						// 0 == MappingModifier, 1 == MappingKeyboard
						if (e.MappingEvent.request == 0 || e.MappingEvent.request == 1) {
							Debug.Print("keybard mapping refreshed");
							API.XRefreshKeyboardMapping(ref e.MappingEvent);
						}
						break;

					case XEventName.PropertyNotify:
						if (e.PropertyEvent.atom == net_wm_state) {
							if (WindowStateChanged != null)
								WindowStateChanged(this, EventArgs.Empty);
						}

						//if (e.PropertyEvent.atom == net_frame_extents) {
						//    RefreshWindowBorders();
						//}
						break;
						
					default:
						//Debug.WriteLine(String.Format("{0} event was not handled", e.type));
						break;
				}
			}
		}
		
		public string GetClipboardText() { return ""; }
		
		public void SetClipboardText( string value ) {	}	
		
		public Rectangle Bounds {
			get { return bounds; }
			set {
				API.XMoveResizeWindow(
					window.Display, window.WindowHandle, value.X, value.Y,
					value.Width - borderLeft - borderRight, value.Height - borderTop - borderBottom);
				ProcessEvents();
			}
		}
		
		public Point Location {
			get { return Bounds.Location; }
			set {
				API.XMoveWindow(window.Display, window.WindowHandle, value.X, value.Y);
				ProcessEvents();
			}
		}
		
		public Size Size {
			get { return Bounds.Size; }
			set {
				int width = value.Width - borderLeft - borderRight;
				int height = value.Height - borderTop - borderBottom;
				width = width <= 0 ? 1 : width;
				height = height <= 0 ? 1 : height;
				API.XResizeWindow(window.Display, window.WindowHandle, width, height);
				ProcessEvents();
			}
		}
		
		public Rectangle ClientRectangle {
			get {
				if (client_rectangle.Width == 0)
					client_rectangle.Width = 1;
				if (client_rectangle.Height == 0)
					client_rectangle.Height = 1;
				return client_rectangle;
			}
			set {
				API.XResizeWindow(window.Display, window.WindowHandle, value.Width, value.Height);
				ProcessEvents();
			}
		}
		
		public Size ClientSize {
			get { return ClientRectangle.Size; }
			set { ClientRectangle = new Rectangle(Point.Empty, value); }
		}

		public int Width {
			get { return ClientSize.Width; }
			set { ClientSize = new Size(value, Height); }
		}
		
		public int Height {
			get { return ClientSize.Height; }
			set { ClientSize = new Size(Width, value); }
		}
		
		public int X {
			get { return Location.X; }
			set { Location = new Point(value, Y); }
		}

		public int Y {
			get { return Location.Y; }
			set { Location = new Point(X, value); }
		}
		
		public Icon Icon {
			get { return icon; }
			set {
				if (value == icon)
					return;

				// Note: it seems that Gnome/Metacity does not respect the _NET_WM_ICON hint.
				// For this reason, we'll also set the icon using XSetWMHints.
				if (value == null) {
					API.XDeleteProperty(window.Display, window.WindowHandle, net_wm_icon);
					DeleteIconPixmaps(window.Display, window.WindowHandle);
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
					
					API.XChangeProperty(window.Display, window.WindowHandle,
					                    net_wm_icon, xa_cardinal, 32,
					                    PropertyMode.Replace, data, size);

					// Set XWMHints
					DeleteIconPixmaps(window.Display, window.WindowHandle);
					IntPtr wmHints_ptr = API.XGetWMHints(window.Display, window.WindowHandle);
					
					if (wmHints_ptr == IntPtr.Zero)
						wmHints_ptr = API.XAllocWMHints();
					
					XWMHints wmHints = (XWMHints)Marshal.PtrToStructure(wmHints_ptr, typeof(XWMHints));
					
					wmHints.flags = new IntPtr(wmHints.flags.ToInt32() | (int)(XWMHintsFlags.IconPixmapHint | XWMHintsFlags.IconMaskHint));
					wmHints.icon_pixmap = API.CreatePixmapFromImage(window.Display, bitmap);
					wmHints.icon_mask = API.CreateMaskFromImage(window.Display, bitmap);
					
					API.XSetWMHints(window.Display, window.WindowHandle, ref wmHints);
					API.XFree(wmHints_ptr);
					API.XSync(window.Display, false);
				}

				icon = value;
				if (IconChanged != null)
					IconChanged(this, EventArgs.Empty);
			}
		}
		
		public bool Focused {
			get { return has_focus; }
		}
		
		public WindowState WindowState {
			get {
				IntPtr actual_atom, nitems, bytes_after, prop = IntPtr.Zero;
				int actual_format;
				bool fullscreen = false, minimised = false;
				int maximised = 0;
				API.XGetWindowProperty(window.Display, window.WindowHandle,
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
                                Functions.XGetWindowAttributes(window.Display, window.WindowHandle, ref attributes);
                                if (attributes.map_state == MapState.IsUnmapped)
                                    return (OpenTK.WindowState)(-1);
				 */
				return WindowState.Normal;
			}
			set {
				WindowState current_state = this.WindowState;

				if (current_state == value)
					return;

				Debug.Print("GameWindow {0} changing WindowState from {1} to {2}.", window.WindowHandle.ToString(),
				            current_state.ToString(), value.ToString());

				// Reset the current window state
				if (current_state == OpenTK.WindowState.Minimized)
					API.XMapWindow(window.Display, window.WindowHandle);
				else if (current_state == OpenTK.WindowState.Fullscreen)
					API.SendNetWMMessage(window, net_wm_state, _remove,
					                     net_wm_state_fullscreen,
					                     IntPtr.Zero);
				else if (current_state == OpenTK.WindowState.Maximized)
					API.SendNetWMMessage(window, net_wm_state, _toggle,
					                     net_wm_state_maximized_horizontal,
					                     net_wm_state_maximized_vertical);
				
				API.XSync(window.Display, false);

				switch (value) {
					case OpenTK.WindowState.Normal:
						API.XRaiseWindow(window.Display, window.WindowHandle);
						break;
						
					case OpenTK.WindowState.Maximized:
						API.SendNetWMMessage(window, net_wm_state, _add,
						                     net_wm_state_maximized_horizontal,
						                     net_wm_state_maximized_vertical);
						API.XRaiseWindow(window.Display, window.WindowHandle);
						break;
						
					case OpenTK.WindowState.Minimized:
						// Todo: multiscreen support
						API.XIconifyWindow(window.Display, window.WindowHandle, window.Screen);
						break;
						
					case OpenTK.WindowState.Fullscreen:
						API.SendNetWMMessage(window, net_wm_state, _add,
						                     net_wm_state_fullscreen, IntPtr.Zero);
						API.XRaiseWindow(window.Display, window.WindowHandle);
						break;
				}
				ProcessEvents();
			}
		}

		public event EventHandler<EventArgs> Load;
		public event EventHandler<EventArgs> Unload;
		public event EventHandler<EventArgs> Move;
		public event EventHandler<EventArgs> Resize;
		public event EventHandler<System.ComponentModel.CancelEventArgs> Closing;
		public event EventHandler<EventArgs> Closed;
		public event EventHandler<EventArgs> Disposed;
		public event EventHandler<EventArgs> IconChanged;
		public event EventHandler<EventArgs> TitleChanged;
		public event EventHandler<EventArgs> VisibleChanged;
		public event EventHandler<EventArgs> FocusedChanged;
		public event EventHandler<EventArgs> WindowStateChanged;
		public event EventHandler<KeyPressEventArgs> KeyPress;
		public event EventHandler<EventArgs> MouseEnter;
		public event EventHandler<EventArgs> MouseLeave;
		
		public KeyboardDevice Keyboard {
			get { return keyboard; }
		}

		public MouseDevice Mouse {
			get { return mouse; }
		}
		
		public Point DesktopCursorPos {
			get {
				IntPtr root, child;
				int rootX, rootY, childX, childY, mask;
				API.XQueryPointer( window.Display, window.RootWindow, out root, out child, out rootX, out rootY, out childX, out childY, out mask );
				return new Point( rootX, rootY );
			}
			set {
				IntPtr root, child;
				int rootX, rootY, childX, childY, mask;
				API.XQueryPointer( window.Display, window.RootWindow, out root, out child, out rootX, out rootY, out childX, out childY, out mask );
				API.XWarpPointer( window.Display, IntPtr.Zero, IntPtr.Zero, 0, 0, 0, 0, value.X - rootX, value.Y - rootY );
				API.XFlush( window.Display ); // TODO: not sure if XFlush call is necessary
			}
		}
		
		bool cursorVisible = true;
		public bool CursorVisible {
			get { return cursorVisible; }
			set {
				cursorVisible = value;
				if( value ) {
					API.XUndefineCursor( window.Display, window.WindowHandle );
				} else {
					if( blankCursor == IntPtr.Zero )
						MakeBlankCursor();
					API.XDefineCursor( window.Display, window.WindowHandle, blankCursor );
				}
			}
		}
		
		IntPtr blankCursor;
		void MakeBlankCursor() {
			XColor color = default( XColor );
			IntPtr pixmap = API.XCreatePixmap( window.Display, window.RootWindow, 1, 1, 1 );
			blankCursor = API.XCreatePixmapCursor( window.Display, pixmap, pixmap, ref color, ref color, 0, 0 );		
			API.XFreePixmap( window.Display, pixmap );
		}
		
		/// <summary> Returns true if a render window/context exists. </summary>
		public bool Exists {
			get { return exists; }
		}
		
		/// <summary> Gets the current window handle. </summary>
		public IntPtr Handle {
			get { return window.WindowHandle; }
		}
		
		/// <summary> TODO: Use atoms for this property.
		/// Gets or sets the GameWindow title. </summary>
		public string Title {
			get {
				IntPtr name = IntPtr.Zero;
				API.XFetchName(window.Display, window.WindowHandle, ref name);
				return name == IntPtr.Zero ? String.Empty : Marshal.PtrToStringAnsi(name);
			}
			set {
				if (value != null && value != Title) {
					API.XStoreName(window.Display, window.WindowHandle, value);
				}

				if (TitleChanged != null)
					TitleChanged(this, EventArgs.Empty);
			}
		}
		
		public bool Visible {
			get { return visible; }
			set {
				if (value && !visible) {
					API.XMapWindow(window.Display, window.WindowHandle);
				} else if (!value && visible) {
					API.XUnmapWindow(window.Display, window.WindowHandle);
				}
			}
		}
		
		public IWindowInfo WindowInfo {
			get { return window; }
		}

		public void Close() {
			XEvent ev = new XEvent();
			ev.type = XEventName.ClientMessage;
			ev.ClientMessageEvent.format = 32;
			ev.ClientMessageEvent.display = window.Display;
			ev.ClientMessageEvent.window = window.WindowHandle;
			ev.ClientMessageEvent.ptr1 = wm_destroy;
			API.XSendEvent(window.Display, window.WindowHandle, false,
			               EventMask.NoEventMask, ref ev);
			API.XFlush(window.Display);
		}

		public void DestroyWindow() {
			Debug.Print("X11GLNative shutdown sequence initiated.");
			API.XSync(window.Display, true);
			API.XDestroyWindow(window.Display, window.WindowHandle);
			exists = false;
		}

		public Point PointToClient(Point point) {
			int ox, oy;
			IntPtr child;
			API.XTranslateCoordinates(window.Display, window.RootWindow, window.WindowHandle, point.X, point.Y, out ox, out oy, out child);
			return new Point( ox, oy );
		}

		public Point PointToScreen(Point point) {
			int ox, oy;
			IntPtr child;
			API.XTranslateCoordinates(window.Display, window.WindowHandle, window.RootWindow, point.X, point.Y, out ox, out oy, out child);
			return new Point( ox, oy );
		}

		public void Dispose() {
			this.Dispose(true);
			GC.SuppressFinalize(this);
		}

		private void Dispose(bool manuallyCalled) {
			if (disposed) return;
			if (manuallyCalled) {
				if (window != null && window.WindowHandle != IntPtr.Zero) {
					if (Exists) {
						DestroyWindow();
					}
					window.Dispose();
					window = null;
				}
			} else {
				Debug.Print("[Warning] {0} leaked.", this.GetType().Name);
			}
			disposed = true;
		}

		~X11GLNative() {
			this.Dispose(false);
		}
	}
}
