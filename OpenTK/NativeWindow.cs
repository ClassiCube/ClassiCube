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
using OpenTK.Graphics;
using OpenTK.Input;
using OpenTK.Platform;

namespace OpenTK {

	/// <summary> Instances of this class implement the <see cref="OpenTK.INativeWindow"/> interface on the current platform. </summary>
	public class NativeWindow : INativeWindow {
		
		private readonly GameWindowFlags options;
		private readonly DisplayDevice device;
		private readonly INativeWindow implementation;
		private bool disposed, events;
		
		/// <summary>Constructs a new NativeWindow with default attributes without enabling events.</summary>
		public NativeWindow()
			: this(640, 480, "OpenTK Native Window", GameWindowFlags.Default, GraphicsMode.Default, DisplayDevice.Default) { }

		/// <summary>Constructs a new centered NativeWindow with the specified attributes.</summary>
		/// <param name="width">The width of the NativeWindow in pixels.</param>
		/// <param name="height">The height of the NativeWindow in pixels.</param>
		/// <param name="title">The title of the NativeWindow.</param>
		/// <param name="options">GameWindow options specifying window appearance and behavior.</param>
		/// <param name="mode">The OpenTK.Graphics.GraphicsMode of the NativeWindow.</param>
		/// <param name="device">The OpenTK.Graphics.DisplayDevice to construct the NativeWindow in.</param>
		/// <exception cref="System.ArgumentOutOfRangeException">If width or height is less than 1.</exception>
		/// <exception cref="System.ArgumentNullException">If mode or device is null.</exception>
		public NativeWindow(int width, int height, string title, GameWindowFlags options, GraphicsMode mode, DisplayDevice device)
			: this(device.Bounds.Left + (device.Bounds.Width - width) / 2,
			       device.Bounds.Top + (device.Bounds.Height - height) / 2,
			       width, height, title, options, mode, device) { }

		/// <summary>Constructs a new NativeWindow with the specified attributes.</summary>
		/// <param name="x">Horizontal screen space coordinate of the NativeWindow's origin.</param>
		/// <param name="y">Vertical screen space coordinate of the NativeWindow's origin.</param>
		/// <param name="width">The width of the NativeWindow in pixels.</param>
		/// <param name="height">The height of the NativeWindow in pixels.</param>
		/// <param name="title">The title of the NativeWindow.</param>
		/// <param name="options">GameWindow options specifying window appearance and behavior.</param>
		/// <param name="mode">The OpenTK.Graphics.GraphicsMode of the NativeWindow.</param>
		/// <param name="device">The OpenTK.Graphics.DisplayDevice to construct the NativeWindow in.</param>
		/// <exception cref="System.ArgumentOutOfRangeException">If width or height is less than 1.</exception>
		/// <exception cref="System.ArgumentNullException">If mode or device is null.</exception>
		public NativeWindow(int x, int y, int width, int height, string title, GameWindowFlags options, GraphicsMode mode, DisplayDevice device) {
			// TODO: Should a constraint be added for the position?
			if (width < 1)
				throw new ArgumentOutOfRangeException("width", "Must be greater than zero.");
			if (height < 1)
				throw new ArgumentOutOfRangeException("height", "Must be greater than zero.");
			if (mode == null)
				throw new ArgumentNullException("mode");
			if (device == null)
				throw new ArgumentNullException("device");

			this.options = options;
			this.device = device;

			implementation = Factory.Default.CreateNativeWindow(x, y, width, height, title, mode, options, this.device);

			if ((options & GameWindowFlags.Fullscreen) != 0) {
				this.device.ChangeResolution(width, height, mode.ColorFormat.BitsPerPixel, 0);
				WindowState = WindowState.Fullscreen;
			}
		}

		/// <summary> Closes the NativeWindow. </summary>
		public void Close() {
			EnsureUndisposed();
			implementation.Close();
		}
		
		/// <summary> Gets the current contents of the clipboard. </summary>
		public string GetClipboardText() {
			EnsureUndisposed(); return implementation.GetClipboardText();
		}
		
		/// <summary> Sets the current contents of the clipboard. </summary>
		public void SetClipboardText(string value) {
			EnsureUndisposed(); implementation.SetClipboardText(value);
		}		

		/// <summary> Transforms the specified point from screen to client coordinates. </summary>
		/// <param name="point"> A <see cref="System.Drawing.Point"/> to transform. </param>
		/// <returns> The point transformed to client coordinates. </returns>
		public Point PointToClient(Point point)  {
			return implementation.PointToClient(point);
		}

