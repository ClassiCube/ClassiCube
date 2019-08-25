#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Utils.h"
#include "Logger.h"
#include "Platform.h"
#include "Chat.h"


/*########################################################################################################################*
*-----------------------------------------------------------Key-----------------------------------------------------------*
*#########################################################################################################################*/
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

const char* const Key_Names[KEY_COUNT] = {
	"None",
	Key_Function_Names,
	"ShiftLeft", "ShiftRight", "ControlLeft", "ControlRight",
	"AltLeft", "AltRight", "WinLeft", "WinRight",
	"Up", "Down", "Left", "Right",
	"Number0", "Number1", "Number2", "Number3", "Number4",
	"Number5", "Number6", "Number7", "Number8", "Number9",
	"Insert", "Delete", "Home", "End", "PageUp", "PageDown",
	"Menu",
	Key_Ascii_Names,
	"Enter", "Escape", "Space", "BackSpace", "Tab", "CapsLock",
	"ScrollLock", "PrintScreen", "Pause", "NumLock",
	"Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4",
	"Keypad5", "Keypad6", "Keypad7", "Keypad8", "Keypad9",
	"KeypadDivide", "KeypadMultiply", "KeypadSubtract",
	"KeypadAdd", "KeypadDecimal", "KeypadEnter",	
	"Tilde", "Minus", "Plus", "BracketLeft", "BracketRight", "Slash",
	"Semicolon", "Quote", "Comma", "Period", "BackSlash",
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
	bool wasPressed  = Key_Pressed[key];
	Key_Pressed[key] = pressed;

	if (pressed) {
		Event_RaiseInput(&KeyEvents.Down, key, wasPressed);
	} else if (wasPressed) {
		Event_RaiseInt(&KeyEvents.Up, key);
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
		Event_RaiseInt(&MouseEvents.Down, btn);
	} else {
		Event_RaiseInt(&MouseEvents.Up, btn);
	}
}

void Mouse_SetWheel(float wheel) {
	float delta = wheel - Mouse_Wheel;
	Mouse_Wheel = wheel;
	Event_RaiseFloat(&MouseEvents.Wheel, delta);
}

void Mouse_SetPosition(int x, int y) {
	int deltaX = x - Mouse_X, deltaY = y - Mouse_Y;
	Mouse_X = x; Mouse_Y = y;
	Event_RaiseMouseMove(&MouseEvents.Moved, deltaX, deltaY);
}


/*########################################################################################################################*
*---------------------------------------------------------Keybinds--------------------------------------------------------*
*#########################################################################################################################*/
cc_uint8 KeyBinds[KEYBIND_COUNT];
const cc_uint8 KeyBind_Defaults[KEYBIND_COUNT] = {
	'W', 'S', 'A', 'D',
	KEY_SPACE, 'R', KEY_ENTER, 'T',
	'B', 'F', KEY_ENTER, KEY_TAB, 
	KEY_LSHIFT, 'X', 'Z', 'Q', 'E', 
	KEY_LALT, KEY_F3, KEY_F12, KEY_F11, 
	KEY_F5, KEY_F1, KEY_F7, 'C', 
	KEY_LCTRL, 0, 0, 0, 
	KEY_F6, KEY_LALT, KEY_F8, 
	'G', KEY_F10, 0
};
static const char* const keybindNames[KEYBIND_COUNT] = {
	"Forward", "Back", "Left", "Right",
	"Jump", "Respawn", "SetSpawn", "Chat", "Inventory", 
	"ToggleFog", "SendChat", "PlayerList", 
	"Speed", "NoClip", "Fly", "FlyUp", "FlyDown", 
	"ExtInput", "HideFPS", "Screenshot", "Fullscreen", 
	"ThirdPerson", "HideGUI", "AxisLines", "ZoomScrolling", 
	"HalfSpeed", "MouseLeft", "MouseMiddle", "MouseRight", 
	"AutoRotate", "HotbarSwitching", "SmoothCamera", 
	"DropBlock", "IDOverlay", "BreakableLiquids"
};

bool KeyBind_IsPressed(KeyBind binding) { return Key_Pressed[KeyBinds[binding]]; }

static void KeyBind_Load(void) {
	String name; char nameBuffer[STRING_SIZE + 1];
	Key mapping;
	int i;

	String_InitArray_NT(name, nameBuffer);
	for (i = 0; i < KEYBIND_COUNT; i++) {
		name.length = 0;
		String_Format1(&name, "key-%c", keybindNames[i]);
		name.buffer[name.length] = '\0';

		mapping = Options_GetEnum(name.buffer, KeyBind_Defaults[i], Key_Names, KEY_COUNT);
		if (mapping != KEY_ESCAPE) KeyBinds[i] = mapping;
	}
}

static void KeyBind_Save(void) {
	String name; char nameBuffer[STRING_SIZE];
	String value;
	int i;	

	String_InitArray(name, nameBuffer);
	for (i = 0; i < KEYBIND_COUNT; i++) {
		name.length = 0; 
		String_Format1(&name, "key-%c", keybindNames[i]);

		value = String_FromReadonly(Key_Names[KeyBinds[i]]);
		Options_SetString(&name, &value);
	}
}

void KeyBind_Set(KeyBind binding, Key key) {
	KeyBinds[binding] = key;
	KeyBind_Save();
}

void KeyBind_Init(void) {
	int i;
	for (i = 0; i < KEYBIND_COUNT; i++) {
		KeyBinds[i] = KeyBind_Defaults[i];
	}
	KeyBind_Load();
}


