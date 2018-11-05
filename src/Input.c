#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Utils.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"


/*########################################################################################################################*
*--------------------------------------------------------Key/Mouse--------------------------------------------------------*
*#########################################################################################################################*/
#define Key_Function_Names \
"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",  "F10",\
"F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",\
"F21", "F22", "F23", "F24", "F25", "F26", "F27", "F28", "F29", "F30",\
"F31", "F32", "F33", "F34", "F35"
#define Key_Ascii_Names \
"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",\
"K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",\
"U", "V", "W", "X", "Y", "Z"

const char* Key_Names[Key_Count] = {
	"None",
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
/*const char* Key_Names[Key_Count] = {
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

void Key_SetPressed(Key key, bool pressed) {
	if (Key_Pressed[key] == pressed && !Key_KeyRepeat) return;
	Key_Pressed[key] = pressed;

	if (pressed) {
		Event_RaiseInt(&KeyEvents_Down, key);
	} else {
		Event_RaiseInt(&KeyEvents_Up, key);
	}
}

void Key_Clear(void) {
	int i;
	for (i = 0; i < Key_Count; i++) {
		if (Key_Pressed[i]) Key_SetPressed((Key)i, false);
	}
}

void Mouse_SetPressed(MouseButton btn, bool pressed) {
	if (Mouse_Pressed[btn] == pressed) return;
	Mouse_Pressed[btn] = pressed;

	if (pressed) {
		Event_RaiseInt(&MouseEvents_Down, btn);
	} else {
		Event_RaiseInt(&MouseEvents_Up, btn);
	}
}

void Mouse_SetWheel(float wheel) {
	float delta = wheel - Mouse_Wheel;
	Mouse_Wheel = wheel;
	Event_RaiseFloat(&MouseEvents_Wheel, delta);
}

void Mouse_SetPosition(int x, int y) {
	int deltaX = x - Mouse_X, deltaY = y - Mouse_Y;
	Mouse_X = x; Mouse_Y = y;
	Event_RaiseMouseMove(&MouseEvents_Moved, deltaX, deltaY);
}


/*########################################################################################################################*
*---------------------------------------------------------Keybinds--------------------------------------------------------*
*#########################################################################################################################*/
Key KeyBind_Keys[KeyBind_Count];
uint8_t KeyBind_Defaults[KeyBind_Count] = {
	Key_W, Key_S, Key_A, Key_D,
	Key_Space, Key_R, Key_Enter, Key_T,
	Key_B, Key_F, Key_Enter, Key_Escape,
	Key_Tab, Key_ShiftLeft, Key_X, Key_Z,
	Key_Q, Key_E, Key_AltLeft, Key_F3,
	Key_F12, Key_F11, Key_F5, Key_F1,
	Key_F7, Key_C, Key_ControlLeft, Key_None,
	Key_None, Key_None, Key_F6, Key_AltLeft, 
	Key_F8, Key_G, Key_F10, Key_None,
};
const char* KeyBind_Names[KeyBind_Count] = {
	"Forward", "Back", "Left", "Right",
	"Jump", "Respawn", "SetSpawn", "Chat",
	"Inventory", "ToggleFog", "SendChat", "PauseOrExit",
	"PlayerList", "Speed", "NoClip", "Fly",
	"FlyUp", "FlyDown", "ExtInput", "HideFPS",
	"Screenshot", "Fullscreen", "ThirdPerson", "HideGUI",
	"AxisLines", "ZoomScrolling", "HalfSpeed", "MouseLeft",
	"MouseMiddle", "MouseRight", "AutoRotate", "HotbarSwitching",
	"SmoothCamera", "DropBlock", "IDOverlay", "BreakableLiquids",
};

Key KeyBind_Get(KeyBind binding) { return KeyBind_Keys[binding]; }
Key KeyBind_GetDefault(KeyBind binding) { return KeyBind_Defaults[binding]; }
bool KeyBind_IsPressed(KeyBind binding) { return Key_Pressed[KeyBind_Keys[binding]]; }

void KeyBind_Load(void) {
	char nameBuffer[STRING_SIZE + 1];
	String name = String_NT_Array(nameBuffer);
	Key mapping;
	int i;

	for (i = 0; i < KeyBind_Count; i++) {
		name.length = 0;
		String_Format1(&name, "key-%c", KeyBind_Names[i]);
		name.buffer[name.length] = '\0';

		mapping = Options_GetEnum(name.buffer, KeyBind_Defaults[i], Key_Names, Key_Count);
		if (mapping != Key_Escape) KeyBind_Keys[i] = mapping;
	}
	KeyBind_Keys[KeyBind_PauseOrExit] = Key_Escape;
}

void KeyBind_Save(void) {
	char nameBuffer[STRING_SIZE];
	String name = String_FromArray(nameBuffer);
	String value;
	int i;

	for (i = 0; i < KeyBind_Count; i++) {
		name.length = 0; 
		String_Format1(&name, "key-%c", KeyBind_Names[i]);

		value = String_FromReadonly(Key_Names[KeyBind_Keys[i]]);
		Options_SetString(&name, &value);
	}
}

void KeyBind_Set(KeyBind binding, Key key) {
	KeyBind_Keys[binding] = key;
	KeyBind_Save();
}

void KeyBind_Init(void) {
	int i;
	for (i = 0; i < KeyBind_Count; i++) {
		KeyBind_Keys[i] = KeyBind_Defaults[i];
	}
	KeyBind_Load();
}


/*########################################################################################################################*
*---------------------------------------------------------Hotkeys---------------------------------------------------------*
*#########################################################################################################################*/
uint8_t Hotkeys_LWJGL[256] = {
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

static void Hotkeys_QuickSort(int left, int right) {
	struct HotkeyData* keys = HotkeysList; struct HotkeyData key;

	while (left < right) {
		int i = left, j = right;
		uint8_t pivot = keys[(i + j) >> 1].Flags;

		/* partition the list */
		while (i <= j) {
			while (pivot < keys[i].Flags) i++;
			while (pivot > keys[j].Flags) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Hotkeys_QuickSort)
	}
}

static void Hotkeys_AddNewHotkey(Key trigger, uint8_t flags, const String* text, bool more) {
	struct HotkeyData hKey;
	hKey.Trigger = trigger;
	hKey.Flags = flags;
	hKey.TextIndex = HotkeysText.Count;
	hKey.StaysOpen = more;

	if (HotkeysText.Count == HOTKEYS_MAX_COUNT) {
		Chat_AddRaw("&cCannot define more than 256 hotkeys");
		return;
	}

	HotkeysList[HotkeysText.Count] = hKey;
	StringsBuffer_Add(&HotkeysText, text);
	/* sort so that hotkeys with largest modifiers are first */
	Hotkeys_QuickSort(0, HotkeysText.Count - 1);
}

static void Hotkeys_RemoveText(int index) {
	 struct HotkeyData* hKey = HotkeysList;
	 int i;

	for (i = 0; i < HotkeysText.Count; i++, hKey++) {
		if (hKey->TextIndex >= index) hKey->TextIndex--;
	}
	StringsBuffer_Remove(&HotkeysText, index);
}


void Hotkeys_Add(Key trigger, HotkeyFlags flags, const String* text, bool more) {
	struct HotkeyData* hKey = HotkeysList;
	int i;

	for (i = 0; i < HotkeysText.Count; i++, hKey++) {		
		if (hKey->Trigger != trigger || hKey->Flags != flags) continue;
		Hotkeys_RemoveText(hKey->TextIndex);

		hKey->StaysOpen = more;
		hKey->TextIndex = HotkeysText.Count;
		StringsBuffer_Add(&HotkeysText, text);
		return;
	}
	Hotkeys_AddNewHotkey(trigger, flags, text, more);
}

bool Hotkeys_Remove(Key trigger, HotkeyFlags flags) {
	struct HotkeyData* hKey = HotkeysList;
	int i, j;

	for (i = 0; i < HotkeysText.Count; i++, hKey++) {
		if (hKey->Trigger != trigger || hKey->Flags != flags) continue;
		Hotkeys_RemoveText(hKey->TextIndex);

		for (j = i; j < HotkeysText.Count; j++) {
			HotkeysList[j] = HotkeysList[j + 1];
		}
		return true;
	}
	return false;
}

int Hotkeys_FindPartial(Key key) {
	struct HotkeyData hKey;
	int i, flags = 0;

	if (Key_IsControlPressed()) flags |= HOTKEY_FLAG_CTRL;
	if (Key_IsShiftPressed())   flags |= HOTKEY_FLAG_SHIFT;
	if (Key_IsAltPressed())     flags |= HOTKEY_FLAG_ALT;

	for (i = 0; i < HotkeysText.Count; i++) {
		hKey = HotkeysList[i];
		/* e.g. if holding Ctrl and Shift, a hotkey with only Ctrl flags matches */
		if ((hKey.Flags & flags) == hKey.Flags && hKey.Trigger == key) return i;
	}
	return -1;
}

void Hotkeys_Init(void) {
	static String prefix = String_FromConst("hotkey-");
	String strKey, strMods, strMore, strText;
	String key, value;
	int i;

	Key trigger;
	uint8_t modifiers;
	bool more;

	for (i = 0; i < Options_Keys.Count; i++) {
		key = StringsBuffer_UNSAFE_Get(&Options_Keys, i);
		if (!String_CaselessStarts(&key, &prefix)) continue;

		/* Format is: key&modifiers = more-input&text */
		key.length -= prefix.length; key.buffer += prefix.length;
		value = StringsBuffer_UNSAFE_Get(&Options_Values, i);
	
		if (!String_UNSAFE_Separate(&key,   '&', &strKey,  &strMods)) continue;
		if (!String_UNSAFE_Separate(&value, '&', &strMore, &strText)) continue;

		trigger = Utils_ParseEnum(&strKey, Key_None, Key_Names, Array_Elems(Key_Names));
		if (trigger == Key_None) continue; 
		if (!Convert_TryParseUInt8(&strMods, &modifiers)) continue;
		if (!Convert_TryParseBool(&strMore,  &more))      continue;

		Hotkeys_Add(trigger, modifiers, &strText, more);
	}
}

void Hotkeys_UserRemovedHotkey(Key trigger, HotkeyFlags flags) {
	String key; char keyBuffer[STRING_SIZE];
	String_InitArray(key, keyBuffer);

	String_Format2(&key, "hotkey-%c&%i", Key_Names[trigger], &flags);
	Options_SetString(&key, NULL);
}

void Hotkeys_UserAddedHotkey(Key trigger, HotkeyFlags flags, bool moreInput, const String* text) {
	String key;   char keyBuffer[STRING_SIZE];
	String value; char valueBuffer[STRING_SIZE * 2];
	String_InitArray(key, keyBuffer);
	String_InitArray(value, valueBuffer);

	String_Format2(&key, "hotkey-%c&%i", Key_Names[trigger], &flags);
	String_Format2(&value, "%t&%s", &moreInput, text);
	Options_SetString(&key, &value);
}
