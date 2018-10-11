#ifndef CC_INPUT_H
#define CC_INPUT_H
#include "String.h"
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
	Key_None, /* Unrecognised key */

	Key_ShiftLeft, Key_ShiftRight, Key_ControlLeft, Key_ControlRight,
	Key_AltLeft, Key_AltRight, Key_WinLeft, Key_WinRight, Key_Menu,

	Key_F1,  Key_F2,  Key_F3,  Key_F4,  Key_F5,  Key_F6,  Key_F7,  Key_F8,  Key_F9,  Key_F10,
	Key_F11, Key_F12, Key_F13, Key_F14, Key_F15, Key_F16, Key_F17, Key_F18, Key_F19, Key_F20,
	Key_F21, Key_F22, Key_F23, Key_F24, Key_F25, Key_F26, Key_F27, Key_F28, Key_F29, Key_F30,
	Key_F31, Key_F32, Key_F33, Key_F34, Key_F35,

	Key_Up, Key_Down, Key_Left, Key_Right,

	Key_Enter, Key_Escape, Key_Space, Key_Tab, Key_BackSpace, Key_Insert,
	Key_Delete, Key_PageUp, Key_PageDown, Key_Home, Key_End, Key_CapsLock,
	Key_ScrollLock, Key_PrintScreen, Key_Pause, Key_NumLock,

	Key_Keypad0, Key_Keypad1, Key_Keypad2, Key_Keypad3, Key_Keypad4,
	Key_Keypad5, Key_Keypad6, Key_Keypad7, Key_Keypad8, Key_Keypad9,
	Key_KeypadDivide, Key_KeypadMultiply, Key_KeypadSubtract,
	Key_KeypadAdd, Key_KeypadDecimal, Key_KeypadEnter,

	Key_A, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J,
	Key_K, Key_L, Key_M, Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T,
	Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,

	Key_0, Key_1, Key_2, Key_3, Key_4,
	Key_5, Key_6, Key_7, Key_8, Key_9,

	Key_Tilde, Key_Minus, Key_Plus, Key_BracketLeft, Key_BracketRight,
	Key_Semicolon, Key_Quote, Key_Comma, Key_Period, Key_Slash, Key_BackSlash,

	Key_XButton1, Key_XButton2, /* so these can be used for hotkeys */
	Key_Count,
} Key;

/* Gets whether key repeating is on or not. When on, multiple KeyDown events are raised when the same key is 
held down for a period of time (frequency depends on platform). Should be on for menu input, off for game input. */
bool Key_KeyRepeat;
/* Simple names for each keyboard button. */
extern const char* Key_Names[Key_Count];

#define Key_IsWinPressed()     (Key_Pressed[Key_WinLeft]     || Key_Pressed[Key_WinRight])
#define Key_IsAltPressed()     (Key_Pressed[Key_AltLeft]     || Key_Pressed[Key_AltRight])
#define Key_IsControlPressed() (Key_Pressed[Key_ControlLeft] || Key_Pressed[Key_ControlRight])
#define Key_IsShiftPressed()   (Key_Pressed[Key_ShiftLeft]   || Key_Pressed[Key_ShiftRight])

/* Pressed state of each keyboard button. Use Key_SetPressed to change. */
bool Key_Pressed[Key_Count];
/* Sets the pressed state of a keyboard button. */
/* Raises KeyEvents_Up or KeyEvents_Down if state differs, or Key_KeyRepeat is on. */
void Key_SetPressed(Key key, bool pressed);
/* Resets all keys to not pressed state. */
/* Raises KeyEvents_Up for each previously pressed key. */
void Key_Clear(void);


typedef enum MouseButton_ {
	MouseButton_Left, MouseButton_Right, MouseButton_Middle,
	MouseButton_Count,
} MouseButton;

/* Wheel position of the mouse. Use Mouse_SetWheel to change. */
float Mouse_Wheel;
/* X and Y coordinates of the mouse. Use Mouse_SetPosition to change. */
int Mouse_X, Mouse_Y;

