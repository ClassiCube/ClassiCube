#ifndef CC_WINDOW_H
#define CC_WINDOW_H
#include "String.h"
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

enum WINDOW_STATE {
	WINDOW_STATE_NORMAL, WINDOW_STATE_MINIMISED,
	WINDOW_STATE_MAXIMISED, WINDOW_STATE_FULLSCREEN,
};

void Window_Create(int x, int y, int width, int height, const String* title, struct GraphicsMode* mode, struct DisplayDevice* device);
void Window_GetClipboardText(String* value);
void Window_SetClipboardText(const String* value);
/* TODO: IMPLEMENT void Window_SetIcon(Bitmap* bmp); */

bool Window_Exists, Window_Focused;
bool Window_GetVisible(void);
void Window_SetVisible(bool visible);
void* Window_GetWindowHandle(void);
UInt8 Window_GetWindowState(void);
void Window_SetWindowState(UInt8 value);

Rect2D Window_Bounds;
Size2D Window_ClientSize;

void Window_SetBounds(Rect2D rect);
void Window_SetLocation(int x, int y);
void Window_SetSize(int width, int height);
void Window_SetClientSize(int width, int height);

void Window_Close(void);
void Window_ProcessEvents(void);
/* Transforms the specified point from screen to client coordinates. */
Point2D Window_PointToClient(int x, int y);
/* Transforms the specified point from client to screen coordinates. */
Point2D Window_PointToScreen(int x, int y);

Point2D Window_GetDesktopCursorPos(void);
void Window_SetDesktopCursorPos(int x, int y);
bool Window_GetCursorVisible(void);
void Window_SetCursorVisible(bool visible);

#ifndef CC_BUILD_D3D9
void GLContext_Init(struct GraphicsMode mode);
void GLContext_Update(void);
void GLContext_Free(void);

#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)
void* GLContext_GetAddress(const char* function);
void GLContext_SwapBuffers(void);
void GLContext_SetVSync(bool enabled);
#endif
#endif
