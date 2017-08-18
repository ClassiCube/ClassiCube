#ifndef CS_KEY_H
#define CS_KEY_H
#include "Typedefs.h"
/* Manages the keyboard, and raises events when keys are pressed etc.
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

typedef enum Key_ {
	/* Key outside the known keys */
	Key_Unknown = 0,

	/* Modifiers */
	Key_ShiftLeft, Key_ShiftRight, Key_ControlLeft, Key_ControlRight,
	Key_AltLeft, Key_AltRight, Key_WinLeft, Key_WinRight, Key_Menu,

	/* Function keys (hopefully enough for most keyboards - mine has 26)
	/ <keysymdef.h> on X11 reports up to 35 function keys. */
	Key_F1,  Key_F2,  Key_F3,  Key_F4,  Key_F5,  Key_F6,  Key_F7,  Key_F8,  Key_F9,  Key_F10,
	Key_F11, Key_F12, Key_F13, Key_F14, Key_F15, Key_F16, Key_F17, Key_F18, Key_F19, Key_F20,
	Key_F21, Key_F22, Key_F23, Key_F24, Key_F25, Key_F26, Key_F27, Key_F28, Key_F29, Key_F30,
	Key_F31, Key_F32, Key_F33, Key_F34, Key_F35,

	/* Direction arrows */
	Key_Up, Key_Down, Key_Left, Key_Right,

	/* Action keys */
	Key_Enter, Key_Escape, Key_Space, Key_Tab, Key_BackSpace, Key_Insert,
	Key_Delete, Key_PageUp, Key_PageDown, Key_Home, Key_End, Key_CapsLock,
	Key_ScrollLock, Key_PrintScreen, Key_Pause, Key_NumLock,

	// Keypad keys
	Key_Keypad0, Key_Keypad1, Key_Keypad2, Key_Keypad3, Key_Keypad4,
	Key_Keypad5, Key_Keypad6, Key_Keypad7, Key_Keypad8, Key_Keypad9,
	Key_KeypadDivide, Key_KeypadMultiply, Key_KeypadSubtract,
	Key_KeypadAdd, Key_KeypadDecimal, Key_KeypadEnter,

	/* Letters */
	Key_A, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J,
	Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T,
	Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,

	/* Numbers */
	Key_Number0, Key_Number1, Key_Number2, Key_Number3, Key_Number4,
	Key_Number5, Key_Number6, Key_Number7, Key_Number8, Key_Number9,

	/* Symbols */
	Key_Tilde, Key_Minus, Key_Plus, Key_BracketLeft, Key_BracketRight,
	Key_Semicolon, Key_Quote, Key_Comma, Key_Period, Key_Slash, Key_BackSlash,

	Key_Count,
} Key;

/* Gets whether the given key is currently being pressed. */
bool Key_GetPressed(Key key);
/* Sets whether the given key is currently being pressed. */
void Key_SetPressed(Key key, bool pressed);

/* Gets whether key repeating is on or not. If on (desirable for text input), multiple KeyDowns (varies by OS) 
are generated for the same key when it is held down for a period of time. Should be off for game input. */
bool Key_KeyRepeat;

/* Unpresses all keys that were previously pressed. */
void Key_Clear(void);
#endif