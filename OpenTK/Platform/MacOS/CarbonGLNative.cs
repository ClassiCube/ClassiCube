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
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Runtime.InteropServices;
using OpenTK.Graphics;
using OpenTK.Platform.MacOS.Carbon;
using OpenTK.Input;

namespace OpenTK.Platform.MacOS
{
	class CarbonGLNative : INativeWindow
	{
		CarbonWindowInfo window;
		static MacOSKeyMap Keymap = new MacOSKeyMap();
		IntPtr uppHandler;

		string title = "OpenTK Window";
		Rectangle bounds, clientRectangle;
		Rectangle windowedBounds;
		bool mIsDisposed = false;
		bool mExists = true;
		DisplayDevice mDisplayDevice;

		WindowPositionMethod mPositionMethod = WindowPositionMethod.CenterOnMainScreen;
		int mTitlebarHeight;
		private WindowState windowState = WindowState.Normal;
		static Dictionary<IntPtr, WeakReference> mWindows = new Dictionary<IntPtr, WeakReference>();
		KeyPressEventArgs mKeyPressArgs = new KeyPressEventArgs();
		bool mMouseIn = false;
		bool mIsActive = false;
		Icon mIcon;

		static internal Dictionary<IntPtr, WeakReference> WindowRefMap { get { return mWindows; } }
		internal DisplayDevice TargetDisplayDevice { get { return mDisplayDevice; } }

		static CarbonGLNative() {
			Application.Initialize();
		}
		
		public CarbonGLNative(int x, int y, int width, int height, string title, GameWindowFlags options, DisplayDevice device)
		{
			this.title = title;
			CreateNativeWindow(WindowClass.Document,
			                   WindowAttributes.StandardDocument | WindowAttributes.StandardHandler |
			                   WindowAttributes.InWindowMenu | WindowAttributes.LiveResize,
			                   new Rect((short)x, (short)y, (short)width, (short)height));
			
			mDisplayDevice = device;
		}

		#region IDisposable

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing) {
			if (mIsDisposed)
				return;

			Debug.Print("Disposing of CarbonGLNative window.");
			API.DisposeWindow(window.WindowRef);
			mIsDisposed = true;
			mExists = false;

			if (disposing) {
				mWindows.Remove(window.WindowRef);
				window = null;
			}
			DisposeUPP();
		}

		~CarbonGLNative() {
			Dispose(false);
		}

		#endregion

		#region Private Members

		void DisposeUPP()
		{
			if (uppHandler != IntPtr.Zero)
			{
				//API.RemoveEventHandler(uppHandler);
				//API.DisposeEventHandlerUPP(uppHandler);
			}

			uppHandler = IntPtr.Zero;
		}

		void CreateNativeWindow(WindowClass @class, WindowAttributes attrib, Rect r) {
			Debug.Print("Creating window...");

			IntPtr windowRef;
			OSStatus err = API.CreateNewWindow(@class, attrib, ref r, out windowRef);
			API.CheckReturn( err );
			Debug.Print( "Created window " + windowRef );
			API.SetWindowTitle(windowRef, title);

			window = new CarbonWindowInfo(windowRef);
			SetLocation(r.X, r.Y);
			SetSize(r.Width, r.Height);
			mWindows.Add(windowRef, new WeakReference(this));
			LoadSize();

			Rect titleSize = API.GetWindowBounds(window.WindowRef, WindowRegionCode.TitleBarRegion);
			mTitlebarHeight = titleSize.Height;
			Debug.Print("Titlebar size: {0}", titleSize);
			ConnectEvents();
			Debug.Print("Attached window events.");
		}