/*########################################################################################################################*
*---------------------------------------------------------Hotkeys---------------------------------------------------------*
*#########################################################################################################################*/
const cc_uint8 Hotkeys_LWJGL[256] = {
	0, KEY_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', KEY_MINUS, KEY_EQUALS, KEY_BACKSPACE, KEY_TAB,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', KEY_LBRACKET, KEY_RBRACKET, KEY_ENTER, KEY_LCTRL, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', KEY_SEMICOLON, KEY_QUOTE, KEY_TILDE, KEY_LSHIFT, KEY_BACKSLASH, 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_RSHIFT, 0, KEY_LALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
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
		cc_uint8 pivot = keys[(i + j) >> 1].Flags;

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

static void Hotkeys_AddNewHotkey(Key trigger, cc_uint8 modifiers, const String* text, bool more) {
	struct HotkeyData hKey;
	hKey.Trigger = trigger;
	hKey.Flags   = modifiers;
	hKey.TextIndex = HotkeysText.count;
	hKey.StaysOpen = more;

	if (HotkeysText.count == HOTKEYS_MAX_COUNT) {
		Chat_AddRaw("&cCannot define more than 256 hotkeys");
		return;
	}

	HotkeysList[HotkeysText.count] = hKey;
	StringsBuffer_Add(&HotkeysText, text);
	/* sort so that hotkeys with largest modifiers are first */
	Hotkeys_QuickSort(0, HotkeysText.count - 1);
}

static void Hotkeys_RemoveText(int index) {
	 struct HotkeyData* hKey = HotkeysList;
	 int i;

	for (i = 0; i < HotkeysText.count; i++, hKey++) {
		if (hKey->TextIndex >= index) hKey->TextIndex--;
	}
	StringsBuffer_Remove(&HotkeysText, index);
}


void Hotkeys_Add(Key trigger, cc_uint8 modifiers, const String* text, bool more) {
	struct HotkeyData* hk = HotkeysList;
	int i;

	for (i = 0; i < HotkeysText.count; i++, hk++) {		
		if (hk->Trigger != trigger || hk->Flags != modifiers) continue;
		Hotkeys_RemoveText(hk->TextIndex);

		hk->StaysOpen = more;
		hk->TextIndex = HotkeysText.count;
		StringsBuffer_Add(&HotkeysText, text);
		return;
	}
	Hotkeys_AddNewHotkey(trigger, modifiers, text, more);
}

bool Hotkeys_Remove(Key trigger, cc_uint8 modifiers) {
	struct HotkeyData* hk = HotkeysList;
	int i, j;

	for (i = 0; i < HotkeysText.count; i++, hk++) {
		if (hk->Trigger != trigger || hk->Flags != modifiers) continue;
		Hotkeys_RemoveText(hk->TextIndex);

		for (j = i; j < HotkeysText.count; j++) {
			HotkeysList[j] = HotkeysList[j + 1];
		}
		return true;
	}
	return false;
}

int Hotkeys_FindPartial(Key key) {
	struct HotkeyData hk;
	int i, modifiers = 0;

	if (Key_IsControlPressed()) modifiers |= HOTKEY_MOD_CTRL;
	if (Key_IsShiftPressed())   modifiers |= HOTKEY_MOD_SHIFT;
	if (Key_IsAltPressed())     modifiers |= HOTKEY_MOD_ALT;

	for (i = 0; i < HotkeysText.count; i++) {
		hk = HotkeysList[i];
		/* e.g. if holding Ctrl and Shift, a hotkey with only Ctrl modifiers matches */
		if ((hk.Flags & modifiers) == hk.Flags && hk.Trigger == key) return i;
	}
	return -1;
}

void Hotkeys_Init(void) {
	static const String prefix = String_FromConst("hotkey-");
	String strKey, strMods, strMore, strText;
	String entry, key, value;
	int i;

	Key trigger;
	cc_uint8 modifiers;
	bool more;

	for (i = 0; i < Options.entries.count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&Options.entries, i);
		String_UNSAFE_Separate(&entry, Options.separator, &key, &value);

		if (!String_CaselessStarts(&key, &prefix)) continue;
		/* Format is: key&modifiers = more-input&text */
		key.length -= prefix.length; key.buffer += prefix.length;
	
		if (!String_UNSAFE_Separate(&key,   '&', &strKey,  &strMods)) continue;
		if (!String_UNSAFE_Separate(&value, '&', &strMore, &strText)) continue;

		trigger = Utils_ParseEnum(&strKey, KEY_NONE, Key_Names, Array_Elems(Key_Names));
		if (trigger == KEY_NONE) continue; 
		if (!Convert_ParseUInt8(&strMods, &modifiers)) continue;
		if (!Convert_ParseBool(&strMore,  &more))      continue;

		Hotkeys_Add(trigger, modifiers, &strText, more);
	}
}

void Hotkeys_UserRemovedHotkey(Key trigger, cc_uint8 modifiers) {
	String key; char keyBuffer[STRING_SIZE];
	String_InitArray(key, keyBuffer);

	String_Format2(&key, "hotkey-%c&%b", Key_Names[trigger], &modifiers);
	Options_SetString(&key, NULL);
}

void Hotkeys_UserAddedHotkey(Key trigger, cc_uint8 modifiers, bool moreInput, const String* text) {
	String key;   char keyBuffer[STRING_SIZE];
	String value; char valueBuffer[STRING_SIZE * 2];
	String_InitArray(key, keyBuffer);
	String_InitArray(value, valueBuffer);

	String_Format2(&key, "hotkey-%c&%b", Key_Names[trigger], &modifiers);
	String_Format2(&value, "%t&%s", &moreInput, text);
	Options_SetString(&key, &value);
}
