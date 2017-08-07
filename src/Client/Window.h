#ifndef CS_WINDOW_H
#define CS_WINDOW_H
#include "Typedefs.h"
#include "String.h"
#include "Compiler.h"
#include "Bitmap.h"
/* Abstracts creating and managing a native window.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3 | Based on OpenTK code
*/
/*
   The Open Toolkit Library License
  
   Copyright (c) 2006 - 2009 the Open Toolkit library.
  
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
   the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
  
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
  
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   */

/* Gets the current contents of the clipboard. */
void Window_GetClipboardText(STRING_TRANSIENT String* value);

/* Sets the current contents of the clipboard. */
void Window_SetClipboardText(STRING_TRANSIENT String* value);

/* Sets the icon of this window. */
void Window_SetIcon(Bitmap* bmp);

/* Gets whether this window has input focus. */
bool Window_GetFocused(void);

/* Gets whether whether this window is visible. */
bool Window_GetVisible(void);

/* Gets whether this window has been created and has not been destroyed. */
bool Window_GetExists(void);

		/// <summary> Gets the <see cref="OpenTK.Platform.IWindowInfo"/> for this window. </summary>
	IWindowInfo WindowInfo{ get; }

		/// <summary> Gets or sets the <see cref="OpenTK.WindowState"/> for this window. </summary>
	WindowState WindowState{ get; set; }

		/// <summary> Gets or sets a <see cref="System.Drawing.Rectangle"/> structure the contains the external bounds of this window, in screen coordinates.
		/// External bounds include the title bar, borders and drawing area of the window. </summary>
	Rectangle Bounds{ get; set; }

		/// <summary> Gets or sets a <see cref="System.Drawing.Point"/> structure that contains the location of this window on the desktop. </summary>
	Point Location{ get; set; }

		/// <summary> Gets or sets a <see cref="System.Drawing.Size"/> structure that contains the external size of this window. </summary>
	Size Size{ get; set; }

		/// <summary> Gets or sets a <see cref="System.Drawing.Rectangle"/> structure that contains the internal bounds of this window, in client coordinates.
		/// The internal bounds include the drawing area of the window, but exclude the titlebar and window borders. </summary>
	Rectangle ClientRectangle{ get; set; }

		/// <summary> Gets or sets a <see cref="System.Drawing.Size"/> structure that contains the internal size this window. </summary>
	Size ClientSize{ get; set; }

		/// <summary> Closes this window. </summary>
	void Close();

	/// <summary> Processes pending window events. </summary>
	void ProcessEvents();

	/// <summary> Transforms the specified point from screen to client coordinates.  </summary>
	/// <param name="point"> A <see cref="System.Drawing.Point"/> to transform. </param>
	/// <returns> The point transformed to client coordinates. </returns>
	Point PointToClient(Point point);

	/// <summary> Transforms the specified point from client to screen coordinates. </summary>
	/// <param name="point"> A <see cref="System.Drawing.Point"/> to transform. </param>
	/// <returns> The point transformed to screen coordinates. </returns>
	Point PointToScreen(Point point);

	/// <summary> Gets the available KeyboardDevice. </summary>
	KeyboardDevice Keyboard{ get; }

		/// <summary> Gets the available MouseDevice. </summary>
	MouseDevice Mouse{ get; }

		/// <summary> Gets or sets the cursor position in screen coordinates. </summary>
	Point DesktopCursorPos{ get; set; }

		/// <summary> Gets or sets whether the cursor is visible in the window. </summary>
	bool CursorVisible{ get; set; }

		/// <summary> Occurs whenever the window is moved. </summary>
	event EventHandler<EventArgs> Move;

	/// <summary> Occurs whenever the window is resized. </summary>
	event EventHandler<EventArgs> Resize;

	/// <summary> Occurs when the window is about to close. </summary>
	event EventHandler<CancelEventArgs> Closing;

	/// <summary> Occurs after the window has closed. </summary>
	event EventHandler<EventArgs> Closed;

	/// <summary> Occurs when the window is disposed. </summary>
	event EventHandler<EventArgs> Disposed;

	/// <summary> Occurs when the <see cref="Visible"/> property of the window changes. </summary>
	event EventHandler<EventArgs> VisibleChanged;

	/// <summary> Occurs when the <see cref="Focused"/> property of the window changes. </summary>
	event EventHandler<EventArgs> FocusedChanged;

	/// <summary> Occurs when the <see cref="WindowState"/> property of the window changes. </summary>
	event EventHandler<EventArgs> WindowStateChanged;

	/// <summary> Occurs whenever a character is typed. </summary>
	event EventHandler<KeyPressEventArgs> KeyPress;

	/// <summary> Occurs whenever the mouse cursor leaves the window <see cref="Bounds"/>. </summary>
	event EventHandler<EventArgs> MouseLeave;

	/// <summary> Occurs whenever the mouse cursor enters the window <see cref="Bounds"/>. </summary>
	event EventHandler<EventArgs> MouseEnter;

#endif