		void ConnectEvents()
		{
			EventTypeSpec[] eventTypes = new EventTypeSpec[]
			{
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

			MacOSEventHandler handler = EventHandler;
			uppHandler = API.NewEventHandlerUPP(handler);
			API.InstallWindowEventHandler(window.WindowRef, uppHandler, eventTypes, window.WindowRef, IntPtr.Zero);
			Application.WindowEventHandler = this;
		}

		void Activate() {
			API.SelectWindow(window.WindowRef);
		}

		void Show() {
			API.ShowWindow(window.WindowRef);
			API.RepositionWindow(window.WindowRef, IntPtr.Zero, mPositionMethod);
			API.SelectWindow(window.WindowRef);
		}

		void Hide() {
			API.HideWindow(window.WindowRef);
		}

		internal void SetFullscreen(AglContext context) {
			windowedBounds = bounds;
			int width, height;
			context.SetFullScreen(window, out width, out height);

			Debug.Print("Prev Size: {0}, {1}", Width, Height);
			clientRectangle.Size = new Size(width, height);
			Debug.Print("New Size: {0}, {1}", Width, Height);

			// TODO: if we go full screen we need to make this use the device specified.
			bounds = mDisplayDevice.Bounds;
			windowState = WindowState.Fullscreen;
		}

		internal void UnsetFullscreen(AglContext context) {
			context.UnsetFullScreen(window);

			Debug.Print("Telling Carbon to reset window state to " + windowState.ToString());
			SetCarbonWindowState();

			SetSize((short)windowedBounds.Width, (short)windowedBounds.Height);
		}

		internal OSStatus DispatchEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData)
		{
			switch (evt.EventClass)
			{
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

		protected static OSStatus EventHandler(IntPtr inCaller, IntPtr inEvent, IntPtr userData)
		{
			// bail out if the window passed in is not actually our window.
			// I think this happens if using winforms with a GameWindow sometimes.
			if (!mWindows.ContainsKey(userData))
				return OSStatus.EventNotHandled;

			WeakReference reference = mWindows[userData];

			// bail out if the CarbonGLNative window has been garbage collected.
			if (!reference.IsAlive) {
				mWindows.Remove(userData);
				return OSStatus.EventNotHandled;
			}

			CarbonGLNative window = (CarbonGLNative)reference.Target;
			//Debug.Print("Processing {0} event for {1}.", evt, window.window);
			if (window == null) {
				Debug.Print("Window for event not found.");
				return OSStatus.EventNotHandled;
			}
			EventInfo evt = new EventInfo(inEvent);
			return window.DispatchEvent(inCaller, inEvent, evt, userData);
		}

		private OSStatus ProcessKeyboardEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			MacOSKeyCode code = (MacOSKeyCode)0;
			char charCode = '\0';

			//Debug.Print("Processing Keyboard event {0}", (KeyboardEventKind)evt.EventKind);

			switch ((KeyboardEventKind)evt.EventKind)
			{
				case KeyboardEventKind.RawKeyDown:
				case KeyboardEventKind.RawKeyRepeat:
				case KeyboardEventKind.RawKeyUp:
					GetCharCodes(inEvent, out code, out charCode);
					mKeyPressArgs.KeyChar = charCode;
					break;
			}
			
			if( !Keymap.ContainsKey( code ) ) {
				Debug.Print( "{0} not mapped, ignoring press.", code );
				return OSStatus.NoError;
			}

			switch ((KeyboardEventKind)evt.EventKind)
			{
				case KeyboardEventKind.RawKeyRepeat:
					keyboard.KeyRepeat = true;
					goto case KeyboardEventKind.RawKeyDown;

				case KeyboardEventKind.RawKeyDown:				
					keyboard[Keymap[code]] = true;
					OnKeyPress(mKeyPressArgs);
					return OSStatus.NoError;

				case KeyboardEventKind.RawKeyUp:
					keyboard[Keymap[code]] = false;
					return OSStatus.NoError;

				case KeyboardEventKind.RawKeyModifiersChanged:
					ProcessModifierKey(inEvent);
					return OSStatus.NoError;

				default:
					return OSStatus.EventNotHandled;
			}
		}

		private OSStatus ProcessWindowEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			switch ((WindowEventKind)evt.EventKind)
			{
				case WindowEventKind.WindowClose:
					CancelEventArgs cancel = new CancelEventArgs();
					OnClosing(cancel);

					if (cancel.Cancel)
						return OSStatus.NoError;
					else
						return OSStatus.EventNotHandled;

				case WindowEventKind.WindowClosed:
					mExists = false;
					OnClosed();

					return OSStatus.NoError;

				case WindowEventKind.WindowBoundsChanged:
					int thisWidth = Width;
					int thisHeight = Height;

					LoadSize();

					if (thisWidth != Width || thisHeight != Height)
						OnResize();

					return OSStatus.EventNotHandled;

				case WindowEventKind.WindowActivate:
					OnActivate();
					return OSStatus.EventNotHandled;

				case WindowEventKind.WindowDeactivate:
					OnDeactivate();
					return OSStatus.EventNotHandled;

				default:
					Debug.Print("{0}", evt);

					return OSStatus.EventNotHandled;
			}
		}
		OSStatus ProcessMouseEvent(IntPtr inCaller, IntPtr inEvent, EventInfo evt, IntPtr userData) {
			MacOSMouseButton button;
			HIPoint pt = new HIPoint();
			HIPoint screenLoc =  new HIPoint();

			OSStatus err = API.GetEventMouseLocation(inEvent, out screenLoc);

			if (this.windowState == WindowState.Fullscreen)
			{
				pt = screenLoc;
			}
			else
			{
				err = API.GetEventWindowMouseLocation(inEvent, out pt);
			}

			if (err != OSStatus.NoError)
			{
				// this error comes up from the application event handler.
				if (err != OSStatus.EventParameterNotFound)
				{
					throw new MacOSException(err);
				}
			}

			Point mousePosInClient = new Point((int)pt.X, (int)pt.Y);
			if (this.windowState != WindowState.Fullscreen)
			{
				mousePosInClient.Y -= mTitlebarHeight;
			}

			// check for enter/leave events
			IntPtr thisEventWindow;
			API.GetEventWindowRef(inEvent, out thisEventWindow);
			CheckEnterLeaveEvents(thisEventWindow, mousePosInClient);
			
			switch ((MouseEventKind)evt.EventKind)
			{
				case MouseEventKind.MouseDown:
					button = API.GetEventMouseButton(inEvent);

					switch (button)
					{
						case MacOSMouseButton.Primary:
							mouse[MouseButton.Left] = true;
							break;

						case MacOSMouseButton.Secondary:
							mouse[MouseButton.Right] = true;
							break;

						case MacOSMouseButton.Tertiary:
							mouse[MouseButton.Middle] = true;
							break;
					}
					return OSStatus.NoError;

				case MouseEventKind.MouseUp:
					button = API.GetEventMouseButton(inEvent);

					switch (button)
					{
						case MacOSMouseButton.Primary:
							mouse[MouseButton.Left] = false;
							break;

						case MacOSMouseButton.Secondary:
							mouse[MouseButton.Right] = false;
							break;

						case MacOSMouseButton.Tertiary:
							mouse[MouseButton.Middle] = false;
							break;
					}
					button = API.GetEventMouseButton(inEvent);
					return OSStatus.NoError;

				case MouseEventKind.WheelMoved:
					mouse.WheelPrecise += API.GetEventMouseWheelDelta(inEvent);
					return OSStatus.NoError;

				case MouseEventKind.MouseMoved:
				case MouseEventKind.MouseDragged:
					
					//Debug.Print("Mouse Location: {0}, {1}", pt.X, pt.Y);

					if (windowState == WindowState.Fullscreen) {
						if (mousePosInClient.X != mouse.X || mousePosInClient.Y != mouse.Y) {
							mouse.Position = mousePosInClient;
						}
					} else {
						// ignore clicks in the title bar
						if (pt.Y < 0)
							return OSStatus.EventNotHandled;

						if (mousePosInClient.X != mouse.X || mousePosInClient.Y != mouse.Y) {
							mouse.Position = mousePosInClient;
						}
					}
					return OSStatus.EventNotHandled;

				default:
					Debug.Print("{0}", evt);
					return OSStatus.EventNotHandled;
			}
		}

