#ifndef CC_WINDOW_H
#define CC_WINDOW_H
#include "Core.h"
/* Abstracts creating and managing the native window.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3 | Based on OpenTK code
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

struct Bitmap;
struct DynamicLibSym;
/* The states the window can be in. */
enum WindowState  { WINDOW_STATE_NORMAL, WINDOW_STATE_FULLSCREEN, WINDOW_STATE_MINIMISED };
enum KeyboardType { KEYBOARD_TYPE_TEXT, KEYBOARD_TYPE_NUMBER, KEYBOARD_TYPE_PASSWORD };
enum SoftKeyboard { SOFT_KEYBOARD_NONE, SOFT_KEYBOARD_RESIZE, SOFT_KEYBOARD_SHIFT };
/* (can't name these structs Window/Display, as that conflicts with X11's Window/Display typedef) */

/* Data for the display monitor. */
CC_VAR extern struct _DisplayData {
	/* Number of bits per pixel. (red bits + green bits + blue bits + alpha bits) */
	/* NOTE: Only 24 or 32 bits per pixel are officially supported. */
	/* Support for other values of bits per pixel is platform dependent. */
	int Depth;
	/* Scale usually based on number of physical dots per inch of the display. (horizontally and vertically) */
	/* NOTE: This may just be the display scaling set in user's settings, not actual DPI. */
	/* NOTE: Usually 1 for compatibility, even if the display's DPI is really higher. */
	/* GUI elements must be scaled by this to look correct. */
	float ScaleX, ScaleY;
	/* Position of this display. (may be non-zero in multi-monitor setup. Platform dependent) */
	int X, Y;
	/* Size/Dimensions of this display in pixels. */
	int Width, Height;
} DisplayInfo;

/* Scales the given X coordinate from 96 dpi to current display dpi. */
int Display_ScaleX(int x);
/* Scales the given Y coordinate from 96 dpi to current display dpi. */
int Display_ScaleY(int y);

/* Data for the game/launcher window. */
CC_VAR extern struct _WinData {
	/* Readonly platform-specific handle to the window. */
	void* Handle;
	/* Size of the content area of the window. (i.e. area that can draw to) */
	/* This area does NOT include borders and titlebar surrounding the window. */
	int Width, Height;
	/* Whether the window is actually valid (i.e. not destroyed). */
	cc_bool Exists;
	/* Whether the user is interacting with the window. */
	cc_bool Focused;
	/* The type of on-screen keyboard this platform supports. (usually SOFT_KEYBOARD_NONE) */
	cc_uint8 SoftKeyboard;
} WindowInfo;

/* Initialises state for window. Also sets Display_ members. */
void Window_Init(void);
/* Creates the window as the given size at centre of the screen. */
void Window_Create(int width, int height);
/* Sets the text of the titlebar above the window. */
CC_API void Window_SetTitle(const cc_string* title);

typedef void (*RequestClipboardCallback)(cc_string* value, void* obj);
/* Gets the text currently on the clipboard. */
CC_API void Clipboard_GetText(cc_string* value);
/* Sets the text currently on the clipboard. */
CC_API void Clipboard_SetText(const cc_string* value);
/* Calls a callback function when text is retrieved from the clipboard. */
/* NOTE: On most platforms this just calls Clipboard_GetText. */
/* With emscripten however, the callback is instead called when a 'paste' event arrives. */
void Clipboard_RequestText(RequestClipboardCallback callback, void* obj);

/* Makes the window visible and focussed on screen. */
void Window_Show(void);
/* Gets the current state of the window, see WindowState enum. */
int Window_GetWindowState(void);
/* Attempts to switch the window to occupy the entire screen. */
cc_result Window_EnterFullscreen(void);
/* Attempts to restore the window to before it entered full screen. */
cc_result Window_ExitFullscreen(void);

/* Sets the size of the internal bounds of the window in pixels. */
/* NOTE: This size excludes the bounds of borders + title */
void Window_SetSize(int width, int height);
/* Closes then destroys the window. */
/* Raises the WindowClosing and WindowClosed events. */
void Window_Close(void);
/* Processes all pending window messages/events. */
void Window_ProcessEvents(void);

