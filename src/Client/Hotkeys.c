#include "Hotkeys.h"
#include "Options.h"
#include "Constants.h"
#include "Utils.h"
#include "Funcs.h"
#include "ErrorHandler.h"

UInt8 Hotkeys_LWJGL[256] = {
	0, Key_Escape, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9, Key_0, Key_Minus, Key_Plus, Key_BackSpace, Key_Tab,
	Key_Q, Key_W, Key_E, Key_R, Key_T, Key_Y, Key_U, Key_I, Key_O, Key_P, Key_BracketLeft, Key_BracketRight, Key_Enter, Key_ControlLeft, Key_A, Key_S,
	Key_D, Key_F, Key_G, Key_H, Key_J, Key_K, Key_L, Key_Semicolon, Key_Quote, Key_Tilde, Key_ShiftLeft, Key_BackSlash, Key_Z, Key_X, Key_C, Key_V,
	Key_B, Key_N, Key_M, Key_Comma, Key_Period, Key_Slash, Key_ShiftRight, 0, Key_AltLeft, Key_Space, Key_CapsLock, Key_F1, Key_F2, Key_F3, Key_F4, Key_F5,
	Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_NumLock, Key_ScrollLock, Key_Keypad7, Key_Keypad8, Key_Keypad9, Key_KeypadSubtract, Key_Keypad4, Key_Keypad5, Key_Keypad6, Key_KeypadAdd, Key_Keypad1,
	Key_Keypad2, Key_Keypad3, Key_Keypad0, Key_KeypadDecimal, 0, 0, 0, Key_F11, Key_F12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, Key_F13, Key_F14, Key_F15, Key_F16, Key_F17, Key_F18, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Key_KeypadAdd, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Key_KeypadEnter, Key_ControlRight, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, Key_KeypadDivide, 0, 0, Key_AltRight, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, Key_Pause, 0, Key_Home, Key_Up, Key_PageUp, 0, Key_Left, 0, Key_Right, 0, Key_End,
	Key_Down, Key_PageDown, Key_Insert, Key_Delete, 0, 0, 0, 0, 0, 0, 0, Key_WinLeft, Key_WinRight, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void Hotkeys_QuickSort(Int32 left, Int32 right) {
	HotkeyData* keys = HotkeysList; HotkeyData key;

	while (left < right) {
		Int32 i = left, j = right;
		UInt8 pivot = keys[(i + j) / 2].Flags;

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i].Flags) i++;
			while (pivot < keys[j].Flags) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Hotkeys_QuickSort)
	}
}

void Hotkeys_AddNewHotkey(Key baseKey, UInt8 flags, STRING_PURE String* text, bool more) {
	HotkeyData hKey;
	hKey.BaseKey = baseKey;
	hKey.Flags = flags;
	hKey.TextIndex = HotkeysText.Count;
	hKey.StaysOpen = more;

	if (HotkeysText.Count == HOTKEYS_MAX_COUNT) {
		ErrorHandler_Fail("Cannot define more than 256 hotkeys");
	}

	HotkeysList[HotkeysText.Count] = hKey;
	StringsBuffer_Add(&HotkeysText, text);
	/* sort so that hotkeys with largest modifiers are first */
	Hotkeys_QuickSort(0, HotkeysText.Count - 1);
}

void Hotkeys_Add(Key baseKey, UInt8 flags, STRING_PURE String* text, bool more) {
	UInt32 i;
	for (i = 0; i < HotkeysText.Count; i++) {
		HotkeyData hKey = HotkeysList[i];
		if (hKey.BaseKey == baseKey && hKey.Flags == flags) {
			StringsBuffer_Remove(&HotkeysText, hKey.TextIndex);
			HotkeysList[i].StaysOpen = more;
			HotkeysList[i].TextIndex = HotkeysText.Count;
			StringsBuffer_Add(&HotkeysText, text);
			return;
		}
	}
	Hotkeys_AddNewHotkey(baseKey, flags, text, more);
}