		private void CheckEnterLeaveEvents(IntPtr eventWindowRef, Point pt)
		{
			if (window == null)
				return;

			bool thisIn = eventWindowRef == window.WindowRef;

			if (pt.Y < 0)
				thisIn = false;

			if (thisIn != mMouseIn)
			{
				mMouseIn = thisIn;

				if (mMouseIn)
					OnMouseEnter();
				else
					OnMouseLeave();
			}
		}

		private static void GetCharCodes(IntPtr inEvent, out MacOSKeyCode code, out char charCode)
		{
			code = API.GetEventKeyboardKeyCode(inEvent);
			charCode = API.GetEventKeyboardChar(inEvent);
		}
		
		private void ProcessModifierKey(IntPtr inEvent)
		{
			MacOSKeyModifiers modifiers = API.GetEventKeyModifiers(inEvent);

			bool caps = (modifiers & MacOSKeyModifiers.CapsLock) != 0;
			bool control = (modifiers & MacOSKeyModifiers.Control) != 0;
			bool command = (modifiers & MacOSKeyModifiers.Command) != 0;
			bool option = (modifiers & MacOSKeyModifiers.Option) != 0;
			bool shift = (modifiers & MacOSKeyModifiers.Shift) != 0;

			Debug.Print("Modifiers Changed: {0}", modifiers);

			if (keyboard[Key.AltLeft] ^ option)
				keyboard[Key.AltLeft] = option;

			if (keyboard[Key.ShiftLeft] ^ shift)
				keyboard[Key.ShiftLeft] = shift;

			if (keyboard[Key.WinLeft] ^ command)
				keyboard[Key.WinLeft] = command;

			if (keyboard[Key.ControlLeft] ^ control)
				keyboard[Key.ControlLeft] = control;

			if (keyboard[Key.CapsLock] ^ caps)
				keyboard[Key.CapsLock] = caps;

		}

