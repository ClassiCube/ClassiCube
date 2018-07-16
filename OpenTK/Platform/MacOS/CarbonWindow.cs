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
using OpenTK.Input;

namespace OpenTK.Platform.MacOS {
	internal class CarbonWindow : INativeWindow {
		static MacOSKeyMap Keymap = new MacOSKeyMap();
		IntPtr uppHandler;
		internal bool goFullScreenHack, goWindowedHack;

		string title = "OpenTK Window";
		Rectangle bounds;
		Size clientSize;
		Rectangle windowedBounds;
		bool mIsDisposed = false;
		internal DisplayDevice Display;

		WindowPositionMethod mPositionMethod = WindowPositionMethod.CenterOnMainScreen;
		int mTitlebarHeight;
		WindowState windowState = WindowState.Normal;
		Icon mIcon;
		readonly MacOSEventHandler handler;

		static CarbonWindow() {
			Application.Initialize();
		}
		
		public CarbonWindow(int x, int y, int width, int height, string title, DisplayDevice device) {
			this.title = title;
			handler = EventHandlerFunc;
			CreateNativeWindow(WindowClass.Document,
			                   WindowAttributes.StandardDocument | WindowAttributes.StandardHandler |
			                   WindowAttributes.InWindowMenu | WindowAttributes.LiveResize,
			                   new Rect((short)x, (short)y, (short)width, (short)height));
			Exists = true;
			Display = device;
		}

		protected override void Dispose(bool disposing) {
			if (mIsDisposed) return;

			Debug.Print("Disposing of CarbonGLNative window.");
			API.DisposeWindow(WinHandle);
			mIsDisposed = true;
			Exists = false;
			DisposeUPP();
		}

		void DisposeUPP() {
			if (uppHandler != IntPtr.Zero) {
				//API.RemoveEventHandler(uppHandler);
				//API.DisposeEventHandlerUPP(uppHandler);
			}
			uppHandler = IntPtr.Zero;
		}

		void CreateNativeWindow(WindowClass @class, WindowAttributes attrib, Rect r) {
			Debug.Print("Creating window...");

			OSStatus err = API.CreateNewWindow(@class, attrib, ref r, out WinHandle);
			API.CheckReturn(err);
			Debug.Print("Created window " + WinHandle.ToString());
			
			IntPtr titleCF = CF.CFSTR(title);
			Debug.Print("Setting window title: {0},   CFstring : {1},  Text : {2}", WinHandle, titleCF, title);
			API.SetWindowTitleWithCFString(WinHandle, titleCF);

			SetLocation(r.X, r.Y);
			SetSize(r.Width, r.Height);
			LoadSize();

			Rect titleSize = API.GetWindowBounds(WinHandle, WindowRegionCode.TitleBarRegion);
			mTitlebarHeight = titleSize.Height;
			Debug.Print("Titlebar size: {0}", titleSize);
			ConnectEvents();
			Debug.Print("Attached window events.");
		}

