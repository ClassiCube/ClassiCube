#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Utils.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"


/*########################################################################################################################*
*-----------------------------------------------------------Key-----------------------------------------------------------*
*#########################################################################################################################*/
bool Key_KeyRepeat;
bool Key_Pressed[KEY_COUNT];

#define Key_Function_Names \
"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",  "F10",\
"F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",\
"F21", "F22", "F23", "F24", "F25", "F26", "F27", "F28", "F29", "F30",\
"F31", "F32", "F33", "F34", "F35"
#define Key_Ascii_Names \
"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",\
"K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",\
"U", "V", "W", "X", "Y", "Z"

const char* Key_Names[KEY_COUNT] = {
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
/*const char* Key_Names[KEY_COUNT] = {
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
	for (i = 0; i < KEY_COUNT; i++) {
		if (Key_Pressed[i]) Key_SetPressed((Key)i, false);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------Mouse----------------------------------------------------------*
*#########################################################################################################################*/
float Mouse_Wheel;
int Mouse_X, Mouse_Y;
bool Mouse_Pressed[MOUSE_COUNT];

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
static Key KeyBind_Keys[KEYBIND_COUNT];
static uint8_t KeyBind_Defaults[KEYBIND_COUNT] = {
	KEY_W, KEY_S, KEY_A, KEY_D,
	KEY_SPACE, KEY_R, KEY_ENTER, KEY_T,
	KEY_B, KEY_F, KEY_ENTER, KEY_ESCAPE,
	KEY_TAB, KEY_LSHIFT, KEY_X, KEY_Z,
	KEY_Q, KEY_E, KEY_LALT, KEY_F3,
	KEY_F12, KEY_F11, KEY_F5, KEY_F1,
	KEY_F7, KEY_C, KEY_LCTRL, KEY_NONE,
	KEY_NONE, KEY_NONE, KEY_F6, KEY_LALT, 
	KEY_F8, KEY_G, KEY_F10, KEY_NONE,
};
const char* KeyBind_Names[KEYBIND_COUNT] = {
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
	String name; char nameBuffer[STRING_SIZE + 1];
	Key mapping;
	int i;

	String_InitArray_NT(name, nameBuffer);
	for (i = 0; i < KEYBIND_COUNT; i++) {
		name.length = 0;
		String_Format1(&name, "key-%c", KeyBind_Names[i]);
		name.buffer[name.length] = '\0';

		mapping = Options_GetEnum(name.buffer, KeyBind_Defaults[i], Key_Names, KEY_COUNT);
		if (mapping != KEY_ESCAPE) KeyBind_Keys[i] = mapping;
	}
	KeyBind_Keys[KEYBIND_PAUSE_EXIT] = KEY_ESCAPE;
}

void KeyBind_Save(void) {
	String name; char nameBuffer[STRING_SIZE];
	String value;
	int i;	

	String_InitArray(name, nameBuffer);
	for (i = 0; i < KEYBIND_COUNT; i++) {
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
	for (i = 0; i < KEYBIND_COUNT; i++) {
		KeyBind_Keys[i] = KeyBind_Defaults[i];
	}
	KeyBind_Load();
}


/*########################################################################################################################*
*---------------------------------------------------------Hotkeys---------------------------------------------------------*
*#########################################################################################################################*/
const uint8_t Hotkeys_LWJGL[256] = {
	0, KEY_ESCAPE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_PLUS, KEY_BACKSPACE, KEY_TAB,
	KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LBRACKET, KEY_RBRACKET, KEY_ENTER, KEY_LCTRL, KEY_A, KEY_S,
	KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_QUOTE, KEY_TILDE, KEY_LSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V,
	KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_RSHIFT, 0, KEY_LALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KP_MINUS, KEY_KP4, KEY_KP5, KEY_KP6, KEY_KP_PLUS, KEY_KP1,
	KEY_KP2, KEY_KP3, KEY_KP0, KEY_KP_DECIMAL, 0, 0, 0, KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_KP_PLUS, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_KP_ENTER, KEY_RCTRL, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, KEY_KP_DIVIDE, 0, 0, KEY_RALT, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, KEY_PAUSE, 0, KEY_HOME, KEY_UP, KEY_PAGEUP, 0, KEY_LEFT, 0, KEY_RIGHT, 0, KEY_END,
	KEY_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, 0, 0, 0, 0, 0, 0, 0, KEY_LWIN, KEY_RWIN, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
struct HotkeyData HotkeysList[HOTKEYS_MAX_COUNT];
StringsBuffer HotkeysText;

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

		trigger = Utils_ParseEnum(&strKey, KEY_NONE, Key_Names, Array_Elems(Key_Names));
		if (trigger == KEY_NONE) continue; 
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
