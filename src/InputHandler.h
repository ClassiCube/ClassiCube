#ifndef CC_INPUTHANDLER_H
#define CC_INPUTHANDLER_H
#include "Core.h"
/* 
Manages base game input handling
Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct StringsBuffer;
struct InputDevice;
extern struct IGameComponent InputHandler_Component;


/* Enumeration of all input bindings. */
enum InputBind_ {
	BIND_FORWARD, BIND_BACK, BIND_LEFT, BIND_RIGHT,
	BIND_JUMP, BIND_RESPAWN, BIND_SET_SPAWN, BIND_CHAT,
	BIND_INVENTORY, BIND_FOG, BIND_SEND_CHAT, BIND_TABLIST,
	BIND_SPEED, BIND_NOCLIP, BIND_FLY, BIND_FLY_UP, BIND_FLY_DOWN,
	BIND_EXT_INPUT, BIND_HIDE_FPS, BIND_SCREENSHOT, BIND_FULLSCREEN,
	BIND_THIRD_PERSON, BIND_HIDE_GUI, BIND_AXIS_LINES, BIND_ZOOM_SCROLL,
	BIND_HALF_SPEED, BIND_DELETE_BLOCK, BIND_PICK_BLOCK, BIND_PLACE_BLOCK,
	BIND_AUTOROTATE, BIND_HOTBAR_SWITCH, BIND_SMOOTH_CAMERA,
	BIND_DROP_BLOCK, BIND_IDOVERLAY, BIND_BREAK_LIQUIDS,
	BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_RIGHT, BIND_LOOK_LEFT,
	BIND_HOTBAR_1, BIND_HOTBAR_2, BIND_HOTBAR_3,
	BIND_HOTBAR_4, BIND_HOTBAR_5, BIND_HOTBAR_6,
	BIND_HOTBAR_7, BIND_HOTBAR_8, BIND_HOTBAR_9,
	BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT,
	BIND_COUNT
};
typedef int InputBind;
typedef struct BindMapping_ { cc_uint8 button1, button2; } BindMapping;
typedef cc_bool (*BindTriggered)(int key, struct InputDevice* device);
typedef void    (*BindReleased)(int key, struct InputDevice* device);
#define BindMapping_Set(mapping, btn1, btn2) (mapping)->button1 = btn1; (mapping)->button2 = btn2;

/* The keyboard/mouse buttons that are bound to each input binding */
extern BindMapping KeyBind_Mappings[BIND_COUNT];
/* The gamepad buttons that are bound to each input binding */
extern BindMapping PadBind_Mappings[BIND_COUNT];
/* Default keyboard/mouse button that each input binding is bound to */
extern const BindMapping KeyBind_Defaults[BIND_COUNT];
/* Default gamepad button that each input binding is bound to */
extern const BindMapping PadBind_Defaults[BIND_COUNT];
/* Callback behaviour for when the given input binding is triggered */
extern BindTriggered Bind_OnTriggered[BIND_COUNT];
/* Callback behaviour for when the given input binding is released */
extern BindReleased  Bind_OnReleased[BIND_COUNT];
/* Whether the given input binding is activated by one or more devices */
extern cc_uint8 Bind_IsTriggered[BIND_COUNT];

/* Whether the given binding should be triggered in response to given input button being pressed */
cc_bool InputBind_Claims(InputBind binding, int btn, struct InputDevice* device);
/* Gets whether the given input binding is currently being triggered */
CC_API cc_bool KeyBind_IsPressed(InputBind binding);

/* Sets the key/mouse button that the given input binding is bound to */
void KeyBind_Set(InputBind binding, int btn);
/* Sets the gamepad button that the given input binding is bound to */
void PadBind_Set(InputBind binding, int btn);
/* Resets the key/mouse button that the given input binding is bound to */
void KeyBind_Reset(InputBind binding);
/* Resets the gamepad button that the given input binding is bound to */
void PadBind_Reset(InputBind binding);


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
#endif
