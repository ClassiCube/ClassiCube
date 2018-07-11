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
using OpenTK.Input;
using OpenTK.Platform;

namespace OpenTK {
	
	/// <summary> Defines the interface for a native window.  </summary>
	public abstract class INativeWindow : IDisposable {
		
		/// <summary> Gets the current contents of the clipboard. </summary>
		public abstract string GetClipboardText();
		
		/// <summary> Sets the current contents of the clipboard. </summary>
		public abstract void SetClipboardText(string value);
		
		/// <summary> Gets or sets the <see cref="System.Drawing.Icon"/> of the window. </summary>
		public abstract Icon Icon { get; set; }
		
		/// <summary> Gets a System.Boolean that indicates whether this window has input focus. </summary>
		public abstract bool Focused { get; }
		
		/// <summary> Gets or sets a System.Boolean that indicates whether the window is visible. </summary>
		public abstract bool Visible { get; set; }
		
		/// <summary> Gets a System.Boolean that indicates whether the window has been created and has not been destroyed. </summary>
		public abstract bool Exists { get; }
		
		/// <summary> Gets the <see cref="OpenTK.Platform.IWindowInfo"/> for this window. </summary>
		public abstract IWindowInfo WindowInfo { get; }
		
		/// <summary> Gets or sets the <see cref="OpenTK.WindowState"/> for this window. </summary>
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

		/// <summary> Occurs whenever the window is moved. </summary>
		public event EventHandler Move;
		protected void RaiseMove() {
			if (Move != null) Move(this, EventArgs.Empty);
		}

		/// <summary> Occurs whenever the window is resized. </summary>
		public event EventHandler Resize;
		protected void RaiseResize() {
			if (Resize != null) Resize(this, EventArgs.Empty);
		}

		/// <summary> Occurs when the window is about to close. </summary>
		public event EventHandler Closing;
		protected void RaiseClosing() {
			if (Closing != null) Closing(this, EventArgs.Empty);
		}

		/// <summary> Occurs after the window has closed. </summary>
		public event EventHandler Closed;
		protected void RaiseClosed() {
			if (Closed != null) Closed(this, EventArgs.Empty);
		}

		/// <summary> Occurs when the <see cref="Visible"/> property of the window changes. </summary>
		public event EventHandler VisibleChanged;
		protected void RaiseVisibleChanged() {
			if (VisibleChanged != null) VisibleChanged(this, EventArgs.Empty);
		}

		/// <summary> Occurs when the <see cref="Focused"/> property of the window changes. </summary>
		public event EventHandler FocusedChanged;
		protected void RaiseFocusedChanged() {
			if (FocusedChanged != null) FocusedChanged(this, EventArgs.Empty);
		}

		/// <summary> Occurs when the <see cref="WindowState"/> property of the window changes. </summary>
		public event EventHandler WindowStateChanged;
		protected void RaiseWindowStateChanged() {
			if (WindowStateChanged != null) WindowStateChanged(this, EventArgs.Empty);
		}

		/// <summary> Occurs whenever a character is typed. </summary>
		public event EventHandler<KeyPressEventArgs> KeyPress;
		KeyPressEventArgs pressArgs = new KeyPressEventArgs();
		protected void RaiseKeyPress(char key) {
			pressArgs.KeyChar = key;
			if (KeyPress != null) KeyPress(this, pressArgs);
		}
		
		public void Dispose() {
        	Dispose(true);
        	GC.SuppressFinalize( this );
        }
        
        protected abstract void Dispose(bool calledManually);
        
        ~INativeWindow() { Dispose(false); }
	}
}