		Rect GetRegion() {
			return API.GetWindowBounds(window.WindowRef, WindowRegionCode.ContentRegion);
		}

		void SetLocation(short x, short y) {
			if (windowState == WindowState.Fullscreen)
				return;

			API.MoveWindow(window.WindowRef, x, y, false);
		}

		void SetSize(short width, short height) {
			if (WindowState == WindowState.Fullscreen)
				return;

			// The bounds of the window should be the size specified, but
			// API.SizeWindow sets the content region size.  So
			// we reduce the size to get the correct bounds.
			width -= (short)(bounds.Width - clientRectangle.Width);
			height -= (short)(bounds.Height - clientRectangle.Height);
			
			API.SizeWindow(window.WindowRef, width, height, true);
		}

		void SetClientSize(short width, short height) {
			if (WindowState == WindowState.Fullscreen)
				return;
			
			API.SizeWindow(window.WindowRef, width, height, true);
		}
		
		protected void OnResize() {
			LoadSize();
			if (Resize != null) {
				Resize(this, EventArgs.Empty);
			}
		}

		private void LoadSize() {
			if (WindowState == WindowState.Fullscreen)
				return;

			Rect r = API.GetWindowBounds(window.WindowRef, WindowRegionCode.StructureRegion);
			bounds = new Rectangle(r.X, r.Y, r.Width, r.Height);

			r = API.GetWindowBounds(window.WindowRef, WindowRegionCode.GlobalPortRegion);
			clientRectangle = new Rectangle(0, 0, r.Width, r.Height);
		}

		#endregion

		#region INativeWindow Members
		