/* Pressed state of each mouse button. Use Mouse_SetPressed to change. */
bool Mouse_Pressed[MouseButton_Count];
/* Sets the pressed state of a mouse button. */
/* Raises MouseEvents_Up or MouseEvents_Down if state differs. */
void Mouse_SetPressed(MouseButton btn, bool pressed);
/* Sets wheel position of the mouse, always raising MouseEvents_Wheel. */
void Mouse_SetWheel(float wheel);
/* Sets X and Y position of the mouse, always raising MouseEvents_Moved. */
void Mouse_SetPosition(int x, int y);


/* Enumeration of all key bindings. */
typedef enum KeyBind_ {
	KeyBind_Forward, KeyBind_Back, KeyBind_Left, KeyBind_Right, 
	KeyBind_Jump, KeyBind_Respawn, KeyBind_SetSpawn, KeyBind_Chat,
	KeyBind_Inventory, KeyBind_ToggleFog, KeyBind_SendChat, KeyBind_PauseOrExit, 
	KeyBind_PlayerList, KeyBind_Speed, KeyBind_NoClip, KeyBind_Fly, 
	KeyBind_FlyUp, KeyBind_FlyDown, KeyBind_ExtInput, KeyBind_HideFps,
	KeyBind_Screenshot, KeyBind_Fullscreen, KeyBind_ThirdPerson, KeyBind_HideGui, 
	KeyBind_AxisLines, KeyBind_ZoomScrolling, KeyBind_HalfSpeed, KeyBind_MouseLeft, 
	KeyBind_MouseMiddle, KeyBind_MouseRight, KeyBind_Autorotate, KeyBind_HotbarSwitching, 
	KeyBind_SmoothCamera, KeyBind_DropBlock, KeyBind_IDOverlay, KeyBind_BreakableLiquids,
	KeyBind_Count
} KeyBind;

/* Gets the key that is bound to the the given key binding. */
Key KeyBind_Get(KeyBind binding);
/* Gets the default key that the given key binding is bound to */
Key KeyBind_GetDefault(KeyBind binding);
/* Gets whether the key bound to the given key binding is pressed. */
bool KeyBind_IsPressed(KeyBind binding);
/* Set the key that the given key binding is bound to. (also updates options list) */
void KeyBind_Set(KeyBind binding, Key key);
/* Initalises and loads key bindings from options. */
void KeyBind_Init(void);


extern uint8_t Hotkeys_LWJGL[256];
struct HotkeyData {
	int TextIndex;   /* contents to copy directly into the input bar */
	uint8_t Trigger; /* Member of Key enumeration */
	uint8_t Flags;   /* ctrl 1, shift 2, alt 4 */
	bool StaysOpen;  /* whether the user is able to enter further input */
};

#define HOTKEYS_MAX_COUNT 256
struct HotkeyData HotkeysList[HOTKEYS_MAX_COUNT];
StringsBuffer HotkeysText;
enum HOTKEY_FLAGS {
	HOTKEY_FLAG_CTRL = 1, HOTKEY_FLAG_SHIFT = 2, HOTKEY_FLAG_ALT = 4,
};

/* Adds or updates a new hotkey. */
void Hotkeys_Add(Key trigger, int flags, const String* text, bool more);
/* Removes the given hotkey. */
bool Hotkeys_Remove(Key trigger, int flags);
/* Returns the first hotkey which is bound to the given key and has its modifiers pressed. */
/* NOTE: The hotkeys list is sorted, so hotkeys with most modifiers are checked first. */
int Hotkeys_FindPartial(Key key);
/* Initalises and loads hotkeys from options. */
void Hotkeys_Init(void);
/* Called when user has removed a hotkey. (removes it from options) */
void Hotkeys_UserRemovedHotkey(Key trigger, int flags);
/* Called when user has added a hotkey. (Adds it to options) */
void Hotkeys_UserAddedHotkey(Key trigger, int flags, bool moreInput, const String* text);
#endif
