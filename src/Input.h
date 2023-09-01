#ifndef CC_INPUT_H
#define CC_INPUT_H
#include "Core.h"
/* 
Manages input state, raising input related events, and base input handling
Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct StringsBuffer;
extern struct IGameComponent Input_Component;

enum InputButtons {
	INPUT_NONE, /* Unrecognised key */

	CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F8, CCKEY_F9, CCKEY_F10,
	CCKEY_F11, CCKEY_F12, CCKEY_F13, CCKEY_F14, CCKEY_F15, CCKEY_F16, CCKEY_F17, CCKEY_F18, CCKEY_F19, CCKEY_F20,
	CCKEY_F21, CCKEY_F22, CCKEY_F23, CCKEY_F24,

	CCKEY_TILDE, CCKEY_MINUS, CCKEY_EQUALS, CCKEY_LBRACKET, CCKEY_RBRACKET, CCKEY_SLASH,
	CCKEY_SEMICOLON, CCKEY_QUOTE, CCKEY_COMMA, CCKEY_PERIOD, CCKEY_BACKSLASH,

	CCKEY_LSHIFT, CCKEY_RSHIFT, CCKEY_LCTRL, CCKEY_RCTRL,
	CCKEY_LALT, CCKEY_RALT, CCKEY_LWIN, CCKEY_RWIN,

	CCKEY_UP, CCKEY_DOWN, CCKEY_LEFT, CCKEY_RIGHT,

	CCKEY_0, CCKEY_1, CCKEY_2, CCKEY_3, CCKEY_4,
	CCKEY_5, CCKEY_6, CCKEY_7, CCKEY_8, CCKEY_9, /* same as '0'-'9' */

	CCKEY_INSERT, CCKEY_DELETE, CCKEY_HOME, CCKEY_END, CCKEY_PAGEUP, CCKEY_PAGEDOWN,
	CCKEY_MENU,

	CCKEY_A, CCKEY_B, CCKEY_C, CCKEY_D, CCKEY_E, CCKEY_F, CCKEY_G, CCKEY_H, CCKEY_I, CCKEY_J,
	CCKEY_K, CCKEY_L, CCKEY_M, CCKEY_N, CCKEY_O, CCKEY_P, CCKEY_Q, CCKEY_R, CCKEY_S, CCKEY_T,
	CCKEY_U, CCKEY_V, CCKEY_W, CCKEY_X, CCKEY_Y, CCKEY_Z, /* same as 'A'-'Z' */

	CCKEY_ENTER, CCKEY_ESCAPE, CCKEY_SPACE, CCKEY_BACKSPACE, CCKEY_TAB, CCKEY_CAPSLOCK,
	CCKEY_SCROLLLOCK, CCKEY_PRINTSCREEN, CCKEY_PAUSE, CCKEY_NUMLOCK,

	CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4,
	CCKEY_KP5, CCKEY_KP6, CCKEY_KP7, CCKEY_KP8, CCKEY_KP9,
	CCKEY_KP_DIVIDE, CCKEY_KP_MULTIPLY, CCKEY_KP_MINUS,
	CCKEY_KP_PLUS, CCKEY_KP_DECIMAL, CCKEY_KP_ENTER,

	/* NOTE: RMOUSE must be before MMOUSE for PlayerClick compatibility */
	CCMOUSE_X1, CCMOUSE_X2, CCMOUSE_L, CCMOUSE_R, CCMOUSE_M,

	CCPAD_A, CCPAD_B, CCPAD_X, CCPAD_Y, CCPAD_L, CCPAD_R,
	CCPAD_LEFT, CCPAD_RIGHT, CCPAD_UP, CCPAD_DOWN,
	CCPAD_START, CCPAD_SELECT, CCPAD_ZL, CCPAD_ZR,

	INPUT_COUNT,

	INPUT_CLIPBOARD_COPY  = 1001,
	INPUT_CLIPBOARD_PASTE = 1002
};

extern const char* const Input_DisplayNames[INPUT_COUNT];

extern struct _InputState {
	/* Pressed state of each input button. Use Input_Set to change */
	cc_bool Pressed[INPUT_COUNT];
	/* Whether raw mouse/touch input is currently being listened for */
	cc_bool RawMode;
	/* Sources available for input (Mouse/Keyboard, Gamepad) */
	cc_uint8 Sources;
	/* Whether a gamepad joystick is being used to control player movement */
	cc_bool JoystickMovement;
	/* Angle of the gamepad joystick being used to control player movement */
	float JoystickAngle;
} Input;

