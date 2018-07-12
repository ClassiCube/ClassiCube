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

namespace OpenTK {
	
	public class KeyPressEventArgs : EventArgs { public char KeyChar; }
	/// <summary> Enumerates available window states. </summary>
	public enum WindowState {
		/// <summary> The window is in its normal state. </summary>
		Normal = 0,
		/// <summary> The window is minimized to the taskbar (also known as 'iconified'). </summary>
		Minimized,
		/// <summary> The window covers the whole working area, which includes the desktop but not the taskbar and/or panels. </summary>
		Maximized,
		/// <summary> The window covers the whole screen, including all taskbars and/or panels. </summary>
		Fullscreen,
	}
	
	/// <summary> Defines the interface for a native window.  </summary>
	public abstract class INativeWindow : IDisposable {
		
		public abstract string GetClipboardText();
		public abstract void SetClipboardText(string value);
		public abstract Icon Icon { get; set; }
		
		/// <summary> Gets or sets the handle of the window. </summary>
		public IntPtr WinHandle;
		
		/// <summary> Gets a System.Boolean that indicates whether this window has input focus. </summary>
		public abstract bool Focused { get; }
		
		/// <summary> Gets or sets a System.Boolean that indicates whether the window is visible. </summary>
		public abstract bool Visible { get; set; }
		
		/// <summary> Gets a System.Boolean that indicates whether the window has been created and has not been destroyed. </summary>
		public abstract bool Exists { get; }
		
		public abstract WindowState WindowState { get; set; }

		/// <summary> Gets or sets a <see cref="System.Drawing.Rectangle"/> structure the contains the external bounds of this window, in screen coordinates.
		/// External bounds include the title bar, borders and drawing area of the window. </summary>
		public abstract Rectangle Bounds { get; set; }
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Point"/> structure that contains the location of this window on the desktop. </summary>
		public abstract Point Location { get; set; }
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Size"/> structure that contains the external size of this window. </summary>
		public abstract Size Size { get; set; }
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Rectangle"/> structure that contains the internal bounds of this window, in client coordinates.
		/// The internal bounds include the drawing area of the window, but exclude the titlebar and window borders. </summary>
		public abstract Rectangle ClientRectangle { get; set; }
		
		/// <summary> Gets or sets a <see cref="System.Drawing.Size"/> structure that contains the internal size this window. </summary>
		public abstract Size ClientSize { get; set; }

		/// <summary> Closes this window. </summary>
		public abstract void Close();
		
		/// <summary> Processes pending window events. </summary>
		public abstract void ProcessEvents();
		
		/// <summary> Transforms the specified point from screen to client coordinates.  </summary>
		/// <param name="point"> A <see cref="System.Drawing.Point"/> to transform. </param>
		/// <returns> The point transformed to client coordinates. </returns>
		public abstract Point PointToClient(Point point);
		
		/// <summary> Transforms the specified point from client to screen coordinates. </summary>
		/// <param name="point"> A <see cref="System.Drawing.Point"/> to transform. </param>
		/// <returns> The point transformed to screen coordinates. </returns>
		public abstract Point PointToScreen(Point point);
		/*public virtual Point PointToScreen(Point point) {
			// Here we use the fact that PointToClient just translates the point, and PointToScreen
			// should perform the inverse operation.
			Point trans = PointToClient(Point.Empty);
			point.X -= trans.X;
			point.Y -= trans.Y;
			return point;
		}*/
		
		/// <summary> Gets or sets the cursor position in screen coordinates. </summary>
		public abstract Point DesktopCursorPos { get; set; }
		
		/// <summary> Gets or sets whether the cursor is visible in the window. </summary>
		public abstract bool CursorVisible { get; set; }

		public event EventHandler Move;
		protected void RaiseMove() { Raise(Move); }

		public event EventHandler Resize;
		protected void RaiseResize() { Raise(Resize); }
		
		public event EventHandler Redraw;
		protected void RaiseRedraw() { Raise(Redraw); }

		public event EventHandler Closing;
		protected void RaiseClosing() { Raise(Closing); }

		public event EventHandler Closed;
		protected void RaiseClosed() { Raise(Closed); }

		public event EventHandler VisibleChanged;
		protected void RaiseVisibleChanged() { Raise(VisibleChanged); }

		public event EventHandler FocusedChanged;
		protected void RaiseFocusedChanged() { Raise(FocusedChanged); }

		public event EventHandler WindowStateChanged;
		protected void RaiseWindowStateChanged() { Raise(WindowStateChanged); }

		/// <summary> Occurs whenever a character is typed. </summary>
		public event EventHandler<KeyPressEventArgs> KeyPress;
		KeyPressEventArgs pressArgs = new KeyPressEventArgs();
		
		protected void RaiseKeyPress(char key) {
			pressArgs.KeyChar = key;
			if (KeyPress != null) KeyPress(this, pressArgs);
		}
		
		protected void Raise(EventHandler handler) {
			if (handler != null) handler(this, EventArgs.Empty);
		}
		
		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}
		
		protected abstract void Dispose(bool calledManually);
		
		~INativeWindow() { Dispose(false); }
	}
}
