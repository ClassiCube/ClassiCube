#ifndef CC_INPUT_H
#define CC_INPUT_H
#include "String.h"
/* Manages the keyboard, and raises events when keys are pressed etc.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3 | Based on OpenTK code
*/

enum Key_ {
	KEY_NONE, /* Unrecognised key */

	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
	KEY_F11, KEY_F12, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20,
	KEY_F21, KEY_F22, KEY_F23, KEY_F24, KEY_F25, KEY_F26, KEY_F27, KEY_F28, KEY_F29, KEY_F30,
	KEY_F31, KEY_F32, KEY_F33, KEY_F34, KEY_F35,

	KEY_LSHIFT, KEY_RSHIFT, KEY_LCTRL, KEY_RCTRL,
	KEY_LALT, KEY_RALT, KEY_LWIN, KEY_RWIN,

	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

	KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
	KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, /* same as '0'-'9' */

	KEY_INSERT, KEY_DELETE, KEY_HOME, KEY_END, KEY_PAGEUP, KEY_PAGEDOWN,
	KEY_MENU,

	KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
	KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
	KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, /* same as 'A'-'Z' */

	KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_BACKSPACE, KEY_TAB, KEY_CAPSLOCK,
	KEY_SCROLLLOCK, KEY_PRINTSCREEN, KEY_PAUSE, KEY_NUMLOCK,

	KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4,
	KEY_KP5, KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9,
	KEY_KP_DIVIDE, KEY_KP_MULTIPLY, KEY_KP_MINUS,
	KEY_KP_PLUS, KEY_KP_DECIMAL, KEY_KP_ENTER,

	KEY_TILDE, KEY_MINUS, KEY_EQUALS, KEY_LBRACKET, KEY_RBRACKET, KEY_SLASH,
	KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA, KEY_PERIOD, KEY_BACKSLASH,

	KEY_XBUTTON1, KEY_XBUTTON2, KEY_MOUSEMID, /* so these can be used for hotkeys */
	KEY_COUNT
};
typedef int Key;

/* Simple names for each keyboard button. */
extern const char* const Key_Names[KEY_COUNT];

#define Key_IsWinPressed()     (Key_Pressed[KEY_LWIN]   || Key_Pressed[KEY_RWIN])
#define Key_IsAltPressed()     (Key_Pressed[KEY_LALT]   || Key_Pressed[KEY_RALT])
#define Key_IsControlPressed() (Key_Pressed[KEY_LCTRL]  || Key_Pressed[KEY_RCTRL])
#define Key_IsShiftPressed()   (Key_Pressed[KEY_LSHIFT] || Key_Pressed[KEY_RSHIFT])

#ifdef CC_BUILD_OSX
/* osx uses CMD instead of CTRL for clipboard and stuff */
#define Key_IsActionPressed() Key_IsWinPressed()
#else
#define Key_IsActionPressed() Key_IsControlPressed()
#endif

/* Pressed state of each keyboard button. Use Key_SetPressed to change. */
extern bool Key_Pressed[KEY_COUNT];
/* Sets the pressed state of a keyboard button. */
/* Raises KeyEvents.Up   if not pressed, but was pressed before. */
/* Raises KeyEvents.Down if pressed (repeating is whether it was pressed before) */
void Key_SetPressed(Key key, bool pressed);
/* Resets all keys to be not pressed. */
/* Raises KeyEvents.Up for each previously pressed key. */
void Key_Clear(void);


enum MouseButton_ {
	MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE,
	MOUSE_COUNT
};
typedef int MouseButton;

/* Wheel position of the mouse. Use Mouse_SetWheel to change. */
extern float Mouse_Wheel;
/* X and Y coordinates of the mouse. Use Mouse_SetPosition to change. */
extern int Mouse_X, Mouse_Y;

