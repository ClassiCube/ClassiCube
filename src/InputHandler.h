#ifndef CC_INPUTHANDLER_H
#define CC_INPUTHANDLER_H
#include "Input.h"
CC_BEGIN_HEADER

/* 
Manages base game input handling
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct StringsBuffer;
struct InputDevice;
extern struct IGameComponent InputHandler_Component;


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


typedef cc_bool (*BindTriggered)(int key, struct InputDevice* device);
typedef void    (*BindReleased)(int key, struct InputDevice* device);
/* Gets whether the given input binding is currently being triggered */
CC_API cc_bool KeyBind_IsPressed(InputBind binding);

/* Callback behaviour for when the given input binding is triggered */
extern BindTriggered Bind_OnTriggered[BIND_COUNT];
/* Callback behaviour for when the given input binding is released */
extern BindReleased  Bind_OnReleased[BIND_COUNT];
/* Whether the given input binding is activated by one or more devices */
extern cc_uint8 Bind_IsTriggered[BIND_COUNT];

CC_END_HEADER
#endif
