#ifndef CC_INPUT_H
#define CC_INPUT_H
#include "Core.h"
/* 
Manages input state, raising input related events, and base input handling
Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct StringsBuffer;
extern struct IGameComponent Input_Component;

enum InputButtons {
	IPT_NONE, /* Unrecognised key */

	IPT_F1, IPT_F2, IPT_F3, IPT_F4, IPT_F5, IPT_F6, IPT_F7, IPT_F8, IPT_F9, IPT_F10,
	IPT_F11, IPT_F12, IPT_F13, IPT_F14, IPT_F15, IPT_F16, IPT_F17, IPT_F18, IPT_F19, IPT_F20,
	IPT_F21, IPT_F22, IPT_F23, IPT_F24,

	IPT_TILDE, IPT_MINUS, IPT_EQUALS, IPT_LBRACKET, IPT_RBRACKET, IPT_SLASH,
	IPT_SEMICOLON, IPT_QUOTE, IPT_COMMA, IPT_PERIOD, IPT_BACKSLASH,

	IPT_LSHIFT, IPT_RSHIFT, IPT_LCTRL, IPT_RCTRL,
	IPT_LALT, IPT_RALT, IPT_LWIN, IPT_RWIN,

	IPT_UP, IPT_DOWN, IPT_LEFT, IPT_RIGHT,

	IPT_0, IPT_1, IPT_2, IPT_3, IPT_4,
	IPT_5, IPT_6, IPT_7, IPT_8, IPT_9, /* same as '0'-'9' */

	IPT_INSERT, IPT_DELETE, IPT_HOME, IPT_END, IPT_PAGEUP, IPT_PAGEDOWN,
	IPT_MENU,

	IPT_A, IPT_B, IPT_C, IPT_D, IPT_E, IPT_F, IPT_G, IPT_H, IPT_I, IPT_J,
	IPT_K, IPT_L, IPT_M, IPT_N, IPT_O, IPT_P, IPT_Q, IPT_R, IPT_S, IPT_T,
	IPT_U, IPT_V, IPT_W, IPT_X, IPT_Y, IPT_Z, /* same as 'A'-'Z' */

	IPT_ENTER, IPT_ESCAPE, IPT_SPACE, IPT_BACKSPACE, IPT_TAB, IPT_CAPSLOCK,
	IPT_SCROLLLOCK, IPT_PRINTSCREEN, IPT_PAUSE, IPT_NUMLOCK,

	IPT_KP0, IPT_KP1, IPT_KP2, IPT_KP3, IPT_KP4,
	IPT_KP5, IPT_KP6, IPT_KP7, IPT_KP8, IPT_KP9,
	IPT_KP_DIVIDE, IPT_KP_MULTIPLY, IPT_KP_MINUS,
	IPT_KP_PLUS, IPT_KP_DECIMAL, IPT_KP_ENTER,

	/* NOTE: RMOUSE must be before MMOUSE for PlayerClick compatibility */
	IPT_XBUTTON1, IPT_XBUTTON2, IPT_LMOUSE, IPT_RMOUSE, IPT_MMOUSE,
	INPUT_COUNT,

	INPUT_CLIPBOARD_COPY  = 1001,
	INPUT_CLIPBOARD_PASTE = 1002
};

/* Names for each input button when stored to disc */
extern const char* const Input_StorageNames[INPUT_COUNT];
/* Simple display names for each input button */
extern const char* const Input_DisplayNames[INPUT_COUNT];

#define Input_IsWinPressed()   (Input_Pressed[IPT_LWIN]   || Input_Pressed[IPT_RWIN])
#define Input_IsAltPressed()   (Input_Pressed[IPT_LALT]   || Input_Pressed[IPT_RALT])
#define Input_IsCtrlPressed()  (Input_Pressed[IPT_LCTRL]  || Input_Pressed[IPT_RCTRL])
#define Input_IsShiftPressed() (Input_Pressed[IPT_LSHIFT] || Input_Pressed[IPT_RSHIFT])