#define INPUT_SOURCE_NORMAL  (1 << 0)
#define INPUT_SOURCE_GAMEPAD (1 << 1)

/* Sets Input_Pressed[key] to true and raises InputEvents.Down */
void Input_SetPressed(int key);
/* Sets Input_Pressed[key] to false and raises InputEvents.Up */
void Input_SetReleased(int key);
/* Calls either Input_SetPressed or Input_SetReleased */
void Input_Set(int key, int pressed);
void Input_SetNonRepeatable(int key, int pressed);
/* Resets all input buttons to released state. (Input_SetReleased) */
void Input_Clear(void);


#define Input_IsWinPressed()   (Input.Pressed[CCKEY_LWIN]   || Input.Pressed[CCKEY_RWIN])
#define Input_IsAltPressed()   (Input.Pressed[CCKEY_LALT]   || Input.Pressed[CCKEY_RALT])
#define Input_IsCtrlPressed()  (Input.Pressed[CCKEY_LCTRL]  || Input.Pressed[CCKEY_RCTRL])
#define Input_IsShiftPressed() (Input.Pressed[CCKEY_LSHIFT] || Input.Pressed[CCKEY_RSHIFT])

#define Input_IsUpButton(btn)     ((btn) == CCKEY_UP     || (btn) == CCPAD_UP)
#define Input_IsDownButton(btn)   ((btn) == CCKEY_DOWN   || (btn) == CCPAD_DOWN)
#define Input_IsLeftButton(btn)   ((btn) == CCKEY_LEFT   || (btn) == CCPAD_LEFT)
#define Input_IsRightButton(btn)  ((btn) == CCKEY_RIGHT  || (btn) == CCPAD_RIGHT)

#define Input_IsEnterButton(btn)  ((btn) == CCKEY_ENTER  || (btn) == CCPAD_START || (btn) == CCKEY_KP_ENTER)
#define Input_IsPauseButton(btn)  ((btn) == CCKEY_ESCAPE || (btn) == CCPAD_START || (btn) == CCKEY_PAUSE)
#define Input_IsEscapeButton(btn) ((btn) == CCKEY_ESCAPE || (btn) == CCPAD_SELECT)

#if defined CC_BUILD_HAIKU
/* Haiku uses ALT instead of CTRL for clipboard and stuff */
#define Input_IsActionPressed() Input_IsAltPressed()
#elif defined CC_BUILD_DARWIN
/* macOS uses CMD instead of CTRL for clipboard and stuff */
#define Input_IsActionPressed() Input_IsWinPressed()
#else
#define Input_IsActionPressed() Input_IsCtrlPressed()
#endif

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
	KEYBIND_HOTBAR_1, KEYBIND_HOTBAR_2, KEYBIND_HOTBAR_3,
	KEYBIND_HOTBAR_4, KEYBIND_HOTBAR_5, KEYBIND_HOTBAR_6,
	KEYBIND_HOTBAR_7, KEYBIND_HOTBAR_8, KEYBIND_HOTBAR_9,
	KEYBIND_HOTBAR_LEFT, KEYBIND_HOTBAR_RIGHT,
	KEYBIND_COUNT
};
typedef int KeyBind;

/* The keyboard/mouse buttons that are bound to each key binding */
extern cc_uint8 KeyBinds_Normal[KEYBIND_COUNT];
/* The gamepad buttons that are bound to each key binding */
extern cc_uint8 KeyBinds_Gamepad[KEYBIND_COUNT];
/* Default keyboard/mouse button that each key binding is bound to */
extern const cc_uint8 KeyBind_NormalDefaults[KEYBIND_COUNT];
/* Default gamepad button that each key binding is bound to */
extern const cc_uint8 KeyBind_GamepadDefaults[KEYBIND_COUNT];
#define KeyBind_GetDefaults() (Input.GamepadSource ? KeyBind_GamepadDefaults : KeyBind_NormalDefaults)

/* Whether the given keyboard/mouse or gamepad button is bound to the given keybinding */
#define KeyBind_Claims(binding, btn) (KeyBinds_Normal[binding] == (btn) || KeyBinds_Gamepad[binding] == (btn))
/* Gets whether the key bound to the given key binding is pressed. */
CC_API cc_bool KeyBind_IsPressed(KeyBind binding);
/* Set the key that the given key binding is bound to. (also updates options list) */
void KeyBind_Set(KeyBind binding, int key, cc_uint8* binds);

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
