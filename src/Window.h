#ifndef CC_WINDOW_H
#define CC_WINDOW_H
#include "String.h"
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

/* The states a window can be in. */
typedef enum WindowState_ {
	WINDOW_STATE_NORMAL, WINDOW_STATE_MINIMISED, WINDOW_STATE_MAXIMISED, WINDOW_STATE_FULLSCREEN
} WindowState;
struct GraphicsMode;

/* Creates a new window of the given size at the given position on screen. */
void Window_Create(int x, int y, int width, int height, struct GraphicsMode* mode);
/* Sets the text of the titlebar above the window. */
void Window_SetTitle(const String* title);
/* Gets the text currently on the clipboard. */
/* NOTE: You must have created a window beforehand. (necessary for X11) */
void Window_GetClipboardText(String* value);
/* Sets the text currently on the clipboard. */
/* NOTE: You must have created a window beforehand. (necessary for X11) */
void Window_SetClipboardText(const String* value);
/* TODO: IMPLEMENT void Window_SetIcon(Bitmap* bmp); */

/* Whether the window is actually valid (i.e. not destroyed). */
extern bool Window_Exists;
/* Whether the user is interacting with the window. */
extern bool Window_Focused;
/* Whether the window is visible on screen at all. */
/* NOTE: This does not count when just hidden behind other windows. */
bool Window_GetVisible(void);
/* Sets whether the window is visible on screen at all. */
void Window_SetVisible(bool visible);
/* Returns the platform-specific handle to the window. */
void* Window_GetWindowHandle(void);
/* Gets the current state of the window, see WindowState enum. */
int Window_GetWindowState(void);
/* Sets the current state of the window, see WindowState enum. */
void Window_SetWindowState(int state);

/* The external bounds of the window in screen coordinates. */
/* Size of external bounds is client size + borders + title */
extern Rect2D Window_Bounds;
/* Size of the internal bounds of the window. */
/* This is the size of area that can be drawn on. (i.e. content size) */
extern Size2D Window_ClientSize;
/* Sets the position and external size of the window. */
void Window_SetBounds(Rect2D rect);
/* Sets the position of the window on the screen. */
void Window_SetLocation(int x, int y);
/* Sets the external size of the window. */
/* NOTE: You usually want to use Window_SetClientSize instead. */
void Window_SetSize(int width, int height);
/* Sets the internal size of the window. */
void Window_SetClientSize(int width, int height);

/* Closes then destroys the window. */
/* Raises the WindowClosing and WindowClosed events. */
void Window_Close(void);
/* Processes all pending window messages/events. */
void Window_ProcessEvents(void);
/* Converts the specified point from screen to client coordinates. */
Point2D Window_PointToClient(int x, int y);
/* Converts the specified point from client to screen coordinates. */
Point2D Window_PointToScreen(int x, int y);

/* Gets the position of the cursor in screen coordinates. */
Point2D Window_GetScreenCursorPos(void);
/* Sets the position of the cursor in screen coordinates. */
void Window_SetScreenCursorPos(int x, int y);
/* Whether the cursor is visible when over this window. */
bool Window_GetCursorVisible(void);
/* Sets whether the cursor is visible when over this window. */
/* NOTE: You must be careful with this! OS typically uses a counter for visibility,
so setting invisible multiple times means you must then set visible multiple times. */
void Window_SetCursorVisible(bool visible);

/* Shows a dialog box window. */
CC_EXPORT void Window_ShowDialog(const char* title, const char* msg);
/* Initialises the internal state for being able to set window's pixels. */
/* NOTE: Do not manually free bmp->Scan0 - it may be allocated by system. */
/* NOTE: This function must also be called whenever the window is resized. */
void Window_InitRaw(Bitmap* bmp);
/* Updates the window's pixels using the bitmap from Window_InitRaw. */
/* r can be used to only update a small region of pixels (may be ignored) */
void Window_DrawRaw(Rect2D r);

#ifndef CC_BUILD_D3D9
/* Initialises an OpenGL context that most closely matches the input arguments. */
/* NOTE: You must have created a window beforehand, as the GL context is attached to the window. */
void GLContext_Init(struct GraphicsMode* mode);
/* Updates the OpenGL context after the window is resized. */
void GLContext_Update(void);
/* Destroys the OpenGL context. */
/* NOTE: This also unattaches the OpenGL context from the window. */
void GLContext_Free(void);

#define GLContext_IsInvalidAddress(ptr) (ptr == (void*)0 || ptr == (void*)1 || ptr == (void*)-1 || ptr == (void*)2)
/* Returns the address of a function pointer for the given OpenGL function. */
/* NOTE: The platform may still return an address for unsupported functions. 
You must check the OpenGL version and/or GL_EXTENSIONS string for actual support! */
void* GLContext_GetAddress(const char* function);
/* Swaps the front and back buffer, displaying the back buffer on screen. */
void GLContext_SwapBuffers(void);
/* Sets whether synchronisation with the monitor is used. */
/* NOTE: The underlying platform may choose to still ignore this. */
void GLContext_SetVSync(bool enabled);
#endif
#endif