#if defined CC_BUILD_HAIKU
/* Haiku uses ALT instead of CTRL for clipboard and stuff */
#define Input_IsActionPressed() Input_IsAltPressed()
#elif defined CC_BUILD_DARWIN
/* macOS uses CMD instead of CTRL for clipboard and stuff */
#define Input_IsActionPressed() Input_IsWinPressed()
#else
#define Input_IsActionPressed() Input_IsCtrlPressed()
#endif

/* Pressed state of each input button. Use Input_Set to change. */
extern cc_bool Input_Pressed[INPUT_COUNT];
/* Sets Input_Pressed[key] to true and raises InputEvents.Down */
void Input_SetPressed(int key);
/* Sets Input_Pressed[key] to false and raises InputEvents.Up */
void Input_SetReleased(int key);
/* Calls either Input_SetPressed or Input_SetReleased */
void Input_Set(int key, int pressed);
void Input_SetNonRepeatable(int key, int pressed);
/* Resets all input buttons to released state. (Input_SetReleased) */
void Input_Clear(void);

/* Whether raw mouse/touch input is currently being listened for. */
extern cc_bool Input_RawMode;

#ifdef CC_BUILD_TOUCH
#define INPUT_MAX_POINTERS 32
enum INPUT_MODE { INPUT_MODE_PLACE, INPUT_MODE_DELETE, INPUT_MODE_NONE, INPUT_MODE_COUNT };

extern int Pointers_Count;
extern int Input_TapMode, Input_HoldMode;
/* Whether touch input is being used. */
extern cc_bool Input_TouchMode;
void Input_SetTouchMode(cc_bool enabled);

void Input_AddTouch(long id,    int x, int y);
void Input_UpdateTouch(long id, int x, int y);
void Input_RemoveTouch(long id, int x, int y);
#else
#define INPUT_MAX_POINTERS 1
#define Pointers_Count 1
#define Input_TouchMode false
#endif

/* Touch fingers are initially are 'all' type, meaning they could */
/*  trigger menu clicks, camera movement, or place/delete blocks  */
/* But for example, after clicking on a menu button, you wouldn't */
/*  want moving that finger anymore to move the camera */
#define TOUCH_TYPE_GUI    1
#define TOUCH_TYPE_CAMERA 2
#define TOUCH_TYPE_BLOCKS 4
#define TOUCH_TYPE_ALL (TOUCH_TYPE_GUI | TOUCH_TYPE_CAMERA | TOUCH_TYPE_BLOCKS)

/* Data for mouse and touch */
extern struct Pointer { int x, y; } Pointers[INPUT_MAX_POINTERS];
/* Raises InputEvents.Wheel with the given wheel delta. */
void Mouse_ScrollWheel(float delta);
/* Sets X and Y position of the given pointer, always raising PointerEvents.Moved. */
void Pointer_SetPosition(int idx, int x, int y);


/* Enumeration of all key bindings. */
enum KeyBind_ {
	KEYBIND_FORWARD, KEYBIND_BACK, KEYBIND_LEFT, KEYBIND_RIGHT,
	KEYBIND_JUMP, KEYBIND_RESPAWN, KEYBIND_SET_SPAWN, KEYBIND_CHAT,
	KEYBIND_INVENTORY, KEYBIND_FOG, KEYBIND_SEND_CHAT, KEYBIND_TABLIST,
	KEYBIND_SPEED, KEYBIND_NOCLIP, KEYBIND_FLY, KEYBIND_FLY_UP, KEYBIND_FLY_DOWN,
	KEYBIND_EXT_INPUT, KEYBIND_HIDE_FPS, KEYBIND_SCREENSHOT, KEYBIND_FULLSCREEN,
	KEYBIND_THIRD_PERSON, KEYBIND_HIDE_GUI, KEYBIND_AXIS_LINES, KEYBIND_ZOOM_SCROLL,
	KEYBIND_HALF_SPEED, KEYBIND_DELETE_BLOCK, KEYBIND_PICK_BLOCK, KEYBIND_PLACE_BLOCK,
	KEYBIND_AUTOROTATE, KEYBIND_HOTBAR_SWITCH, KEYBIND_SMOOTH_CAMERA,
	KEYBIND_DROP_BLOCK, KEYBIND_IDOVERLAY, KEYBIND_BREAK_LIQUIDS,
	KEYBIND_LOOK_UP, KEYBIND_LOOK_DOWN, KEYBIND_LOOK_RIGHT, KEYBIND_LOOK_LEFT,
	KEYBIND_COUNT
};
typedef int KeyBind;