		void ConnectEvents() {
			EventTypeSpec[] eventTypes = new EventTypeSpec[] {
				new EventTypeSpec(EventClass.Window, WindowEventKind.WindowClose),
				new EventTypeSpec(EventClass.Window, WindowEventKind.WindowClosed),
				new EventTypeSpec(EventClass.Window, WindowEventKind.WindowBoundsChanged),
				new EventTypeSpec(EventClass.Window, WindowEventKind.WindowActivate),
				new EventTypeSpec(EventClass.Window, WindowEventKind.WindowDeactivate),

				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseDown),
				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseUp),
				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseMoved),
				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseDragged),
				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseEntered),
				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.MouseExited),
				//new EventTypeSpec(EventClass.Mouse, MouseEventKind.WheelMoved),

				//new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyDown),
				//new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyRepeat),
				//new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyUp),
				//new EventTypeSpec(EventClass.Keyboard, KeyboardEventKind.RawKeyModifiersChanged),
			};

			uppHandler = API.NewEventHandlerUPP(handler);
			API.InstallWindowEventHandler(WinHandle, uppHandler, eventTypes, WinHandle, IntPtr.Zero);
			Application.WindowEventHandler = this;
		}

		#if !USE_DX
		internal void SetFullscreen(AglContext context) {
			windowedBounds = bounds;
			int width, height;
			context.SetFullScreen(this, out width, out height);

			Debug.Print("Prev Size: {0}, {1}", clientSize.Width, clientSize.Height);
			clientSize = new Size(width, height);
			Debug.Print("New Size: {0}, {1}", clientSize.Width, clientSize.Height);

			// TODO: if we go full screen we need to make this use the device specified.
			bounds = Display.Bounds;
			windowState = WindowState.Fullscreen;
		}

		internal void UnsetFullscreen(AglContext context) {
			context.UnsetFullScreen(this);
			Debug.Print("Telling Carbon to reset window state to " + windowState.ToString());
			
			SetCarbonWindowState();
			SetSize((short)windowedBounds.Width, (short)windowedBounds.Height);
		}
		#endif

		internal OSStatus DispatchEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			switch (evt.EventClass) {
				case EventClass.Window:
					return ProcessWindowEvent(inCaller, inEvent, evt, userData);
				case EventClass.Mouse:
					return ProcessMouseEvent(inCaller, inEvent, evt, userData);
				case EventClass.Keyboard:
					return ProcessKeyboardEvent(inCaller, inEvent, evt, userData);
				default:
					return OSStatus.EventNotHandled;
			}
		}

		OSStatus EventHandlerFunc(IntPtr inCaller, IntPtr inEvent, IntPtr userData) {
			EventInfo evt = new EventInfo(inEvent);
			return DispatchEvent(inCaller, inEvent, evt, userData);
		}

		OSStatus ProcessKeyboardEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			MacOSKeyCode code = (MacOSKeyCode)0;
			char charCode = '\0';
			//Debug.Print("Processing Keyboard event {0}", (KeyboardEventKind)evt.EventKind);

			switch ((KeyboardEventKind)evt.EventKind) {
				case KeyboardEventKind.RawKeyDown:
				case KeyboardEventKind.RawKeyRepeat:
				case KeyboardEventKind.RawKeyUp:
					code = API.GetEventKeyboardKeyCode(inEvent);
					charCode = API.GetEventKeyboardChar(inEvent);
					break;
			}
			
			Key tkKey;
			if (!Keymap.TryGetValue(code, out tkKey)) {
				Debug.Print("{0} not mapped, ignoring press.", code);
				return OSStatus.NoError;
			}

			switch ((KeyboardEventKind)evt.EventKind) {
				case KeyboardEventKind.RawKeyRepeat:
					Keyboard.KeyRepeat = true;
					goto case KeyboardEventKind.RawKeyDown;

				case KeyboardEventKind.RawKeyDown:
					Keyboard.Set(tkKey, true);
					RaiseKeyPress(charCode);
					return OSStatus.NoError;

				case KeyboardEventKind.RawKeyUp:
					Keyboard.Set(tkKey, false);
					return OSStatus.NoError;

				case KeyboardEventKind.RawKeyModifiersChanged:
					ProcessModifierKey(inEvent);
					return OSStatus.NoError;
			}
			return OSStatus.EventNotHandled;
		}

		OSStatus ProcessWindowEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			switch ((WindowEventKind)evt.EventKind) {
				case WindowEventKind.WindowClose:
					RaiseClosing();
					return OSStatus.EventNotHandled;

				case WindowEventKind.WindowClosed:
					Exists = false;
					RaiseClosed();
					return OSStatus.NoError;

				case WindowEventKind.WindowBoundsChanged:
					int curWidth = clientSize.Width;
					int curHeight = clientSize.Height;
					LoadSize();

					if (curWidth != clientSize.Width || curHeight != clientSize.Height)
						OnResize();
					return OSStatus.EventNotHandled;

				case WindowEventKind.WindowActivate:
					Focused = true;
					RaiseFocusedChanged();
					return OSStatus.EventNotHandled;

				case WindowEventKind.WindowDeactivate:
					Focused = false;
					RaiseFocusedChanged();
					return OSStatus.EventNotHandled;
			}
			return OSStatus.EventNotHandled;
		}
		
		OSStatus ProcessMouseEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			MacOSMouseButton button;
			HIPoint pt = new HIPoint();
			HIPoint screenLoc =  new HIPoint();

			OSStatus err = API.GetEventMouseLocation(inEvent, out screenLoc);
			if (windowState == WindowState.Fullscreen) {
				pt = screenLoc;
			} else {
				err = API.GetEventWindowMouseLocation(inEvent, out pt);
			}

			if (err != OSStatus.NoError) {
				// this error comes up from the application event handler.
				if (err != OSStatus.EventParameterNotFound) {
					throw new MacOSException(err);
				}
			}

			Point mousePosInClient = new Point((int)pt.X, (int)pt.Y);
			if (windowState != WindowState.Fullscreen) {
				mousePosInClient.Y -= mTitlebarHeight;
			}
			
			switch ((MouseEventKind)evt.EventKind) {
				case MouseEventKind.MouseDown:
					button = API.GetEventMouseButton(inEvent);

					switch (button) {
						case MacOSMouseButton.Primary:
							Mouse.Set(MouseButton.Left, true); break;
						case MacOSMouseButton.Secondary:
							Mouse.Set(MouseButton.Right, true); break;
						case MacOSMouseButton.Tertiary:
							Mouse.Set(MouseButton.Middle, true); break;
					}
					return OSStatus.NoError;

				case MouseEventKind.MouseUp:
					button = API.GetEventMouseButton(inEvent);

					switch (button) {
						case MacOSMouseButton.Primary:
							Mouse.Set(MouseButton.Left, false); break;
						case MacOSMouseButton.Secondary:
							Mouse.Set(MouseButton.Right, false); break;
						case MacOSMouseButton.Tertiary:
							Mouse.Set(MouseButton.Middle, false); break;
					}
					return OSStatus.NoError;

				case MouseEventKind.WheelMoved:
					Mouse.SetWheel(Mouse.Wheel + API.GetEventMouseWheelDelta(inEvent));
					return OSStatus.NoError;

				case MouseEventKind.MouseMoved:
				case MouseEventKind.MouseDragged:
					
					//Debug.Print("Mouse Location: {0}, {1}", pt.X, pt.Y);

					if (windowState == WindowState.Fullscreen) {
						if (mousePosInClient.X != Mouse.X || mousePosInClient.Y != Mouse.Y) {
							Mouse.SetPos(mousePosInClient.X, mousePosInClient.Y);
						}
					} else {
						// ignore clicks in the title bar
						if (pt.Y < 0)
							return OSStatus.EventNotHandled;

						if (mousePosInClient.X != Mouse.X || mousePosInClient.Y != Mouse.Y) {
							Mouse.SetPos(mousePosInClient.X, mousePosInClient.Y);
						}
					}
					return OSStatus.EventNotHandled;
			}
			return OSStatus.EventNotHandled;
		}
		
		void ProcessModifierKey(IntPtr inEvent) {
			MacOSKeyModifiers modifiers = API.GetEventKeyModifiers(inEvent);

			bool caps = (modifiers & MacOSKeyModifiers.CapsLock) != 0;
			bool control = (modifiers & MacOSKeyModifiers.Control) != 0;
			bool command = (modifiers & MacOSKeyModifiers.Command) != 0;
			bool option = (modifiers & MacOSKeyModifiers.Option) != 0;
			bool shift = (modifiers & MacOSKeyModifiers.Shift) != 0;

			Debug.Print("Modifiers Changed: {0}", modifiers);
			// TODO: Is this even needed
			bool repeat = Keyboard.KeyRepeat; Keyboard.KeyRepeat = false;

			Keyboard.Set(Key.AltLeft, option);
			Keyboard.Set(Key.ShiftLeft, shift);
			Keyboard.Set(Key.WinLeft, command);
			Keyboard.Set(Key.ControlLeft, control);
			Keyboard.Set(Key.CapsLock, caps);
			
			Keyboard.KeyRepeat = repeat;
		}

		void SetLocation(short x, short y) {
			if (windowState == WindowState.Fullscreen) return;
			API.MoveWindow(WinHandle, x, y, false);
		}

		void SetSize(short width, short height) {
			if (WindowState == WindowState.Fullscreen) return;

			// The bounds of the window should be the size specified, but
			// API.SizeWindow sets the content region size.  So
			// we reduce the size to get the correct bounds.
			width -= (short)(bounds.Width - clientSize.Width);
			height -= (short)(bounds.Height - clientSize.Height);
			
			API.SizeWindow(WinHandle, width, height, true);
		}

		void SetClientSize(short width, short height) {
			if (WindowState == WindowState.Fullscreen) return;			
			API.SizeWindow(WinHandle, width, height, true);
		}
		
		void OnResize() {
			LoadSize();
			RaiseResize();
		}

		void LoadSize() {
			if (WindowState == WindowState.Fullscreen) return;

			Rect r = API.GetWindowBounds(WinHandle, WindowRegionCode.StructureRegion);
			bounds = new Rectangle(r.X, r.Y, r.Width, r.Height);

			r = API.GetWindowBounds(WinHandle, WindowRegionCode.GlobalPortRegion);
			clientSize = new Size(r.Width, r.Height);
		}

		IntPtr pbStr, utf16, utf8;
		public override string GetClipboardText() {
			IntPtr pbRef = GetPasteboard();
			API.PasteboardSynchronize(pbRef);

			uint itemCount;
			OSStatus err = API.PasteboardGetItemCount(pbRef, out itemCount);
			if (err != OSStatus.NoError)
				throw new MacOSException(err, "Getting item count from Pasteboard.");
			if (itemCount < 1) return "";

			uint itemID;
			err = API.PasteboardGetItemIdentifier(pbRef, 1, out itemID);
			if (err != OSStatus.NoError)
				throw new MacOSException(err, "Getting item identifier from Pasteboard.");
			
			IntPtr outData;
			if ((err = API.PasteboardCopyItemFlavorData(pbRef, itemID, utf16, out outData)) == OSStatus.NoError) {
				IntPtr ptr = API.CFDataGetBytePtr(outData);
				if (ptr == IntPtr.Zero)
					throw new InvalidOperationException("CFDataGetBytePtr() returned null pointer.");
				return Marshal.PtrToStringUni(ptr);
			} else if ((err = API.PasteboardCopyItemFlavorData(pbRef, itemID, utf8, out outData)) == OSStatus.NoError) {
				IntPtr ptr = API.CFDataGetBytePtr(outData);
				if (ptr == IntPtr.Zero)
					throw new InvalidOperationException("CFDataGetBytePtr() returned null pointer.");
				return GetUTF8(ptr);
			}
			return "";
		}
		
		unsafe static string GetUTF8(IntPtr ptr) {
			byte* countPtr = (byte*)ptr, readPtr = (byte*)ptr;
			int length = 0;
			while (*countPtr != 0) { length++; countPtr++; }
			
			byte[] text = new byte[length];
			for(int i = 0; i < text.Length; i++) {
				text[i] = *readPtr; readPtr++;
			}
			return Encoding.UTF8.GetString(text);
		}
		
		public override void SetClipboardText(string value) {
			IntPtr pbRef = GetPasteboard();
			OSStatus err = API.PasteboardClear(pbRef);
			if (err != OSStatus.NoError)
				throw new MacOSException(err, "Cleaing Pasteboard.");
			API.PasteboardSynchronize(pbRef);

			IntPtr ptr = Marshal.StringToHGlobalUni(value);
			IntPtr cfData = API.CFDataCreate(IntPtr.Zero, ptr, (value.Length + 1) * 2);
			if (cfData == IntPtr.Zero)
				throw new InvalidOperationException("CFDataCreate() returned null pointer.");
			
			API.PasteboardPutItemFlavor(pbRef, 1, utf16, cfData, 0);
			Marshal.FreeHGlobal(ptr);
		}
		
		IntPtr GetPasteboard() {
			if (pbStr == IntPtr.Zero) {
				pbStr = CF.CFSTR("com.apple.pasteboard.clipboard");
				utf16 = CF.CFSTR("public.utf16-plain-text");
				utf8 = CF.CFSTR("public.utf8-plain-text");
			}
			
			IntPtr pbRef;
			OSStatus err = API.PasteboardCreate(pbStr, out pbRef);
			if (err != OSStatus.NoError)
				throw new MacOSException(err, "Creating Pasteboard reference.");
			API.PasteboardSynchronize(pbRef);
			return pbRef;
		}

		// Processes events in the queue and then returns.
		public override void ProcessEvents() {
			IntPtr theEvent;
			IntPtr target = API.GetEventDispatcherTarget();
			
			for (;;) {
				OSStatus status = API.ReceiveNextEvent(0, IntPtr.Zero, 0.0, true, out theEvent);
				if (status == OSStatus.EventLoopTimedOut) break;

				if (status != OSStatus.NoError) {
					Debug.Print("Message Loop status: {0}", status); break;
				}
				if (theEvent == IntPtr.Zero) break;

				API.SendEventToEventTarget(theEvent, target);
				API.ReleaseEvent(theEvent);
			}
		}

		public override Point PointToClient(Point point) {
			Rect r = API.GetWindowBounds(WinHandle, WindowRegionCode.ContentRegion);
			return new Point(point.X - r.X, point.Y - r.Y);
		}
		
		public override Point PointToScreen(Point point) {
			Rect r = API.GetWindowBounds(WinHandle, WindowRegionCode.ContentRegion);
			return new Point(point.X + r.X, point.Y + r.Y);
		}

		public override Icon Icon {
			get { return mIcon; }
			set { SetIcon(value); }
		}

		unsafe void SetIcon(Icon icon) {
			// The code for this function was adapted from Mono's
			// XplatUICarbon implementation, written by Geoff Norton
			// http://anonsvn.mono-project.com/viewvc/trunk/mcs/class/Managed.Windows.Forms/System.Windows.Forms/XplatUICarbon.cs?view=markup&pathrev=136932
			if (icon == null)
			{
				API.RestoreApplicationDockTileImage();
			}
			else
			{
				Bitmap bitmap;
				int size;
				IntPtr[] data;
				int index;

				bitmap = new Bitmap(128, 128);
				using (System.Drawing.Graphics g = System.Drawing.Graphics.FromImage(bitmap))
				{
					g.DrawImage(icon.ToBitmap(), 0, 0, 128, 128);
				}
				index = 0;
				size = bitmap.Width * bitmap.Height;
				data = new IntPtr[size];

				for (int y = 0; y < bitmap.Height; y++)
				{
					for (int x = 0; x < bitmap.Width; x++)
					{
						int pixel = bitmap.GetPixel(x, y).ToArgb();
						if (BitConverter.IsLittleEndian)
						{
							byte a = (byte)((pixel >> 24) & 0xFF);
							byte r = (byte)((pixel >> 16) & 0xFF);
							byte g = (byte)((pixel >> 8) & 0xFF);
							byte b = (byte)(pixel & 0xFF);
							data[index++] = (IntPtr)(a | (r << 8) | (g << 16) | (b << 24));
						}
						else
						{
							data[index++] = (IntPtr)pixel;
						}
					}
				}

				fixed (IntPtr* ptr = data) {
					IntPtr provider = API.CGDataProviderCreateWithData(IntPtr.Zero, (IntPtr)(void*)ptr, size * 4, IntPtr.Zero);
					IntPtr colorSpace = API.CGColorSpaceCreateDeviceRGB();
					IntPtr image = API.CGImageCreate(128, 128, 8, 32, 4 * 128,
					                                 colorSpace, 4, provider, IntPtr.Zero, 0, 0);
					API.SetApplicationDockTileImage(image);
				}
			}
		}

		public override bool Visible {
			get { return API.IsWindowVisible(WinHandle); }
			set {
				if (value && !Visible) {
					API.ShowWindow(WinHandle);
					API.RepositionWindow(WinHandle, IntPtr.Zero, mPositionMethod);
					API.SelectWindow(WinHandle);
				} else {
					API.HideWindow(WinHandle);
				}
			}
		}

		public override Rectangle Bounds {
			get { return bounds; }
			set {
				Location = value.Location;
				Size = value.Size;
			}
		}

		public override Point Location {
			get { return Bounds.Location; }
			set { SetLocation((short)value.X, (short)value.Y); }
		}

		public override Size Size {
			get { return bounds.Size; }
			set { SetSize((short)value.Width, (short)value.Height); }
		}

		public override Size ClientSize {
			get { return clientSize; }
			set {
				API.SizeWindow(WinHandle, (short)value.Width, (short)value.Height, true);
				OnResize();
			}
		}

		public override void Close() {
			RaiseClosed();
			Dispose();
		}

		public override WindowState WindowState {
			get {
				if (windowState == WindowState.Fullscreen)
					return WindowState.Fullscreen;
				if (API.IsWindowCollapsed(WinHandle))
					return WindowState.Minimized;
				if (API.IsWindowInStandardState(WinHandle))
					return WindowState.Maximized;
				return WindowState.Normal;
			}
			
			set {
				if (value == WindowState) return;
				Debug.Print("Switching window state from {0} to {1}", WindowState, value);
				WindowState oldState = WindowState;
				windowState = value;

				if (oldState == WindowState.Fullscreen) {
					goWindowedHack = true;
					// when returning from full screen, wait until the context is updated
					// to actually do the work.
					return;
				}

				if (oldState == WindowState.Minimized) {
					OSStatus err = API.CollapseWindow(WinHandle, false);
					API.CheckReturn(err);
				}
				SetCarbonWindowState();
			}
		}

		void SetCarbonWindowState() {
			CarbonPoint idealSize;
			OSStatus result;
			
			switch (windowState) {
				case WindowState.Fullscreen:
					goFullScreenHack = true;
					break;

				case WindowState.Maximized:
					// hack because mac os has no concept of maximized. Instead windows are "zoomed"
					// meaning they are maximized up to their reported ideal size.  So we report a
					// large ideal size.
					idealSize = new CarbonPoint(9000, 9000);
					result = API.ZoomWindowIdeal(WinHandle, (short)WindowPartCode.inZoomOut, ref idealSize);
					API.CheckReturn(result);
					break;

				case WindowState.Normal:
					if (WindowState == WindowState.Maximized) {
						idealSize = new CarbonPoint();
						result = API.ZoomWindowIdeal(WinHandle, (short)WindowPartCode.inZoomIn, ref idealSize);
						API.CheckReturn(result);
					}
					break;

				case WindowState.Minimized:
					result = API.CollapseWindow(WinHandle, true);
					API.CheckReturn(result);
					break;
			}

			RaiseWindowStateChanged();
			OnResize();
		}
		
		public override Point DesktopCursorPos {
			get {
				HIPoint point = default(HIPoint);
				// NOTE: HIGetMousePosition is only available on OSX 10.5 or later
				API.HIGetMousePosition(HICoordinateSpace.ScreenPixel, IntPtr.Zero, ref point);
				return new Point((int)point.X, (int)point.Y);
			}
			set {
				HIPoint point = default(HIPoint);
				point.X = value.X; point.Y = value.Y;
				CG.CGAssociateMouseAndMouseCursorPosition(0);
				CG.CGDisplayMoveCursorToPoint(CG.CGMainDisplayID(), point);
				CG.CGAssociateMouseAndMouseCursorPosition(1);
			}
		}
		
		bool cursorVisible = true;
		public override bool CursorVisible {
			get { return cursorVisible; }
			set {
				cursorVisible = value;
				if (cursorVisible)
					CG.CGDisplayShowCursor(CG.CGMainDisplayID());
				else
					CG.CGDisplayHideCursor(CG.CGMainDisplayID());
			}
		}
	}
}