		/// <summary> Transforms the specified point from client to screen coordinates. </summary>
		/// <param name="point"> A <see cref="System.Drawing.Point"/> to transform. </param>
		/// <returns> The point transformed to screen coordinates. </returns>
		public Point PointToScreen(Point point) {
			// Here we use the fact that PointToClient just translates the point, and PointToScreen
			// should perform the inverse operation.
			Point trans = PointToClient(Point.Empty);
			point.X -= trans.X;
			point.Y -= trans.Y;
			return point;
		}
		
		/// <summary> Processes operating system events until the NativeWindow becomes idle. </summary>
		public void ProcessEvents() {
			ProcessEvents(false);
		}
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Rectangle"/> structure that contains the external bounds of this window, in screen coordinates.
		/// External bounds include the title bar, borders and drawing area of the window. </summary>
		public Rectangle Bounds {
			get { EnsureUndisposed(); return implementation.Bounds; }
			set { EnsureUndisposed(); implementation.Bounds = value; }
		}

		/// <summary> Gets or sets a <see cref="System.Drawing.Rectangle"/> structure that contains the internal bounds of this window, in client coordinates.
		/// The internal bounds include the drawing area of the window, but exclude the titlebar and window borders. </summary>
		public Rectangle ClientRectangle {
			get { EnsureUndisposed(); return implementation.ClientRectangle; }
			set { EnsureUndisposed(); implementation.ClientRectangle = value; }
		}

		/// <summary> Gets or sets a <see cref="System.Drawing.Size"/> structure that contains the internal size this window. </summary>
		public Size ClientSize {
			get { EnsureUndisposed(); return implementation.ClientSize; }
			set { EnsureUndisposed(); implementation.ClientSize = value;
			}
		}

		/// <summary> Gets a value indicating whether a render window exists. </summary>
		public bool Exists {
			get {
				return IsDisposed ? false : implementation.Exists; // TODO: Should disposed be ignored instead?
			}
		}

		/// <summary> Gets a System.Boolean that indicates whether this NativeWindow has input focus. </summary>
		public bool Focused {
			get { EnsureUndisposed(); return implementation.Focused; }
		}
		
		/// <summary> Gets or sets the external height of this window. </summary>
		public int Height {
			get { EnsureUndisposed(); return implementation.Height; }
			set { EnsureUndisposed(); implementation.Height = value; }
		}

		/// <summary> Gets or sets the System.Drawing.Icon for this GameWindow. </summary>
		public Icon Icon {
			get { EnsureUndisposed(); return implementation.Icon; }
			set { EnsureUndisposed(); implementation.Icon = value; }
		}
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Point"/> structure that contains the location of this window on the desktop. </summary>
		public Point Location {
			get { EnsureUndisposed(); return implementation.Location; }
			set { EnsureUndisposed(); implementation.Location = value; }
		}
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Size"/> structure that contains the external size of this window. </summary>
		public Size Size {
			get { EnsureUndisposed(); return implementation.Size; }
			set { EnsureUndisposed(); implementation.Size = value; }
		}
		
		/// <summary> Gets or sets the NativeWindow title. </summary>
		public string Title {
			get { EnsureUndisposed(); return implementation.Title; }
			set { EnsureUndisposed(); implementation.Title = value; }
		}
		
		/// <summary> Gets or sets a System.Boolean that indicates whether this NativeWindow is visible. </summary>
		public bool Visible {
			get { EnsureUndisposed(); return implementation.Visible; }
			set { EnsureUndisposed(); implementation.Visible = value; }
		}

		/// <summary> Gets or sets the external width of this window. </summary>
		public int Width {
			get { EnsureUndisposed(); return implementation.Width; }
			set { EnsureUndisposed(); implementation.Width = value; }
		}
		
		/// <summary> Gets the <see cref="OpenTK.Platform.IWindowInfo"/> of this window. </summary>
		public IWindowInfo WindowInfo {
			get {  EnsureUndisposed(); return implementation.WindowInfo; }
		}

		/// <summary> Gets or states the state of the NativeWindow. </summary>
		public virtual WindowState WindowState {
			get { return implementation.WindowState; }
			set { implementation.WindowState = value; }
		}
		
		/// <summary> Gets or sets the horizontal location of this window on the desktop. </summary>
		public int X {
			get { EnsureUndisposed(); return implementation.X; }
			set { EnsureUndisposed(); implementation.X = value; }
		}
		
		/// <summary> Gets or sets the vertical location of this window on the desktop. </summary>
		public int Y {
			get { EnsureUndisposed(); return implementation.Y; }
			set { EnsureUndisposed(); implementation.Y = value; }
		}
		