		IntPtr pbStr, utf16, utf8;
		public string GetClipboardText() {
			IntPtr pbRef = GetPasteboard();
			API.PasteboardSynchronize( pbRef );

			uint itemCount;
			OSStatus err = API.PasteboardGetItemCount( pbRef, out itemCount );
			if( err != OSStatus.NoError )
				throw new MacOSException( err, "Getting item count from Pasteboard." );
			if( itemCount < 1 ) return "";

			uint itemID;
			err = API.PasteboardGetItemIdentifier( pbRef, 1, out itemID );
			if( err != OSStatus.NoError )
				throw new MacOSException( err, "Getting item identifier from Pasteboard." );
			
			IntPtr outData;
			if ( (err = API.PasteboardCopyItemFlavorData( pbRef, itemID, utf16, out outData )) == OSStatus.NoError ) {
				IntPtr ptr = API.CFDataGetBytePtr( outData );
				if( ptr == IntPtr.Zero )
					throw new InvalidOperationException( "CFDataGetBytePtr() returned null pointer." );
				return Marshal.PtrToStringUni( ptr );
			}
			// TODO: UTF 8
			return "";
		}
		
		public void SetClipboardText( string value ) {
			IntPtr pbRef = GetPasteboard();
			OSStatus err = API.PasteboardClear( pbRef );
			if( err != OSStatus.NoError )
				throw new MacOSException( err, "Cleaing Pasteboard." );
			API.PasteboardSynchronize( pbRef );

			IntPtr ptr = Marshal.StringToHGlobalUni( value );
			IntPtr cfData = API.CFDataCreate( IntPtr.Zero, ptr, (value.Length + 1) * 2 );
			if( cfData == IntPtr.Zero )
			    throw new InvalidOperationException( "CFDataCreate() returned null pointer." );
			
			API.PasteboardPutItemFlavor( pbRef, 1, utf16, cfData, 0 );
			Marshal.FreeHGlobal( ptr );
		}
		
		IntPtr GetPasteboard() {
			if( pbStr == IntPtr.Zero ) {
				pbStr = CF.CFSTR( "com.apple.pasteboard.clipboard" );
				utf16 = CF.CFSTR( "public.utf16-plain-text" );
				utf8 = CF.CFSTR( "public.utf8-plain-text" );
			}
			
			IntPtr pbRef;
			OSStatus err = API.PasteboardCreate( pbStr, out pbRef );
			if( err != OSStatus.NoError )
				throw new MacOSException( err, "Creating Pasteboard reference." );
			API.PasteboardSynchronize( pbRef );
			return pbRef;
		}

		public void ProcessEvents()
		{
			API.ProcessEvents();
		}

		public Point PointToClient(Point point)
		{
			IntPtr handle = window.WindowRef;
			Rect r = Carbon.API.GetWindowBounds(window.WindowRef, WindowRegionCode.ContentRegion);
			return new Point(point.X - r.X, point.Y - r.Y);
		}
		
		public Point PointToScreen(Point point)
		{
			IntPtr handle = window.WindowRef;
			Rect r = Carbon.API.GetWindowBounds(window.WindowRef, WindowRegionCode.ContentRegion);
			return new Point(point.X + r.X, point.Y + r.Y);
		}

		public bool Exists
		{
			get { return mExists; }
		}

		public IWindowInfo WindowInfo
		{
			get { return window; }
		}

		public Icon Icon {
			get { return mIcon; }
			set {
				SetIcon(value);
			}
		}

