#ifndef CC_INPUT_H
#define CC_INPUT_H
#include "String.h"
/* Manages keyboard, mouse, and touch state.
   Raises events when keys are pressed etc, and implements base handlers for them.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
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

	/* NOTE: RMOUSE must be before MMOUSE for PlayerClick compatibility */
	KEY_XBUTTON1, KEY_XBUTTON2, KEY_LMOUSE, KEY_RMOUSE, KEY_MMOUSE,
	INPUT_COUNT
};
typedef int Key;

/* Simple names for each keyboard button. */
extern const char* const Input_Names[INPUT_COUNT];

#define Key_IsWinPressed()     (Input_Pressed[KEY_LWIN]   || Input_Pressed[KEY_RWIN])
#define Key_IsAltPressed()     (Input_Pressed[KEY_LALT]   || Input_Pressed[KEY_RALT])
#define Key_IsControlPressed() (Input_Pressed[KEY_LCTRL]  || Input_Pressed[KEY_RCTRL])
#define Key_IsShiftPressed()   (Input_Pressed[KEY_LSHIFT] || Input_Pressed[KEY_RSHIFT])

#ifdef CC_BUILD_OSX
/* osx uses CMD instead of CTRL for clipboard and stuff */
#define Key_IsActionPressed() Key_IsWinPressed()
#else
#define Key_IsActionPressed() Key_IsControlPressed()
#endif

/* Pressed state of each keyboard button. Use Input_SetPressed to change. */
extern bool Input_Pressed[INPUT_COUNT];
/* Sets the pressed state of a keyboard button. */
/* Raises InputEvents.Up   if not pressed, but was pressed before. */
/* Raises InputEvents.Down if pressed (repeating is whether it was pressed before) */
void Input_SetPressed(Key key, bool pressed);
/* Resets all keyboard keys to released state. */
/* Raises InputEvents.Up for each previously pressed key. */
void Key_Clear(void);
typedef int MouseButton;

/* Whether raw mouse/touch input is being listened for. */
extern bool Input_RawMode;
/* Whether touch input is being used. */
extern bool Input_TouchMode;

#ifdef CC_BUILD_TOUCH
#define INPUT_MAX_POINTERS 32
extern int Pointers_Count;

void Input_AddTouch(long id,    int x, int y);
void Input_UpdateTouch(long id, int x, int y);
void Input_RemoveTouch(long id, int x, int y);
#else
#define INPUT_MAX_POINTERS 1
#define Pointers_Count 1
#endif

/* Data for mouse and touch */
extern struct Pointer { int x, y; } Pointers[INPUT_MAX_POINTERS];
/* X and Y coordinates of the mouse. Use Mouse_SetPosition to change. */
extern int Mouse_X, Mouse_Y;

/* Raises PointerEvents.Up or PointerEvents.Down. */
void Pointer_SetPressed(int idx, bool pressed);
/* Raises InputEvents.Wheel with the given wheel delta. */
void Mouse_ScrollWheel(float delta);
/* Sets X and Y position of the given pointer, always raising PointerEvents.Moved. */
void Pointer_SetPosition(int idx, int x, int y);


/* Enumeration of all key bindings. */
enum KeyBind_ {
	KEYBIND_FORWARD, KEYBIND_BACK, KEYBIND_LEFT, KEYBIND_RIGHT,
	KEYBIND_JUMP, KEYBIND_RESPAWN, KEYBIND_SET_SPAWN, KEYBIND_CHAT,
	KEYBIND_INVENTORY, KEYBIND_FOG, KEYBIND_SEND_CHAT, KEYBIND_PLAYER_LIST,
	KEYBIND_SPEED, KEYBIND_NOCLIP, KEYBIND_FLY, KEYBIND_FLY_UP, KEYBIND_FLY_DOWN,
	KEYBIND_EXT_INPUT, KEYBIND_HIDE_FPS, KEYBIND_SCREENSHOT, KEYBIND_FULLSCREEN,
	KEYBIND_THIRD_PERSON, KEYBIND_HIDE_GUI, KEYBIND_AXIS_LINES, KEYBIND_ZOOM_SCROLL,
	KEYBIND_HALF_SPEED, KEYBIND_DELETE_BLOCK, KEYBIND_PICK_BLOCK, KEYBIND_PLACE_BLOCK,
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
/* Called when user has removed a hotkey. (removes it from options) */
void Hotkeys_UserRemovedHotkey(Key trigger, cc_uint8 modifiers);
/* Called when user has added a hotkey. (Adds it to options) */
void Hotkeys_UserAddedHotkey(Key trigger, cc_uint8 modifiers, bool moreInput, const String* text);

bool InputHandler_SetFOV(int fov);
void InputHandler_PickBlocks(void);
void InputHandler_Init(void);
void InputHandler_OnScreensChanged(void);
#endif
