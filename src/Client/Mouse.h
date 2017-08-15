#ifndef CS_MOUSE_H
#define CS_MOUSE_H
#include "Typedefs.h"
/* Manages the mouse, and raises events when mouse buttons are pressed, moved, wheel scrolled, etc.
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

typedef enum MouseButton_ {
	/* The left mouse button. */
	MouseButton_Left,
	/* The right mouse button. */
	MouseButton_Right,
	/* The middle mouse button. */
	MouseButton_Middle,
	/* The first extra mouse button. */
	MouseButton_Button1,
	/* The second extra mouse button. */
	MouseButton_Button2,

	MouseButton_Count,
} MouseButton;

/* Gets whether the given mouse button is currently being pressed. */
bool Mouse_GetPressed(MouseButton btn);
/* Sets whether the given mouse button is currently being pressed. */
void Mouse_SetPressed(MouseButton btn, bool pressed);

/* Gets the current wheel value of the mouse. */
Real32 Mouse_Wheel;
/* Gets the current position of the mouse. */
Int32 Mouse_X, Mouse_Y;

/* Sets the current wheel value of the mouse. */
void Mouse_SetWheel(Real32 wheel);
/* Sets the current position of the mouse. */
void Mouse_SetPosition(Int32 x, Int32 y);
#endif