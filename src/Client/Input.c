#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Utils.h"
#include "ErrorHandler.h"

#define Key_Function_Names \
"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",  "F10",\
"F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",\
"F21", "F22", "F23", "F24", "F25", "F26", "F27", "F28", "F29", "F30",\
"F31", "F32", "F33", "F34", "F35"
#define Key_Ascii_Names \
"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",\
"K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",\
"U", "V", "W", "X", "Y", "Z"

const UInt8* Key_Names[Key_Count] = {
	"Unknown",
	"ShiftLeft", "ShiftRight", "ControlLeft", "ControlRight",
	"AltLeft", "AltRight", "WinLeft", "WinRight", "Menu",
	Key_Function_Names,
	"Up", "Down", "Left", "Right",
	"Enter", "Escape", "Space", "Tab", "BackSpace", "Insert",
	"Delete", "PageUp", "PageDown", "Home", "End", "CapsLock",
	"ScrollLock", "PrintScreen", "Pause", "NumLock",
	"Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4",
	"Keypad5", "Keypad6", "Keypad7", "Keypad8", "Keypad9",
	"KeypadDivide", "KeypadMultiply", "KeypadSubtract",
	"KeypadAdd", "KeypadDecimal", "KeypadEnter",
	Key_Ascii_Names,
	"Number0", "Number1", "Number2", "Number3", "Number4",
	"Number5", "Number6", "Number7", "Number8", "Number9",
	"Tilde", "Minus", "Plus", "BracketLeft", "BracketRight",
	"Semicolon", "Quote", "Comma", "Period", "Slash", "BackSlash",
	"XButton1", "XButton2",
};

/* TODO: Should this only be shown in GUI? not saved to disc? */
/*const UInt8* Key_Names[Key_Count] = {
	"NONE",
	"LSHIFT", "RSHIFT", "LCONTROL", "RCONTROL",
	"LMENU", "RMENU", "LWIN", "RWIN", "MENU",
	Key_Function_Names,
	"UP", "DOWN", "LEFT", "RIGHT",
	"RETURN", "ESCAPE", "SPACE", "TAB", "BACK", "INSERT",
	"DELETE", "PRIOR", "DOWN", "HOME", "END", "CAPITAL",
	"SCROLL", "PRINT", "PAUSE", "NUMLOCK",
	"NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3", "NUMPAD4",
	"NUMPAD5", "NUMPAD6", "NUMPAD7", "NUMPAD8", "NUMPAD9",
	"DIVIDE", "MULTIPLY", "SUBTRACT",
	"ADD", "DECIMAL", "NUMPADENTER",
	Key_Ascii_Names,
	"0", "1", "2", "3", "4",
	"5", "6", "7", "8", "9",
	"GRAVE", "MINUS", "PLUS", "LBRACKET", "RBRACKET",
	"SEMICOLON", "APOSTROPHE", "COMMA", "PERIOD", "SLASH", "BACKSLASH",
	"XBUTTON1", "XBUTTON2",
};*/

bool Key_States[Key_Count];
bool Key_IsPressed(Key key) { return Key_States[key]; }

void Key_SetPressed(Key key, bool pressed) {
	if (Key_States[key] != pressed || Key_KeyRepeat) {
		Key_States[key] = pressed;

		if (pressed) {
			Event_RaiseInt32(&KeyEvents_Down, key);
		} else {
			Event_RaiseInt32(&KeyEvents_Up, key);
		}
	}
}

void Key_Clear(void) {
	Int32 i;
	for (i = 0; i < Key_Count; i++) {
		if (Key_States[i]) Key_SetPressed((Key)i, false);
	}
}


bool MouseButton_States[MouseButton_Count];
bool Mouse_IsPressed(MouseButton btn) { return MouseButton_States[btn]; }

void Mouse_SetPressed(MouseButton btn, bool pressed) {
	if (MouseButton_States[btn] != pressed) {
		MouseButton_States[btn] = pressed;

		if (pressed) {
			Event_RaiseInt32(&MouseEvents_Down, btn);
		} else {
			Event_RaiseInt32(&MouseEvents_Up, btn);
		}
	}
}

