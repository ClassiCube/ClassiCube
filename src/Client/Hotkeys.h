#ifndef CC_HOTKEYS_H
#define CC_HOTKEYS_H
#include "Typedefs.h"
#include "Input.h"
#include "String.h"
/* Maintains list of hotkeys defined by the client and SetTextHotkey packets.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


extern UInt8 Hotkeys_LWJGL[256];
typedef struct HotkeyData_ {
	UInt32 TextIndex; /* contents to copy directly into the input bar */
	UInt8 BaseKey;    /* Member of Key enumeration */
	UInt8 Flags;      /* ctrl 1, shift 2, alt 4 */
	bool StaysOpen;   /* whether the user is able to enter further input */
} HotkeyData;

#define HOTKEYS_MAX_COUNT 256
HotkeyData HotkeysList[HOTKEYS_MAX_COUNT];
StringsBuffer HotkeysText;
#define HOTKEYS_FLAG_CTRL  1
#define HOTKEYS_FLAG_SHIFT 2
#define HOTKEYS_FLAT_ALT   4

void Hotkeys_Add(Key baseKey, UInt8 flags, STRING_PURE String* text, bool more);
bool Hotkeys_Remove(Key baseKey, UInt8 flags);
bool Hotkeys_IsHotkey(Key key, STRING_TRANSIENT String* text, bool* moreInput);
void Hotkeys_Init(void);
void Hotkeys_UserRemovedHotkey(Key baseKey, UInt8 flags);
void Hotkeys_UserAddedHotkey(Key baseKey, UInt8 flags, bool moreInput, STRING_PURE String* text);
#endif