bool Hotkeys_Remove(Key baseKey, UInt8 flags) {
	UInt32 i, j;
	for (i = 0; i < HotkeysText.Count; i++) {
		HotkeyData hKey = HotkeysList[i];
		if (hKey.BaseKey == baseKey && hKey.Flags == flags) {
			for (j = i + 1; j < HotkeysText.Count; j++) { HotkeysList[j - 1] = HotkeysList[j]; }
			StringsBuffer_Remove(&HotkeysText, hKey.TextIndex);
			HotkeysList[i].TextIndex = UInt32_MaxValue;
			return true;
		}
	}
	return false;
}

bool Hotkeys_IsHotkey(Key key, STRING_TRANSIENT String* text, bool* moreInput) {
	UInt8 flags = 0;
	if (Key_IsControlPressed()) flags |= 1;
	if (Key_IsShiftPressed())   flags |= 2;
	if (Key_IsAltPressed())     flags |= 4;

	String_Clear(text);
	*moreInput = false;

	UInt32 i;
	for (i = 0; i < HotkeysText.Count; i++) {
		HotkeyData hKey = HotkeysList[i];
		if ((hKey.Flags & flags) == hKey.Flags && hKey.BaseKey == key) {
			String hkeyText = StringsBuffer_UNSAFE_Get(&HotkeysText, hKey.TextIndex);
			String_AppendString(text, &hkeyText);
			*moreInput = hKey.StaysOpen;
			return true;
		}
	}
	return false;
}

void Hotkeys_Init(void) {
	StringBuffers_Init(&HotkeysText);
	String prefix = String_FromConst("hotkey-");
	UInt32 i;
	for (i = 0; i < Options_Keys.Count; i++) {
		String key = StringsBuffer_UNSAFE_Get(&Options_Keys, i);
		if (!String_CaselessStarts(&key, &prefix)) continue;
		Int32 keySplit = String_IndexOf(&key, '&', prefix.length);
		if (keySplit < 0) continue; /* invalid key */

		String strKey   = String_UNSAFE_Substring(&key, prefix.length, keySplit - prefix.length);
		String strFlags = String_UNSAFE_SubstringAt(&key, keySplit + 1);

		String value = StringsBuffer_UNSAFE_Get(&Options_Values, i);
		Int32 valueSplit = String_IndexOf(&value, '&', 0);
		if (valueSplit < 0) continue; /* invalid value */

		String strMoreInput = String_UNSAFE_Substring(&value, 0, valueSplit - 0);
		String strText      = String_UNSAFE_SubstringAt(&value, valueSplit + 1);

		/* Then try to parse the key and value */
		Key hotkey = Utils_ParseEnum(&strKey, Key_Unknown, Key_Names, Array_NumElements(Key_Names));
		UInt8 flags; bool moreInput;
		if (hotkey == Key_Unknown || strText.length == 0 || !Convert_TryParseUInt8(&strFlags, &flags) 
			|| !Convert_TryParseBool(&strMoreInput, &moreInput)) { continue; }

		Hotkeys_Add(hotkey, flags, &strText, moreInput);
	}
}

void Hotkeys_UserRemovedHotkey(Key baseKey, UInt8 flags) {
	UInt8 keyBuffer[String_BufferSize(STRING_SIZE)];
	String key = String_InitAndClearArray(keyBuffer);

	String_AppendConst(&key, "hotkey-"); String_AppendConst(&key, Key_Names[baseKey]);
	String_Append(&key, '&');            String_AppendInt32(&key, flags);
	Options_Set(key.buffer, NULL);
}

void Hotkeys_UserAddedHotkey(Key baseKey, UInt8 flags, bool moreInput, STRING_PURE String* text) {
	UInt8 keyBuffer[String_BufferSize(STRING_SIZE)];
	String key = String_InitAndClearArray(keyBuffer);
	UInt8 valueBuffer[String_BufferSize(STRING_SIZE * 2)];
	String value = String_InitAndClearArray(valueBuffer);

	String_AppendConst(&key, "hotkey-"); String_AppendConst(&key, Key_Names[baseKey]);
	String_Append(&key, '&');            String_AppendInt32(&key, flags);

	String_AppendBool(&value, moreInput); String_Append(&value, '&');
	String_AppendString(&value, text);
	Options_Set(key.buffer, &value);
}