/* Sets the position of the cursor. */
/* NOTE: This should be avoided because it is unsupported on some platforms. */
void Cursor_SetPosition(int x, int y);
/* Sets whether the cursor is visible when over this window. */
/* NOTE: You MUST BE VERY CAREFUL with this! OS typically uses a counter for visibility, */
/*  so setting invisible multiple times means you must then set visible multiple times. */
void Cursor_SetVisible(cc_bool visible);

/* Shows a dialog box window. */
CC_API void Window_ShowDialog(const char* title, const char* msg);
typedef void (*OpenFileDialogCallback)(const cc_string* path);
/* Shows an 'load file' dialog window. */
cc_result Window_OpenFileDialog(const char* filter, OpenFileDialogCallback callback);

/* Allocates a framebuffer that can be drawn/transferred to the window. */
/* NOTE: Do NOT free bmp->Scan0, use Window_FreeFramebuffer. */
/* NOTE: This MUST be called whenever the window is resized. */
void Window_AllocFramebuffer(struct Bitmap* bmp);
/* Transfers pixels from the allocated framebuffer to the on-screen window. */
/* r can be used to only update a small region of pixels (may be ignored) */
void Window_DrawFramebuffer(Rect2D r);
/* Frees the previously allocated framebuffer. */
void Window_FreeFramebuffer(struct Bitmap* bmp);

struct OpenKeyboardArgs { const cc_string* text; int type; const char* placeholder; };
void OpenKeyboardArgs_Init(struct OpenKeyboardArgs* args, STRING_REF const cc_string* text, int type);
/* Displays on-screen keyboard for platforms that lack physical keyboard input. */
/* NOTE: On desktop platforms, this won't do anything. */
void Window_OpenKeyboard(const struct OpenKeyboardArgs* args);
/* Sets the text used for keyboard input. */
/* NOTE: This is only used for mobile on-screen keyboard input with the web client, */
/*  because it is backed by a HTML input, rather than true keyboard input events. */
/* As such, this is necessary to ensure the HTML input is consistent with */
/*  whatever text input widget is actually being displayed on screen. */
void Window_SetKeyboardText(const cc_string* text);
/* Hides/Removes the previously displayed on-screen keyboard. */
void Window_CloseKeyboard(void);

/* Begins listening for raw input and starts raising PointerEvents.RawMoved. */
/* NOTE: Some backends only raise it when Window_UpdateRawMouse is called. */
/* Cursor will also be hidden and moved to window centre. */
void Window_EnableRawMouse(void);
/* Updates mouse state. (such as centreing cursor) */
void Window_UpdateRawMouse(void);
/* Disables listening for raw input and stops raising PointerEvents.RawMoved */
/* Cursor will also be unhidden and moved back to window centre. */
void Window_DisableRawMouse(void);

#ifdef CC_BUILD_GL
/* Creates an OpenGL context, then makes it the active context. */
/* NOTE: You MUST have created the window beforehand, as the GL context is attached to the window. */
void GLContext_Create(void);
/* Updates the OpenGL context after the window is resized. */
void GLContext_Update(void);
/* Attempts to restore a lost OpenGL context. */
cc_bool GLContext_TryRestore(void);
/* Destroys the OpenGL context. */
/* NOTE: This also unattaches the OpenGL context from the window. */
void GLContext_Free(void);

/* Returns the address of a function pointer for the given OpenGL function. */
/* NOTE: The implementation may still return an address for unsupported functions! */
/* You MUST check the OpenGL version and/or GL_EXTENSIONS string for actual support! */
void* GLContext_GetAddress(const char* function);
/* Loads all OpenGL function pointers using GLContext_GetAddress in the given list */
void GLContext_GetAll(const struct DynamicLibSym* syms, int count);

/* Swaps the front and back buffer, displaying the back buffer on screen. */
cc_bool GLContext_SwapBuffers(void);
/* Sets whether synchronisation with the monitor is enabled. */
/* NOTE: The implementation may choose to still ignore this. */
void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs);
/* Gets OpenGL context specific graphics information. */
void GLContext_GetApiInfo(cc_string* info);
#endif
#endif
