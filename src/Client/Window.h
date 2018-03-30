#ifndef CC_WINDOW_H
#define CC_WINDOW_H
#include "String.h"
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

#define WINDOW_STATE_NORMAL 0
#define WINDOW_STATE_MINIMISED 1
#define WINDOW_STATE_MAXIMISED 2
#define WINDOW_STATE_FULLSCREEN 3

void Window_Create(Int32 x, Int32 y, Int32 width, Int32 height, STRING_REF String* title, DisplayDevice* device);
void Window_GetClipboardText(STRING_TRANSIENT String* value);
void Window_SetClipboardText(STRING_PURE String* value);
/* TODO: IMPLEMENT THIS  void Window_SetIcon(Bitmap* bmp); */

bool Window_GetFocused(void);
bool Window_GetVisible(void);
void Window_SetVisible(bool visible);
bool Window_GetExists(void);
void* Window_GetWindowHandle(void);
UInt8 Window_GetWindowState(void);
void Window_SetWindowState(UInt8 value);

Rectangle2D Window_GetBounds(void);
void Window_SetBounds(Rectangle2D rect);
Point2D Window_GetLocation(void);
void Window_SetLocation(Point2D point);
Size2D Window_GetSize(void);
void Window_SetSize(Size2D size);
Rectangle2D Window_GetClientRectangle(void);
void Window_SetClientRectangle(Rectangle2D rect);
Size2D Window_GetClientSize(void);
void Window_SetClientSize(Size2D size);

void Window_Close(void);
void Window_ProcessEvents(void);
/* Transforms the specified point from screen to client coordinates. */
Point2D Window_PointToClient(Point2D point);
/* Transforms the specified point from client to screen coordinates. */
Point2D Window_PointToScreen(Point2D point);

Point2D Window_GetDesktopCursorPos(void);
void Window_SetDesktopCursorPos(Point2D point);
bool Window_GetCursorVisible(void);
void Window_SetCursorVisible(bool visible);

#if !USE_DX
void GLContext_Init(GraphicsMode mode);
void GLContext_Update(void);
void GLContext_Free(void);

#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)
void* GLContext_GetAddress(const UInt8* function);
void GLContext_SwapBuffers(void);
bool GLContext_GetVSync(void);
void GLContext_SetVSync(bool enabled);
#endif
#endif