		/// <summary> Gets the primary Keyboard device, or null if no Keyboard exists. </summary>
		public KeyboardDevice Keyboard {
			get { return implementation.Keyboard; }
		}

		/// <summary> Gets the primary Mouse device, or null if no Mouse exists. </summary>
		public MouseDevice Mouse {
			get { return implementation.Mouse; }
		}
		
		/// <summary> Sets whether the cursor is visible in the window. </summary>
		public bool CursorVisible {
			get { return implementation.CursorVisible; }
			set { implementation.CursorVisible = value; }
		}
		
		/// <summary> Gets or sets the cursor position in screen coordinates. </summary>
		public Point DesktopCursorPos {
			get { return implementation.DesktopCursorPos; }
			set { implementation.DesktopCursorPos = value; }
		}
		
		/// <summary> Occurs after the window has closed. </summary>
		public event EventHandler<EventArgs> Closed;

		/// <summary> Occurs when the window is about to close. </summary>
		public event EventHandler<CancelEventArgs> Closing;

		/// <summary> Occurs when the window is disposed. </summary>
		public event EventHandler<EventArgs> Disposed;

		/// <summary> Occurs when the <see cref="Focused"/> property of the window changes. </summary>
		public event EventHandler<EventArgs> FocusedChanged;

		/// <summary> Occurs when the <see cref="Icon"/> property of the window changes. </summary>
		public event EventHandler<EventArgs> IconChanged;

		/// <summary> Occurs whenever a character is typed. </summary>
		public event EventHandler<KeyPressEventArgs> KeyPress;

		/// <summary> Occurs whenever the window is moved. </summary>
		public event EventHandler<EventArgs> Move;

		/// <summary> Occurs whenever the mouse cursor enters the window <see cref="Bounds"/>. </summary>
		public event EventHandler<EventArgs> MouseEnter;
		
		/// <summary> Occurs whenever the mouse cursor leaves the window <see cref="Bounds"/>. </summary>
		public event EventHandler<EventArgs> MouseLeave;

		/// <summary> Occurs whenever the window is resized. </summary>
		public event EventHandler<EventArgs> Resize;

		/// <summary> Occurs when the <see cref="Title"/> property of the window changes. </summary>
		public event EventHandler<EventArgs> TitleChanged;

		/// <summary> Occurs when the <see cref="Visible"/> property of the window changes. </summary>
		public event EventHandler<EventArgs> VisibleChanged;

		/// <summary> Occurs when the <see cref="WindowState"/> property of the window changes. </summary>
		public event EventHandler<EventArgs> WindowStateChanged;
		
		/// <summary> Releases all non-managed resources belonging to this NativeWindow. </summary>
		public virtual void Dispose() {
			if (!IsDisposed) {
				if ((options & GameWindowFlags.Fullscreen) != 0) {
					//if (WindowState == WindowState.Fullscreen) WindowState = WindowState.Normal; // TODO: Revise.
					device.RestoreResolution();
				}
				implementation.Dispose();
				GC.SuppressFinalize(this);
				IsDisposed = true;
			}
		}
		
		/// <summary> Ensures that this NativeWindow has not been disposed. </summary>
		/// <exception cref="System.ObjectDisposedException"> If this NativeWindow has been disposed. </exception>
		protected void EnsureUndisposed() {
			if (disposed) throw new ObjectDisposedException(GetType().Name);
		}
		
		/// <summary> Gets or sets a <see cref="System.Boolean"/>, which indicates whether this instance has been disposed. </summary>
		protected bool IsDisposed {
			get { return disposed; }
			set { disposed = value; }
		}
		
		/// <summary> Called when the NativeWindow has closed. </summary>
		/// <param name="e">Not used.</param>
		protected virtual void OnClosed(object sender, EventArgs e) {
			if (Closed != null) Closed(this, e);
		}

		/// <summary> Called when the NativeWindow is about to close. </summary>
		/// <param name="e"> The <see cref="System.ComponentModel.CancelEventArgs" /> for this event.
		/// Set e.Cancel to true in order to stop the NativeWindow from closing.</param>
		protected virtual void OnClosing(object sender, CancelEventArgs e) {
			if (Closing != null) Closing(this, e);
		}
		
		/// <summary> Called when the NativeWindow is disposed. </summary>
		protected virtual void OnDisposed(object sender, EventArgs e) {
			if (Disposed != null) Disposed(this, e);
		}

		/// <summary> Called when the <see cref="OpenTK.INativeWindow.Focused"/> property of the NativeWindow has changed. </summary>
		protected virtual void OnFocusedChanged(object sender, EventArgs e) {
			if (FocusedChanged != null) FocusedChanged(this, e);
		}
		