void Mouse_SetWheel(Real32 wheel) {
	Real32 delta = wheel - Mouse_Wheel;
	Mouse_Wheel = wheel;
	Event_RaiseReal32(&MouseEvents_Wheel, delta);
}

void Mouse_SetPosition(Int32 x, Int32 y) {
	Int32 deltaX = x - Mouse_X, deltaY = y - Mouse_Y;
	Mouse_X = x; Mouse_Y = y;
	Event_RaiseMouseMove(&MouseEvents_Moved, deltaX, deltaY);
}

Key KeyBind_Keys[KeyBind_Count];
Key KeyBind_Defaults[KeyBind_Count] = {
	Key_W, Key_S, Key_A, Key_D,
	Key_Space, Key_R, Key_Enter, Key_T,
	Key_B, Key_F, Key_Enter, Key_Escape,
	Key_Tab, Key_ShiftLeft, Key_X, Key_Z,
	Key_Q, Key_E, Key_AltLeft, Key_F3,
	Key_F12, Key_F11, Key_F5, Key_F1,
	Key_F7, Key_C, Key_ControlLeft, Key_Unknown,
	Key_Unknown, Key_Unknown, Key_F6, Key_AltLeft, 
	Key_F8, Key_G, Key_F10,
};
const UInt8* KeyBind_Names[KeyBind_Count] = {
	"Forward", "Back", "Left", "Right",
	"Jump", "Respawn", "SetSpawn", "Chat",
	"Inventory", "ToggleFog", "SendChat", "PauseOrExit",
	"PlayerList", "Speed", "NoClip", "Fly",
	"FlyUp", "FlyDown", "ExtInput", "HideFPS",
	"Screenshot", "Fullscreen", "ThirdPerson", "HideGUI",
	"AxisLines", "ZoomScrolling", "HalfSpeed", "MouseLeft",
	"MouseMiddle", "MouseRight", "AutoRotate", "HotbarSwitching",
	"SmoothCamera", "DropBlock", "IDOverlay",
};

Key KeyBind_Get(KeyBind binding) { return KeyBind_Keys[binding]; }
Key KeyBind_GetDefault(KeyBind binding) { return KeyBind_Defaults[binding]; }
bool KeyBind_IsPressed(KeyBind binding) { return Key_States[KeyBind_Keys[binding]]; }

#define KeyBind_MakeName(name) String_Clear(&name); String_AppendConst(&name, "key-"); String_AppendConst(&name, KeyBind_Names[i]);

void KeyBind_Load(void) {
	UInt32 i;
	UInt8 nameBuffer[String_BufferSize(STRING_SIZE)];
	String name = String_InitAndClearArray(nameBuffer);

	for (i = 0; i < KeyBind_Count; i++) {
		KeyBind_MakeName(name);
		Key mapping = Options_GetEnum(name.buffer, KeyBind_Keys[i], Key_Names, Key_Count);
		if (mapping != Key_Escape) KeyBind_Keys[i] = mapping;
	}
}

void KeyBind_Save(void) {
	UInt32 i;
	UInt8 nameBuffer[String_BufferSize(STRING_SIZE)];
	String name = String_InitAndClearArray(nameBuffer);

	for (i = 0; i < KeyBind_Count; i++) {
		KeyBind_MakeName(name);
		String value = String_FromReadonly(Key_Names[i]);
		Options_Set(name.buffer, &value);
	}
}

void KeyBind_Set(KeyBind binding, Key key) {
	KeyBind_Keys[binding] = key;
	KeyBind_Save();
}

void KeyBind_Init(void) {
	UInt32 i;
	for (i = 0; i < KeyBind_Count; i++) {
		KeyBind_Keys[i] = KeyBind_Defaults[i];
	}
	KeyBind_Load();
}


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
	if (Key_IsControlPressed()) flags |= HOTKEYS_FLAG_CTRL;
	if (Key_IsShiftPressed())   flags |= HOTKEYS_FLAG_SHIFT;
	if (Key_IsAltPressed())     flags |= HOTKEYS_FLAT_ALT;

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
	StringsBuffer_Init(&HotkeysText);
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
		Key hotkey = Utils_ParseEnum(&strKey, Key_Unknown, Key_Names, Array_Elems(Key_Names));
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