/* The keys that are bound to each key binding. */
extern cc_uint8 KeyBinds[KEYBIND_COUNT];
/* Default key that each key binding is bound to */
extern const cc_uint8 KeyBind_Defaults[KEYBIND_COUNT];

/* Gets whether the key bound to the given key binding is pressed. */
CC_API cc_bool KeyBind_IsPressed(KeyBind binding);
/* Set the key that the given key binding is bound to. (also updates options list) */
void KeyBind_Set(KeyBind binding, int key);

/* whether to leave text input open for user to enter further input */
#define HOTKEY_FLAG_STAYS_OPEN   0x01
/* Whether the hotkey was auto defined (e.g. by server) */
#define HOTKEY_FLAG_AUTO_DEFINED 0x02

extern const cc_uint8 Hotkeys_LWJGL[256];
struct HotkeyData {
	int textIndex;     /* contents to copy directly into the input bar */
	cc_uint8 trigger;  /* Member of Key enumeration */
	cc_uint8 mods;     /* HotkeyModifiers bitflags */
	cc_uint8 flags;    /* HOTKEY_FLAG flags */
};

#define HOTKEYS_MAX_COUNT 256
extern struct HotkeyData HotkeysList[HOTKEYS_MAX_COUNT];
extern struct StringsBuffer HotkeysText;
enum HotkeyModifiers {
	HOTKEY_MOD_CTRL = 1, HOTKEY_MOD_SHIFT = 2, HOTKEY_MOD_ALT = 4
};

/* Adds or updates a new hotkey. */
void Hotkeys_Add(int trigger, cc_uint8 modifiers, const cc_string* text, cc_uint8 flags);
/* Removes the given hotkey. */
cc_bool Hotkeys_Remove(int trigger, cc_uint8 modifiers);
/* Returns the first hotkey which is bound to the given key and has its modifiers pressed. */
/* NOTE: The hotkeys list is sorted, so hotkeys with most modifiers are checked first. */
int Hotkeys_FindPartial(int key);

/* Loads the given hotkey from options. (if it exists) */
void StoredHotkeys_Load(int trigger, cc_uint8 modifiers);
/* Removes the given hotkey from options. */
void StoredHotkeys_Remove(int trigger, cc_uint8 modifiers);
/* Adds the given hotkey from options. */
void StoredHotkeys_Add(int trigger, cc_uint8 modifiers, cc_bool moreInput, const cc_string* text);

cc_bool InputHandler_SetFOV(int fov);
cc_bool Input_HandleMouseWheel(float delta);
void InputHandler_Tick(void);
void InputHandler_OnScreensChanged(void);
void InputHandler_DeleteBlock(void);
void InputHandler_PlaceBlock(void);
void InputHandler_PickBlock(void);

/* Enumeration of on-screen buttons for touch GUI */
#define ONSCREEN_BTN_CHAT      (1 << 0)
#define ONSCREEN_BTN_LIST      (1 << 1)
#define ONSCREEN_BTN_SPAWN     (1 << 2)
#define ONSCREEN_BTN_SETSPAWN  (1 << 3)
#define ONSCREEN_BTN_FLY       (1 << 4)
#define ONSCREEN_BTN_NOCLIP    (1 << 5)
#define ONSCREEN_BTN_SPEED     (1 << 6)
#define ONSCREEN_BTN_HALFSPEED (1 << 7)
#define ONSCREEN_BTN_CAMERA    (1 << 8)
#define ONSCREEN_BTN_DELETE    (1 << 9)
#define ONSCREEN_BTN_PICK      (1 << 10)
#define ONSCREEN_BTN_PLACE     (1 << 11)
#define ONSCREEN_BTN_SWITCH    (1 << 12)
#define ONSCREEN_MAX_BTNS 13
#endif