		/// <summary> Called when the <see cref="OpenTK.INativeWindow.Icon"/> property of the NativeWindow has changed. </summary>
		protected virtual void OnIconChanged(object sender, EventArgs e) {
			if (IconChanged != null) IconChanged(this, e);
		}
		
		/// <summary> Called when a character is typed. </summary>
		/// <param name="e">The <see cref="OpenTK.KeyPressEventArgs"/> for this event.</param>
		protected virtual void OnKeyPress(object sender, KeyPressEventArgs e) {
			if (KeyPress != null) KeyPress(this, e);
		}
		
		/// <summary> Called when the NativeWindow is moved. </summary>
		protected virtual void OnMove(object sender, EventArgs e) {
			if (Move != null) Move(this, e);
		}

		/// <summary> Called whenever the mouse cursor reenters the window <see cref="Bounds"/>. </summary>
		protected virtual void OnMouseEnter(object sender, EventArgs e) {
			if (MouseEnter != null) MouseEnter(this, e);
		}

		/// <summary> Called whenever the mouse cursor leaves the window <see cref="Bounds"/>. </summary>
		protected virtual void OnMouseLeave(object sender, EventArgs e) {
			if (MouseLeave != null) MouseLeave(this, e);
		}
		
		/// <summary> Called when the NativeWindow is resized. </summary>
		/// <param name="e">Not used.</param>
		protected virtual void OnResize(object sender, EventArgs e) {
			if (Resize != null) Resize(this, e);
		}
		
		/// <summary> Called when the <see cref="OpenTK.INativeWindow.Title"/> property of the NativeWindow has changed. </summary>
		protected virtual void OnTitleChanged(object sender, EventArgs e) {
			if (TitleChanged != null) TitleChanged(this, e);
		}

		/// <summary> Called when the <see cref="OpenTK.INativeWindow.Visible"/> property of the NativeWindow has changed. </summary>
		protected virtual void OnVisibleChanged(object sender, EventArgs e) {
			if (VisibleChanged != null) VisibleChanged(this, e);
		}
		
		/// <summary> Called when the WindowState of this NativeWindow has changed. </summary>
		protected virtual void OnWindowStateChanged(object sender, EventArgs e) {
			if (WindowStateChanged != null) WindowStateChanged(this, e);
		}

		/// <summary> Processes operating system events until the NativeWindow becomes idle. </summary>
		/// <param name="retainEvents">If true, the state of underlying system event propagation will be preserved, otherwise event propagation will be enabled if it has not been already.</param>
		protected void ProcessEvents(bool retainEvents) {
			EnsureUndisposed();
			if (!retainEvents && !events) Events = true;
			implementation.ProcessEvents();
		}
		
		private void OnClosedInternal(object sender, EventArgs e)  {
			OnClosed(null, e);
			Events = false;
		}
		
		private bool Events {
			set {
				if (value) {
					if (events) {
						throw new InvalidOperationException("Event propagation is already enabled.");
					}
					implementation.Closed += OnClosed;
					implementation.Closing += OnClosing;
					implementation.Disposed += OnDisposed;
					implementation.FocusedChanged += OnFocusedChanged;
					implementation.IconChanged += OnIconChanged;
					implementation.KeyPress += OnKeyPress;
					implementation.MouseEnter += OnMouseEnter;
					implementation.MouseLeave += OnMouseLeave;
					implementation.Move += OnMove;
					implementation.Resize += OnResize;
					implementation.TitleChanged += OnTitleChanged;
					implementation.VisibleChanged += OnVisibleChanged;
					implementation.WindowStateChanged += OnWindowStateChanged;
					events = true;
				} else if (events) {
					implementation.Closed -= OnClosed;
					implementation.Closing -= OnClosing;
					implementation.Disposed -= OnDisposed;
					implementation.FocusedChanged -= OnFocusedChanged;
					implementation.IconChanged -= OnIconChanged;
					implementation.KeyPress -= OnKeyPress;
					implementation.MouseEnter -= OnMouseEnter;
					implementation.MouseLeave -= OnMouseLeave;
					implementation.Move -= OnMove;
					implementation.Resize -= OnResize;
					implementation.TitleChanged -= OnTitleChanged;
					implementation.VisibleChanged -= OnVisibleChanged;
					implementation.WindowStateChanged -= OnWindowStateChanged;
					events = false;
				} else {
					throw new InvalidOperationException("Event propagation is already disabled.");
				}
			}
		}
	}
}