/* Pressed state of each mouse button. Use Mouse_SetPressed to change. */
extern bool Mouse_Pressed[MOUSE_COUNT];
/* Sets the pressed state of a mouse button. */
/* Raises MouseEvents.Up or MouseEvents.Down if state differs. */
void Mouse_SetPressed(MouseButton btn, bool pressed);
/* Sets wheel position of the mouse, always raising MouseEvents.Wheel. */
void Mouse_SetWheel(float wheel);
/* Sets X and Y position of the mouse, always raising MouseEvents.Moved. */
void Mouse_SetPosition(int x, int y);


/* Enumeration of all key bindings. */
enum KeyBind_ {
	KEYBIND_FORWARD, KEYBIND_BACK, KEYBIND_LEFT, KEYBIND_RIGHT,
	KEYBIND_JUMP, KEYBIND_RESPAWN, KEYBIND_SET_SPAWN, KEYBIND_CHAT,
	KEYBIND_INVENTORY, KEYBIND_FOG, KEYBIND_SEND_CHAT, KEYBIND_PLAYER_LIST,
	KEYBIND_SPEED, KEYBIND_NOCLIP, KEYBIND_FLY, KEYBIND_FLY_UP, KEYBIND_FLY_DOWN,
	KEYBIND_EXT_INPUT, KEYBIND_HIDE_FPS, KEYBIND_SCREENSHOT, KEYBIND_FULLSCREEN,
	KEYBIND_THIRD_PERSON, KEYBIND_HIDE_GUI, KEYBIND_AXIS_LINES, KEYBIND_ZOOM_SCROLL,
	KEYBIND_HALF_SPEED, KEYBIND_MOUSE_LEFT, KEYBIND_PICK_BLOCK, KEYBIND_MOUSE_RIGHT,
	KEYBIND_AUTOROTATE, KEYBIND_HOTBAR_SWITCH, KEYBIND_SMOOTH_CAMERA,
	KEYBIND_DROP_BLOCK, KEYBIND_IDOVERLAY, KEYBIND_BREAK_LIQUIDS,
	KEYBIND_COUNT
};
typedef int KeyBind;

/* The keys that are bound to each key binding. */
extern cc_uint8 KeyBinds[KEYBIND_COUNT];
/* Default key that each key binding is bound to */
extern const cc_uint8 KeyBind_Defaults[KEYBIND_COUNT];

/* Gets whether the key bound to the given key binding is pressed. */
bool KeyBind_IsPressed(KeyBind binding);
/* Set the key that the given key binding is bound to. (also updates options list) */
void KeyBind_Set(KeyBind binding, Key key);
/* Initalises and loads key bindings from options. */
void KeyBind_Init(void);


extern const cc_uint8 Hotkeys_LWJGL[256];
struct HotkeyData {
	int TextIndex;     /* contents to copy directly into the input bar */
	cc_uint8 Trigger;   /* Member of Key enumeration */
	cc_uint8 Flags;     /* HotkeyModifiers bitflags */
	bool StaysOpen;    /* whether the user is able to enter further input */
};

#define HOTKEYS_MAX_COUNT 256
extern struct HotkeyData HotkeysList[HOTKEYS_MAX_COUNT];
extern StringsBuffer HotkeysText;
enum HotkeyModifiers {
	HOTKEY_MOD_CTRL = 1, HOTKEY_MOD_SHIFT = 2, HOTKEY_MOD_ALT = 4
};

/* Adds or updates a new hotkey. */
void Hotkeys_Add(Key trigger, cc_uint8 modifiers, const String* text, bool more);
/* Removes the given hotkey. */
bool Hotkeys_Remove(Key trigger, cc_uint8 modifiers);
/* Returns the first hotkey which is bound to the given key and has its modifiers pressed. */
/* NOTE: The hotkeys list is sorted, so hotkeys with most modifiers are checked first. */
int Hotkeys_FindPartial(Key key);
/* Initalises and loads hotkeys from options. */
void Hotkeys_Init(void);
/* Called when user has removed a hotkey. (removes it from options) */
void Hotkeys_UserRemovedHotkey(Key trigger, cc_uint8 modifiers);
/* Called when user has added a hotkey. (Adds it to options) */
void Hotkeys_UserAddedHotkey(Key trigger, cc_uint8 modifiers, bool moreInput, const String* text);
#endif
