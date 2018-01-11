#ifndef CC_WINDOW_H
#define CC_WINDOW_H
#include "Typedefs.h"
#include "String.h"
#include "Compiler.h"
#include "Bitmap.h"
#include "2DStructs.h"
#include "DisplayDevice.h"
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

/* The window is in its normal state. */
#define WINDOW_STATE_NORMAL 0
/* The window is minimized to the taskbar (also known as 'iconified'). */
#define WINDOW_STATE_MINIMISED 1
/* The window covers the whole working area, which includes the desktop but not the taskbar and/or panels. */
#define WINDOW_STATE_MAXIMISED 2
/* The window covers the whole screen, including all taskbars and/or panels. */
#define WINDOW_STATE_FULLSCREEN 3

/* Creates a new window. */
void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_REF String* title, DisplayDevice* device);
/* Gets the current contents of the clipboard. */
void Window_GetClipboardText(STRING_TRANSIENT String* value);
/* Sets the current contents of the clipboard. */
void Window_SetClipboardText(STRING_PURE String* value);
/* TODO: IMPLEMENT THIS
/* Sets the icon of this window. */
/*void Window_SetIcon(Bitmap* bmp);
*/

/* Gets whether this window has input focus. */
bool Window_GetFocused(void);
/* Gets whether whether this window is visible. */
bool Window_GetVisible(void);
/* Sets whether this window is visible. */
void Window_SetVisible(bool visible);
/* Gets whether this window has been created and has not been destroyed. */
bool Window_GetExists(void);
/* Gets the handle of this window. */
void* Window_GetWindowHandle(void);
/* Gets the WindowState of this window. */
UInt8 Window_GetWindowState(void);
/* Sets the WindowState of this window. */
void Window_SetWindowState(UInt8 value);

/* Gets the external bounds of this window, in screen coordinates. 
External bounds include title bar, borders and drawing area of the window. */
Rectangle2D Window_GetBounds(void);
/* Sets the external bounds of this window, in screen coordinates. */
void Window_SetBounds(Rectangle2D rect);
/* Returns the location of this window on the desktop. */
Point2D Window_GetLocation(void);
/* Sets the location of this window on the desktop. */
void Window_SetLocation(Point2D point);
/* Gets the external size of this window. */
Size2D Window_GetSize(void);
/* Sets the external size of this window. */
void Window_SetSize(Size2D size);
/* Gets the internal bounds of this window, in client coordinates. 
Internal bounds include the drawing area of the window, but exclude the titlebar and window borders.*/
Rectangle2D Window_GetClientRectangle(void);
/* Sets the internal bounds of this window, in client coordinates. */
void Window_SetClientRectangle(Rectangle2D rect);
/* Gets the internal size of this window. */
Size2D Window_GetClientSize(void);
/* Sets the internal size of this window. */
void Window_SetClientSize(Size2D size);

/* Closes this window. */
void Window_Close(void);
/* Processes pending window events. */
void Window_ProcessEvents(void);
/* Transforms the specified point from screen to client coordinates. */
Point2D Window_PointToClient(Point2D point);
/* Transforms the specified point from client to screen coordinates. */
Point2D Window_PointToScreen(Point2D point);

/* Gets the cursor position in screen coordinates. */
Point2D Window_GetDesktopCursorPos(void);
/* Sets the cursor position in screen coordinates. */
void Window_SetDesktopCursorPos(Point2D point);
/* Gets whether the cursor is visible in the window. */
bool Window_GetCursorVisible(void);
/* Sets whether the cursor is visible in the window. */
void Window_SetCursorVisible(bool visible);

#if !USE_DX
/* Initalises the OpenGL context, and makes it current. */
void GLContext_Init(GraphicsMode mode);
/* Updates OpenGL context on a window resize. */
void GLContext_Update(void);
/* Frees the OpenGL context. */
void GLContext_Free(void);

#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)
/* Retrieves the address of the given OpenGL function. */
void* GLContext_GetAddress(const UInt8* function);
/* Swaps the front and back buffer of the OpenGL context. */
void GLContext_SwapBuffers(void);
/* Gets whether the OpenGL context has VSync enabled. */
bool GLContext_GetVSync(void);
/* Sets whether the OpenGL context uses VSync. */
void GLContext_SetVSync(bool enabled);
#endif
#endif