		private unsafe void SetIcon(Icon icon)
		{
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
							data[index++] = (IntPtr)(a + (r << 8) + (g << 16) + (b << 24));
						}
						else
						{
							data[index++] = (IntPtr)pixel;
						}
					}
				}

				fixed( IntPtr* ptr = data ) {
					IntPtr provider = API.CGDataProviderCreateWithData(IntPtr.Zero, (IntPtr)(void*)ptr, size * 4, IntPtr.Zero);
					IntPtr image = API.CGImageCreate(128, 128, 8, 32, 4 * 128, API.CGColorSpaceCreateDeviceRGB(), 4, provider, IntPtr.Zero, 0, 0);
					API.SetApplicationDockTileImage(image);
				}
			}
		}

		public string Title
		{
			get
			{
				return title;
			}
			set
			{
				API.SetWindowTitle(window.WindowRef, value);
				title = value;
			}
		}

		public bool Visible
		{
			get { return API.IsWindowVisible(window.WindowRef); }
			set
			{
				if (value && Visible == false)
					Show();
				else
					Hide();
			}
		}

		public bool Focused
		{
			get { return this.mIsActive; }
		}

		public Rectangle Bounds
		{
			get
			{
				
				return bounds;
			}
			set
			{
				Location = value.Location;
				Size = value.Size;
			}
		}

		public Point Location {
			get { return Bounds.Location; }
			set { SetLocation((short)value.X, (short)value.Y); }
		}

		public Size Size {
			get { return bounds.Size; }
			set { SetSize((short)value.Width, (short)value.Height); }
		}

		public int Width
		{
			get { return ClientRectangle.Width; }
			set { SetClientSize((short)value, (short)Height); }
		}

		public int Height
		{
			get { return ClientRectangle.Height; }
			set { SetClientSize((short)Width, (short)value); }
		}

		public int X {
			get { return ClientRectangle.X; }
			set { Location = new Point(value, Y); }
		}

		public int Y {
			get { return ClientRectangle.Y; }
			set { Location = new Point(X, value); }
		}

		public Rectangle ClientRectangle
		{
			get
			{
				return clientRectangle;
			}
			set
			{
				// just set the size, and ignore the location value.
				// this is the behavior of the Windows WinGLNative.
				ClientSize = value.Size;
			}
		}

		public Size ClientSize
		{
			get
			{
				return clientRectangle.Size;
			}
			set
			{
				API.SizeWindow(window.WindowRef, (short)value.Width, (short)value.Height, true);
				OnResize();
			}
		}

		public void Close()
		{
			CancelEventArgs e = new CancelEventArgs();
			OnClosing(e);

			if (e.Cancel)
				return;

			OnClosed();
			Dispose();
		}

		public WindowState WindowState
		{
			get
			{
				if (windowState == WindowState.Fullscreen)
					return WindowState.Fullscreen;

				if (Carbon.API.IsWindowCollapsed(window.WindowRef))
					return WindowState.Minimized;

				if (Carbon.API.IsWindowInStandardState(window.WindowRef))
				{
					return WindowState.Maximized;
				}

				return WindowState.Normal;
			}
			set
			{
				if (value == WindowState)
					return;

				Debug.Print("Switching window state from {0} to {1}", WindowState, value);
				WindowState oldState = WindowState;

				windowState = value;

				if (oldState == WindowState.Fullscreen)
				{
					window.goWindowedHack = true;

					// when returning from full screen, wait until the context is updated
					// to actually do the work.
					return;
				}

				if (oldState == WindowState.Minimized) {
					OSStatus err = API.CollapseWindow(window.WindowRef, false);
					API.CheckReturn( err );
				}

				SetCarbonWindowState();
			}
		}

		private void SetCarbonWindowState() {
			CarbonPoint idealSize;
			OSStatus err;
			
			switch (windowState)
			{
				case WindowState.Fullscreen:
					window.goFullScreenHack = true;
					break;

				case WindowState.Maximized:
					// hack because mac os has no concept of maximized. Instead windows are "zoomed"
					// meaning they are maximized up to their reported ideal size.  So we report a
					// large ideal size.
					idealSize = new CarbonPoint(9000, 9000);
					err = API.ZoomWindowIdeal(window.WindowRef, (short)WindowPartCode.inZoomOut, ref idealSize);
					API.CheckReturn( err );
					break;

				case WindowState.Normal:
					if (WindowState == WindowState.Maximized)
					{
						idealSize = new CarbonPoint();
						err = API.ZoomWindowIdeal(window.WindowRef, (short)WindowPartCode.inZoomIn, ref idealSize);
						API.CheckReturn( err );
					}
					break;

				case WindowState.Minimized:
					err = API.CollapseWindow(window.WindowRef, true);
					API.CheckReturn( err );
					break;
			}

			OnWindowStateChanged();
			OnResize();
		}

		#region --- Event wrappers ---

		private void OnKeyPress(KeyPressEventArgs keyPressArgs)
		{
			if (KeyPress != null)
				KeyPress(this, keyPressArgs);
		}


		private void OnWindowStateChanged()
		{
			if (WindowStateChanged != null)
				WindowStateChanged(this, EventArgs.Empty);
		}

		protected virtual void OnClosing(CancelEventArgs e)
		{
			if (Closing != null)
				Closing(this, e);
		}

		protected virtual void OnClosed()
		{
			if (Closed != null)
				Closed(this, EventArgs.Empty);
		}


		private void OnMouseLeave()
		{
			if (MouseLeave != null)
				MouseLeave(this, EventArgs.Empty);
		}

		private void OnMouseEnter()
		{
			if (MouseEnter != null)
				MouseEnter(this, EventArgs.Empty);
		}

		private void OnActivate()
		{
			mIsActive = true;
			if (FocusedChanged != null)
				FocusedChanged(this, EventArgs.Empty);
		}
		private void OnDeactivate()
		{
			mIsActive = false;
			if (FocusedChanged != null)
				FocusedChanged(this, EventArgs.Empty);
		}

		#endregion

		public event EventHandler<EventArgs> Load;
		public event EventHandler<EventArgs> Unload;
		public event EventHandler<EventArgs> Move;
		public event EventHandler<EventArgs> Resize;
		public event EventHandler<CancelEventArgs> Closing;
		public event EventHandler<EventArgs> Closed;
		public event EventHandler<EventArgs> Disposed;
		public event EventHandler<EventArgs> IconChanged;
		public event EventHandler<EventArgs> TitleChanged;
		public event EventHandler<EventArgs> ClientSizeChanged;
		public event EventHandler<EventArgs> VisibleChanged;
		public event EventHandler<EventArgs> FocusedChanged;
		public event EventHandler<EventArgs> WindowStateChanged;
		public event EventHandler<KeyPressEventArgs> KeyPress;
		public event EventHandler<EventArgs> MouseEnter;
		public event EventHandler<EventArgs> MouseLeave;

		#endregion
		
		#region IInputDriver Members
		
		KeyboardDevice keyboard = new KeyboardDevice();
		MouseDevice mouse = new MouseDevice();
		
		public KeyboardDevice Keyboard {
			get { return keyboard; }
		}

		public MouseDevice Mouse {
			get { return mouse; }
		}
		
		public Point DesktopCursorPos {
			get {
				HIPoint point = default( HIPoint );
				// NOTE: HIGetMousePosition is only available on OSX 10.5 or later
				API.HIGetMousePosition( HICoordinateSpace.ScreenPixel, IntPtr.Zero, ref point );
				return new Point( (int)point.X, (int)point.Y );
			}
			set {
				HIPoint point = default( HIPoint );
				point.X = value.X; point.Y = value.Y;
				CG.CGAssociateMouseAndMouseCursorPosition( 0 );
				CG.CGDisplayMoveCursorToPoint( CG.CGMainDisplayID(), point );
				CG.CGAssociateMouseAndMouseCursorPosition( 1 );
			}
		}
		
		bool visible = true;
		public bool CursorVisible {
			get { return visible; }
			set {
				visible = value;
				if( visible )
					CG.CGDisplayShowCursor(CG.CGMainDisplayID());
				else
					CG.CGDisplayHideCursor(CG.CGMainDisplayID());
			}
		}
		
		#endregion
	}
}