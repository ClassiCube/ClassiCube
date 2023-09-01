#include "Input.h"
#include "String.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Logger.h"
#include "Platform.h"
#include "Chat.h"
#include "Utils.h"
#include "Server.h"
#include "HeldBlockRenderer.h"
#include "Game.h"
#include "ExtMath.h"
#include "Camera.h"
#include "Inventory.h"
#include "World.h"
#include "Event.h"
#include "Window.h"
#include "Entity.h"
#include "Screens.h"
#include "Block.h"
#include "Menus.h"
#include "Gui.h"
#include "Protocol.h"
#include "AxisLinesRenderer.h"
#include "Picking.h"

struct _InputState Input;
static cc_bool input_buttonsDown[3];
static int input_pickingId = -1;
static TimeMS input_lastClick;
static float input_fovIndex = -1.0f;
#ifdef CC_BUILD_WEB
static cc_bool suppressEscape;
#endif
enum MouseButton_ { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE };
/* Raises PointerEvents.Up or PointerEvents.Down */
static void Pointer_SetPressed(int idx, cc_bool pressed);


/*########################################################################################################################*
*------------------------------------------------------Touch support------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_TOUCH
static struct TouchPointer {
	long id;
	cc_uint8 type;
	int begX, begY;
	TimeMS start;
} touches[INPUT_MAX_POINTERS];
int Pointers_Count;
int Input_TapMode  = INPUT_MODE_PLACE;
int Input_HoldMode = INPUT_MODE_DELETE;
cc_bool Input_TouchMode;

static void MouseStatePress(int button);
static void MouseStateRelease(int button);

static cc_bool AnyBlockTouches(void) {
	int i;
	for (i = 0; i < Pointers_Count; i++) {
		if (!(touches[i].type & TOUCH_TYPE_BLOCKS)) continue;

		/* Touch might be an 'all' type - remove 'gui' type */
		touches[i].type &= TOUCH_TYPE_BLOCKS | TOUCH_TYPE_CAMERA;
		return true;
	}
	return false;
}

static void ClearTouches(void) {
	int i;
	for (i = 0; i < INPUT_MAX_POINTERS; i++) touches[i].type = 0;
	Pointers_Count = Input_TouchMode ? 0 : 1;
}

void Input_SetTouchMode(cc_bool enabled) {
	Input_TouchMode = enabled;
	ClearTouches();
}

static cc_bool MovedFromBeg(int i, int x, int y) {
	return Math_AbsI(x - touches[i].begX) > Display_ScaleX(5) ||
		   Math_AbsI(y - touches[i].begY) > Display_ScaleY(5);
}

static cc_bool TryUpdateTouch(long id, int x, int y) {
	int i;
	for (i = 0; i < Pointers_Count; i++) {
		if (touches[i].id != id || !touches[i].type) continue;

		if (Input.RawMode && (touches[i].type & TOUCH_TYPE_CAMERA)) {
			/* If the pointer hasn't been locked to gui or block yet, moving a bit */
			/* should cause the pointer to get locked to camera movement. */
			if (touches[i].type == TOUCH_TYPE_ALL && MovedFromBeg(i, x, y)) {
				/* Allow a little bit of leeway because though, because devices */
				/* might still report a few pixels of movement depending on how */
				/* user is holding the finger down on the touch surface */
				if (touches[i].type == TOUCH_TYPE_ALL) touches[i].type = TOUCH_TYPE_CAMERA;
			}
			Event_RaiseRawMove(&PointerEvents.RawMoved, x - Pointers[i].x, y - Pointers[i].y);
		}
		Pointer_SetPosition(i, x, y);
		return true;
	}
	return false;
}

void Input_AddTouch(long id, int x, int y) {
	int i;
	/* Check if already existing pointer with same ID */
	if (TryUpdateTouch(id, x, y)) return;

	for (i = 0; i < INPUT_MAX_POINTERS; i++) {
		if (touches[i].type) continue;

		touches[i].id   = id;
		touches[i].type = TOUCH_TYPE_ALL;
		touches[i].begX = x;
		touches[i].begY = y;

		touches[i].start = DateTime_CurrentUTC_MS();
		/* Also set last click time, otherwise quickly tapping */
		/* sometimes triggers a 'delete' in InputHandler_Tick, */
		/* and then another 'delete' in CheckBlockTap. */
		input_lastClick  = touches[i].start;

		if (i == Pointers_Count) Pointers_Count++;
		Pointer_SetPosition(i, x, y);
		Pointer_SetPressed(i, true);
		return;
	}
}
void Input_UpdateTouch(long id, int x, int y) { TryUpdateTouch(id, x, y); }

/* Quickly tapping should trigger a block place/delete */
static void CheckBlockTap(int i) {
	int btn, pressed;
	if (DateTime_CurrentUTC_MS() > touches[i].start + 250) return;
	if (touches[i].type != TOUCH_TYPE_ALL) return;

	if (Input_TapMode == INPUT_MODE_PLACE) {
		btn = MOUSE_RIGHT;
	} else if (Input_TapMode == INPUT_MODE_DELETE) {
		btn = MOUSE_LEFT;
	} else { return; }

	pressed = input_buttonsDown[btn];
	MouseStatePress(btn);

	if (btn == MOUSE_LEFT) { 
		InputHandler_DeleteBlock();
	} else { 
		InputHandler_PlaceBlock();
	}
	if (!pressed) MouseStateRelease(btn);
}

void Input_RemoveTouch(long id, int x, int y) {
	int i;
	for (i = 0; i < Pointers_Count; i++) {
		if (touches[i].id != id || !touches[i].type) continue;

		Pointer_SetPosition(i, x, y);
		Pointer_SetPressed(i, false);

		/* found the touch, remove it */
		Pointer_SetPosition(i, -100000, -100000);
		touches[i].type = 0;

		if ((i + 1) == Pointers_Count) Pointers_Count--;
		return;
	}
}
#else
static void ClearTouches(void) { }
#endif


/*########################################################################################################################*
*-----------------------------------------------------------Key-----------------------------------------------------------*
*#########################################################################################################################*/
#define Key_Function_Names \
"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",  "F10",\
"F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",\
"F21", "F22", "F23", "F24"
#define Key_Ascii_Names \
"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",\
"K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",\
"U", "V", "W", "X", "Y", "Z"
#define Pad_Names \
"PAD_A", "PAD_B", "PAD_X", "PAD_Y", "PAD_L", "PAD_R", \
"PAD_LEFT", "PAD_RIGHT", "PAD_UP", "PAD_DOWN", \
"PAD_START", "PAD_SELECT", "PAD_ZL", "PAD_ZR"

/* Names for each input button when stored to disc */
static const char* const storageNames[INPUT_COUNT] = {
	"None",
	Key_Function_Names,
	"Tilde", "Minus", "Plus", "BracketLeft", "BracketRight", "Slash",
	"Semicolon", "Quote", "Comma", "Period", "BackSlash",
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
	"XButton1", "XButton2", "LeftMouse", "RightMouse", "MiddleMouse",
	Pad_Names
};

const char* const Input_DisplayNames[INPUT_COUNT] = {
	"NONE",
	Key_Function_Names,
	"GRAVE", "MINUS", "PLUS", "LBRACKET", "RBRACKET", "SLASH",
	"SEMICOLON", "APOSTROPHE", "COMMA", "PERIOD", "BACKSLASH",
	"LSHIFT", "RSHIFT", "LCONTROL", "RCONTROL",
	"LALT", "RALT", "LWIN", "RWIN",
	"UP", "DOWN", "LEFT", "RIGHT",
	"0", "1", "2", "3", "4",
	"5", "6", "7", "8", "9",
	"INSERT", "DELETE", "HOME", "END", "PRIOR", "DOWN",
	"MENU",
	Key_Ascii_Names,
	"RETURN", "ESCAPE", "SPACE", "BACK", "TAB", "CAPITAL",
	"SCROLL", "PRINT", "PAUSE", "NUMLOCK",
	"NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3", "NUMPAD4",
	"NUMPAD5", "NUMPAD6", "NUMPAD7", "NUMPAD8", "NUMPAD9",
	"DIVIDE", "MULTIPLY", "SUBTRACT",
	"ADD", "DECIMAL", "NUMPADENTER",
	"XBUTTON1", "XBUTTON2", "LMOUSE", "RMOUSE", "MMOUSE",
	Pad_Names
};

void Input_SetPressed(int key) {
	cc_bool wasPressed = Input.Pressed[key];
	Input.Pressed[key] = true;
	Event_RaiseInput(&InputEvents.Down, key, wasPressed);

	if (key == 'C' && Input_IsActionPressed()) Event_RaiseInput(&InputEvents.Down, INPUT_CLIPBOARD_COPY,  0);
	if (key == 'V' && Input_IsActionPressed()) Event_RaiseInput(&InputEvents.Down, INPUT_CLIPBOARD_PASTE, 0);

	/* don't allow multiple left mouse down events */
	if (key != CCMOUSE_L || wasPressed) return;
	Pointer_SetPressed(0, true);
}

void Input_SetReleased(int key) {
	if (!Input.Pressed[key]) return;
	Input.Pressed[key] = false;

	Event_RaiseInt(&InputEvents.Up, key);
	if (key == CCMOUSE_L) Pointer_SetPressed(0, false);
}

void Input_Set(int key, int pressed) {
	if (pressed) {
		Input_SetPressed(key);
	} else {
		Input_SetReleased(key);
	}
}

void Input_SetNonRepeatable(int key, int pressed) {
	if (pressed) {
		if (Input.Pressed[key]) return;
		Input_SetPressed(key);
	} else {
		Input_SetReleased(key);
	}
}

void Input_Clear(void) {
	int i;
	for (i = 0; i < INPUT_COUNT; i++) 
	{
		if (Input.Pressed[i]) Input_SetReleased(i);
	}
	/* TODO: Properly release instead of just clearing */
	ClearTouches();
}


/*########################################################################################################################*
*----------------------------------------------------------Mouse----------------------------------------------------------*
*#########################################################################################################################*/
struct Pointer Pointers[INPUT_MAX_POINTERS];

void Pointer_SetPressed(int idx, cc_bool pressed) {
	if (pressed) {
		Event_RaiseInt(&PointerEvents.Down, idx);
	} else {
		Event_RaiseInt(&PointerEvents.Up,   idx);
	}
}

void Mouse_ScrollWheel(float delta) {
	Event_RaiseFloat(&InputEvents.Wheel, delta);
}

void Pointer_SetPosition(int idx, int x, int y) {
	if (x == Pointers[idx].x && y == Pointers[idx].y) return;
	/* TODO: reset to -1, -1 when pointer is removed */
	Pointers[idx].x = x; Pointers[idx].y = y;
	
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode && !(touches[idx].type & TOUCH_TYPE_GUI)) return;
#endif
	Event_RaiseInt(&PointerEvents.Moved, idx);
}


/*########################################################################################################################*
*---------------------------------------------------------Keybinds--------------------------------------------------------*
*#########################################################################################################################*/
cc_uint8 KeyBinds_Gamepad[KEYBIND_COUNT];
cc_uint8 KeyBinds_Normal[KEYBIND_COUNT];

const cc_uint8 KeyBind_GamepadDefaults[KEYBIND_COUNT] = {
	CCPAD_UP, CCPAD_DOWN, CCPAD_LEFT, CCPAD_RIGHT, /* Movement */
	CCPAD_A, 0, CCPAD_START, CCPAD_Y, /* Jump, SetSpawn, OpenChat */
	CCPAD_X, 0, CCPAD_START, 0,       /* Inventory, EnterChat */
	0, 0, 0, 0, 0,                    /* Hacks */
	0, 0, 0, 0,                       /* LAlt - F11 */
	0, 0, 0, 0,                       /* F5 - C */
	0, CCPAD_L, 0, CCPAD_R,
	0, 0, 0,
	0,0,0, 0,0,0,0,
	0,0,0, 0,0,0, 0,0,0, /* Hotbar slots */
	CCPAD_ZL, CCPAD_ZR
};
const cc_uint8 KeyBind_NormalDefaults[KEYBIND_COUNT] = {
	'W', 'S', 'A', 'D',
	CCKEY_SPACE, 'R', CCKEY_ENTER, 'T',
	'B', 'F', CCKEY_ENTER, CCKEY_TAB, 
	CCKEY_LSHIFT, 'X', 'Z', 'Q', 'E', 
	CCKEY_LALT, CCKEY_F3, CCKEY_F12, CCKEY_F11, 
	CCKEY_F5, CCKEY_F1, CCKEY_F7, 'C', 
	CCKEY_LCTRL, CCMOUSE_L, CCMOUSE_M, CCMOUSE_R, 
	CCKEY_F6, CCKEY_LALT, CCKEY_F8, 
	'G', CCKEY_F10, 0,
	0, 0, 0, 0,
	'1','2','3', '4','5','6', '7','8','9',
	0, 0
};

static const char* const keybindNames[KEYBIND_COUNT] = {
	"Forward", "Back", "Left", "Right",
	"Jump", "Respawn", "SetSpawn", "Chat", "Inventory", 
	"ToggleFog", "SendChat", "PlayerList", 
	"Speed", "NoClip", "Fly", "FlyUp", "FlyDown", 
	"ExtInput", "HideFPS", "Screenshot", "Fullscreen", 
	"ThirdPerson", "HideGUI", "AxisLines", "ZoomScrolling", 
	"HalfSpeed", "DeleteBlock", "PickBlock", "PlaceBlock", 
	"AutoRotate", "HotbarSwitching", "SmoothCamera", 
	"DropBlock", "IDOverlay", "BreakableLiquids",
	"LookUp", "LookDown", "LookRight", "LookLeft",
	"Hotbar1", "Hotbar2", "Hotbar3",
	"Hotbar4", "Hotbar5", "Horbar6",
	"Hotbar7", "Hotbar8", "Hotbar9",
	"HotbarLeft", "HotbarRight"
};

cc_bool KeyBind_IsPressed(KeyBind binding) { 
	return Input.Pressed[KeyBinds_Normal[binding]] || 
		   Input.Pressed[KeyBinds_Gamepad[binding]];
}

static void KeyBind_Load(const char* prefix, cc_uint8* keybinds, const cc_uint8* defaults) {
	cc_string name; char nameBuffer[STRING_SIZE + 1];
	int mapping, i;

	String_InitArray_NT(name, nameBuffer);
	for (i = 0; i < KEYBIND_COUNT; i++) 
	{
		name.length = 0;
		String_Format1(&name, prefix, keybindNames[i]);
		name.buffer[name.length] = '\0';

		mapping = Options_GetEnum(name.buffer, defaults[i], storageNames, INPUT_COUNT);
		if (mapping == CCKEY_ESCAPE) mapping = defaults[i];
		
		keybinds[i] = mapping;
	}
}

void KeyBind_Set(KeyBind binding, int key, cc_uint8* binds) {
	cc_string name; char nameBuffer[STRING_SIZE];
	cc_string value;
	String_InitArray(name, nameBuffer);

	String_Format1(&name, binds == KeyBinds_Gamepad ? "pad-%c" : "key-%c", 
		keybindNames[binding]);
	value = String_FromReadonly(storageNames[key]);
	Options_SetString(&name, &value);
	binds[binding] = key;
}

/* Initialises and loads key bindings from options */
static void KeyBind_Init(void) {
	KeyBind_Load("key-%c", KeyBinds_Normal,  KeyBind_NormalDefaults);
	KeyBind_Load("pad-%c", KeyBinds_Gamepad, KeyBind_GamepadDefaults);
}


/*########################################################################################################################*
*---------------------------------------------------------Hotkeys---------------------------------------------------------*
*#########################################################################################################################*/
const cc_uint8 Hotkeys_LWJGL[256] = {
	0, CCKEY_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', CCKEY_MINUS, CCKEY_EQUALS, CCKEY_BACKSPACE, CCKEY_TAB,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', CCKEY_LBRACKET, CCKEY_RBRACKET, CCKEY_ENTER, CCKEY_LCTRL, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', CCKEY_SEMICOLON, CCKEY_QUOTE, CCKEY_TILDE, CCKEY_LSHIFT, CCKEY_BACKSLASH, 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', CCKEY_COMMA, CCKEY_PERIOD, CCKEY_SLASH, CCKEY_RSHIFT, 0, CCKEY_LALT, CCKEY_SPACE, CCKEY_CAPSLOCK, CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5,
	CCKEY_F6, CCKEY_F7, CCKEY_F8, CCKEY_F9, CCKEY_F10, CCKEY_NUMLOCK, CCKEY_SCROLLLOCK, CCKEY_KP7, CCKEY_KP8, CCKEY_KP9, CCKEY_KP_MINUS, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP_PLUS, CCKEY_KP1,
	CCKEY_KP2, CCKEY_KP3, CCKEY_KP0, CCKEY_KP_DECIMAL, 0, 0, 0, CCKEY_F11, CCKEY_F12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, CCKEY_F13, CCKEY_F14, CCKEY_F15, CCKEY_F16, CCKEY_F17, CCKEY_F18, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CCKEY_KP_PLUS, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CCKEY_KP_ENTER, CCKEY_RCTRL, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, CCKEY_KP_DIVIDE, 0, 0, CCKEY_RALT, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, CCKEY_PAUSE, 0, CCKEY_HOME, CCKEY_UP, CCKEY_PAGEUP, 0, CCKEY_LEFT, 0, CCKEY_RIGHT, 0, CCKEY_END,
	CCKEY_DOWN, CCKEY_PAGEDOWN, CCKEY_INSERT, CCKEY_DELETE, 0, 0, 0, 0, 0, 0, 0, CCKEY_LWIN, CCKEY_RWIN, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
struct HotkeyData HotkeysList[HOTKEYS_MAX_COUNT];
struct StringsBuffer HotkeysText;

static void Hotkeys_QuickSort(int left, int right) {
	struct HotkeyData* keys = HotkeysList; struct HotkeyData key;

	while (left < right) {
		int i = left, j = right;
		cc_uint8 pivot = keys[(i + j) >> 1].mods;

		/* partition the list */
		while (i <= j) {
			while (pivot < keys[i].mods) i++;
			while (pivot > keys[j].mods) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Hotkeys_QuickSort)
	}
}

static void Hotkeys_AddNewHotkey(int trigger, cc_uint8 modifiers, const cc_string* text, cc_uint8 flags) {
	struct HotkeyData hKey;
	hKey.trigger = trigger;
	hKey.mods    = modifiers;
	hKey.textIndex = HotkeysText.count;
	hKey.flags   = flags;

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
		if (hKey->textIndex >= index) hKey->textIndex--;
	}
	StringsBuffer_Remove(&HotkeysText, index);
}


void Hotkeys_Add(int trigger, cc_uint8 modifiers, const cc_string* text, cc_uint8 flags) {
	struct HotkeyData* hk = HotkeysList;
	int i;

	for (i = 0; i < HotkeysText.count; i++, hk++) {		
		if (hk->trigger != trigger || hk->mods != modifiers) continue;
		Hotkeys_RemoveText(hk->textIndex);

		hk->flags     = flags;
		hk->textIndex = HotkeysText.count;
		StringsBuffer_Add(&HotkeysText, text);
		return;
	}
	Hotkeys_AddNewHotkey(trigger, modifiers, text, flags);
}

cc_bool Hotkeys_Remove(int trigger, cc_uint8 modifiers) {
	struct HotkeyData* hk = HotkeysList;
	int i, j;

	for (i = 0; i < HotkeysText.count; i++, hk++) {
		if (hk->trigger != trigger || hk->mods != modifiers) continue;
		Hotkeys_RemoveText(hk->textIndex);

		for (j = i; j < HotkeysText.count; j++) {
			HotkeysList[j] = HotkeysList[j + 1];
		}
		return true;
	}
	return false;
}

int Hotkeys_FindPartial(int key) {
	struct HotkeyData hk;
	int i, modifiers = 0;

	if (Input_IsCtrlPressed())  modifiers |= HOTKEY_MOD_CTRL;
	if (Input_IsShiftPressed()) modifiers |= HOTKEY_MOD_SHIFT;
	if (Input_IsAltPressed())   modifiers |= HOTKEY_MOD_ALT;

	for (i = 0; i < HotkeysText.count; i++) {
		hk = HotkeysList[i];
		/* e.g. if holding Ctrl and Shift, a hotkey with only Ctrl modifiers matches */
		if ((hk.mods & modifiers) == hk.mods && hk.trigger == key) return i;
	}
	return -1;
}

static const cc_string prefix = String_FromConst("hotkey-");
static void StoredHotkey_Parse(cc_string* key, cc_string* value) {
	cc_string strKey, strMods, strMore, strText;
	int trigger;
	cc_uint8 modifiers;
	cc_bool more;

	/* Format is: key&modifiers = more-input&text */
	key->length -= prefix.length; key->buffer += prefix.length;
	
	if (!String_UNSAFE_Separate(key,   '&', &strKey,  &strMods)) return;
	if (!String_UNSAFE_Separate(value, '&', &strMore, &strText)) return;
	
	trigger = Utils_ParseEnum(&strKey, INPUT_NONE, storageNames, INPUT_COUNT);
	if (trigger == INPUT_NONE) return; 
	if (!Convert_ParseUInt8(&strMods, &modifiers)) return;
	if (!Convert_ParseBool(&strMore,  &more))      return;
	
	Hotkeys_Add(trigger, modifiers, &strText, more);
}

static void StoredHotkeys_LoadAll(void) {
	cc_string entry, key, value;
	int i;

	for (i = 0; i < Options.count; i++) {
		StringsBuffer_UNSAFE_GetRaw(&Options, i, &entry);
		String_UNSAFE_Separate(&entry, '=', &key, &value);

		if (!String_CaselessStarts(&key, &prefix)) continue;
		StoredHotkey_Parse(&key, &value);
	}
}

void StoredHotkeys_Load(int trigger, cc_uint8 modifiers) {
	cc_string key, value; char keyBuffer[STRING_SIZE];
	String_InitArray(key, keyBuffer);

	String_Format2(&key, "hotkey-%c&%b", storageNames[trigger], &modifiers);
	key.buffer[key.length] = '\0'; /* TODO: Avoid this null terminator */

	Options_UNSAFE_Get(key.buffer, &value);
	StoredHotkey_Parse(&key, &value);
}

void StoredHotkeys_Remove(int trigger, cc_uint8 modifiers) {
	cc_string key; char keyBuffer[STRING_SIZE];
	String_InitArray(key, keyBuffer);

	String_Format2(&key, "hotkey-%c&%b", storageNames[trigger], &modifiers);
	Options_SetString(&key, NULL);
}

void StoredHotkeys_Add(int trigger, cc_uint8 modifiers, cc_bool moreInput, const cc_string* text) {
	cc_string key;   char keyBuffer[STRING_SIZE];
	cc_string value; char valueBuffer[STRING_SIZE * 2];
	String_InitArray(key, keyBuffer);
	String_InitArray(value, valueBuffer);

	String_Format2(&key, "hotkey-%c&%b", storageNames[trigger], &modifiers);
	String_Format2(&value, "%t&%s", &moreInput, text);
	Options_SetString(&key, &value);
}


/*########################################################################################################################*
*-----------------------------------------------------Mouse helpers-------------------------------------------------------*
*#########################################################################################################################*/
static void MouseStateUpdate(int button, cc_bool pressed) {
	struct Entity* p;
	/* defer getting the targeted entity, as it's a costly operation */
	if (input_pickingId == -1) {
		p = &LocalPlayer_Instance.Base;
		input_pickingId = Entities_GetClosest(p);
	}

	input_buttonsDown[button] = pressed;
	CPE_SendPlayerClick(button, pressed, (EntityID)input_pickingId, &Game_SelectedPos);	
}

static void MouseStateChanged(int button, cc_bool pressed) {
	if (!Server.SupportsPlayerClick) return;

	if (pressed) {
		/* Can send multiple Pressed events */
		MouseStateUpdate(button, true);
	} else {
		if (!input_buttonsDown[button]) return;
		MouseStateUpdate(button, false);
	}
}

static void MouseStatePress(int button) {
	input_lastClick = DateTime_CurrentUTC_MS();
	input_pickingId = -1;
	MouseStateChanged(button, true);
}

static void MouseStateRelease(int button) {
	input_pickingId = -1;
	MouseStateChanged(button, false);
}

void InputHandler_OnScreensChanged(void) {
	input_lastClick = DateTime_CurrentUTC_MS();
	input_pickingId = -1;
	if (!Gui.InputGrab) return;

	/* If input is grabbed, then the mouse isn't used for picking blocks in world anymore. */
	/* So release all mouse buttons, since game stops sending PlayerClick during grabbed input */
	MouseStateChanged(MOUSE_LEFT,   false);
	MouseStateChanged(MOUSE_RIGHT,  false);
	MouseStateChanged(MOUSE_MIDDLE, false);
}

static cc_bool TouchesSolid(BlockID b) { return Blocks.Collide[b] == COLLIDE_SOLID; }
static cc_bool PushbackPlace(struct AABB* blockBB) {
	struct Entity* p        = &LocalPlayer_Instance.Base;
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;
	Face closestFace;
	cc_bool insideMap;

	Vec3 pos = p->Position;
	struct AABB playerBB;
	struct LocationUpdate update;

	/* Offset position by the closest face */
	closestFace = Game_SelectedPos.Closest;
	if (closestFace == FACE_XMAX) {
		pos.X = blockBB->Max.X + 0.5f;
	} else if (closestFace == FACE_ZMAX) {
		pos.Z = blockBB->Max.Z + 0.5f;
	} else if (closestFace == FACE_XMIN) {
		pos.X = blockBB->Min.X - 0.5f;
	} else if (closestFace == FACE_ZMIN) {
		pos.Z = blockBB->Min.Z - 0.5f;
	} else if (closestFace == FACE_YMAX) {
		pos.Y = blockBB->Min.Y + 1 + ENTITY_ADJUSTMENT;
	} else if (closestFace == FACE_YMIN) {
		pos.Y = blockBB->Min.Y - p->Size.Y - ENTITY_ADJUSTMENT;
	}

	/* Exclude exact map boundaries, otherwise player can get stuck outside map */
	/* Being vertically above the map is acceptable though */
	insideMap =
		pos.X > 0.0f && pos.Y >= 0.0f && pos.Z > 0.0f &&
		pos.X < World.Width && pos.Z < World.Length;
	if (!insideMap) return false;

	AABB_Make(&playerBB, &pos, &p->Size);
	if (!hacks->Noclip && Entity_TouchesAny(&playerBB, TouchesSolid)) {
		/* Don't put player inside another block */
		return false;
	}

	update.flags = LU_HAS_POS | LU_POS_ABSOLUTE_INSTANT;
	update.pos   = pos;
	p->VTABLE->SetLocation(p, &update);
	return true;
}

static cc_bool IntersectsOthers(Vec3 pos, BlockID block) {
	struct AABB blockBB, entityBB;
	struct Entity* e;
	int id;

	Vec3_Add(&blockBB.Min, &pos, &Blocks.MinBB[block]);
	Vec3_Add(&blockBB.Max, &pos, &Blocks.MaxBB[block]);
	
	for (id = 0; id < ENTITIES_SELF_ID; id++) {
		e = Entities.List[id];
		if (!e) continue;

		Entity_GetBounds(e, &entityBB);
		entityBB.Min.Y += 1.0f / 32.0f; /* when player is exactly standing on top of ground */
		if (AABB_Intersects(&entityBB, &blockBB)) return true;
	}
	return false;
}

static cc_bool CheckIsFree(BlockID block) {
	struct Entity* p        = &LocalPlayer_Instance.Base;
	struct HacksComp* hacks = &LocalPlayer_Instance.Hacks;

	Vec3 pos, nextPos;
	struct AABB blockBB, playerBB;
	struct LocationUpdate update;

	/* Non solid blocks (e.g. water/flowers) can always be placed on players */
	if (Blocks.Collide[block] != COLLIDE_SOLID) return true;

	IVec3_ToVec3(&pos, &Game_SelectedPos.TranslatedPos);
	if (IntersectsOthers(pos, block)) return false;
	
	nextPos = LocalPlayer_Instance.Base.next.pos;
	Vec3_Add(&blockBB.Min, &pos, &Blocks.MinBB[block]);
	Vec3_Add(&blockBB.Max, &pos, &Blocks.MaxBB[block]);

	/* NOTE: Need to also test against next position here, otherwise player can */
	/* fall through the block at feet as collision is performed against nextPos */
	Entity_GetBounds(p, &playerBB);
	playerBB.Min.Y = min(nextPos.Y, playerBB.Min.Y);

	if (hacks->Noclip || !AABB_Intersects(&playerBB, &blockBB)) return true;
	if (hacks->CanPushbackBlocks && hacks->PushbackPlacing && hacks->Enabled) {
		return PushbackPlace(&blockBB);
	}

	playerBB.Min.Y += 0.25f + ENTITY_ADJUSTMENT;
	if (AABB_Intersects(&playerBB, &blockBB)) return false;

	/* Push player upwards when they are jumping and trying to place a block underneath them */
	nextPos.Y = pos.Y + Blocks.MaxBB[block].Y + ENTITY_ADJUSTMENT;

	update.flags = LU_HAS_POS | LU_POS_ABSOLUTE_INSTANT;
	update.pos   = nextPos;
	p->VTABLE->SetLocation(p, &update);
	return true;
}

void InputHandler_DeleteBlock(void) {
	IVec3 pos;
	BlockID old;
	/* always play delete animations, even if we aren't deleting a block */
	HeldBlockRenderer_ClickAnim(true);

	pos = Game_SelectedPos.pos;
	if (!Game_SelectedPos.Valid || !World_Contains(pos.X, pos.Y, pos.Z)) return;

	old = World_GetBlock(pos.X, pos.Y, pos.Z);
	if (Blocks.Draw[old] == DRAW_GAS || !Blocks.CanDelete[old]) return;

	Game_ChangeBlock(pos.X, pos.Y, pos.Z, BLOCK_AIR);
	Event_RaiseBlock(&UserEvents.BlockChanged, pos, old, BLOCK_AIR);
}

void InputHandler_PlaceBlock(void) {
	IVec3 pos;
	BlockID old, block;
	pos = Game_SelectedPos.TranslatedPos;
	if (!Game_SelectedPos.Valid || !World_Contains(pos.X, pos.Y, pos.Z)) return;

	old   = World_GetBlock(pos.X, pos.Y, pos.Z);
	block = Inventory_SelectedBlock;
	if (AutoRotate_Enabled) block = AutoRotate_RotateBlock(block);

	if (Game_CanPick(old) || !Blocks.CanPlace[block]) return;
	/* air-ish blocks can only replace over other air-ish blocks */
	if (Blocks.Draw[block] == DRAW_GAS && Blocks.Draw[old] != DRAW_GAS) return;

	/* undeletable gas blocks can't be replaced with other blocks */
	if (Blocks.Collide[old] == COLLIDE_NONE && !Blocks.CanDelete[old]) return;

	if (!CheckIsFree(block)) return;

	Game_ChangeBlock(pos.X, pos.Y, pos.Z, block);
	Event_RaiseBlock(&UserEvents.BlockChanged, pos, old, block);
}

void InputHandler_PickBlock(void) {
	IVec3 pos;
	BlockID cur;
	pos = Game_SelectedPos.pos;
	if (!World_Contains(pos.X, pos.Y, pos.Z)) return;

	cur = World_GetBlock(pos.X, pos.Y, pos.Z);
	if (Blocks.Draw[cur] == DRAW_GAS) return;
	if (!(Blocks.CanPlace[cur] || Blocks.CanDelete[cur])) return;
	Inventory_PickBlock(cur);
}

void InputHandler_Tick(void) {
	cc_bool left, middle, right;
	TimeMS now;
	int delta;
	
	if (Gui.InputGrab) return;

	
	now   = DateTime_CurrentUTC_MS();
	delta = (int)(now - input_lastClick);
	if (delta < 250) return; /* 4 times per second */
	input_lastClick = now;

	left   = KeyBind_IsPressed(KEYBIND_DELETE_BLOCK);
	middle = KeyBind_IsPressed(KEYBIND_PICK_BLOCK);
	right  = KeyBind_IsPressed(KEYBIND_PLACE_BLOCK);
	
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode) {
		left   = (Input_HoldMode == INPUT_MODE_DELETE) && AnyBlockTouches();
		right  = (Input_HoldMode == INPUT_MODE_PLACE)  && AnyBlockTouches();
		middle = false;
	}
#endif

	if (Server.SupportsPlayerClick) {
		input_pickingId = -1;
		MouseStateChanged(MOUSE_LEFT,   left);
		MouseStateChanged(MOUSE_RIGHT,  right);
		MouseStateChanged(MOUSE_MIDDLE, middle);
	}

	if (left) {
		InputHandler_DeleteBlock();
	} else if (right) {
		InputHandler_PlaceBlock();
	} else if (middle) {
		InputHandler_PickBlock();
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Input helpers-------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool InputHandler_IsShutdown(int key) {
	if (key == CCKEY_F4 && Input_IsAltPressed()) return true;

	/* On macOS, Cmd+Q should also end the process */
#ifdef CC_BUILD_DARWIN
	return key == 'Q' && Input_IsWinPressed();
#else
	return false;
#endif
}

static void InputHandler_Toggle(int key, cc_bool* target, const char* enableMsg, const char* disableMsg) {
	*target = !(*target);
	if (*target) {
		Chat_Add2("%c. &ePress &a%c &eto disable.",   enableMsg,  Input_DisplayNames[key]);
	} else {
		Chat_Add2("%c. &ePress &a%c &eto re-enable.", disableMsg, Input_DisplayNames[key]);
	}
}

cc_bool InputHandler_SetFOV(int fov) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPerson) return false;

	Camera.ZoomFov = fov;
	Camera_SetFov(fov);
	return true;
}

cc_bool Input_HandleMouseWheel(float delta) {
	struct HacksComp* h;
	cc_bool hotbar;

	hotbar = Input_IsAltPressed() || Input_IsCtrlPressed() || Input_IsShiftPressed();
	if (!hotbar && Camera.Active->Zoom(delta))   return true;
	if (!KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL)) return false;

	h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPerson) return false;

	if (input_fovIndex == -1.0f) input_fovIndex = (float)Camera.ZoomFov;
	input_fovIndex -= delta * 5.0f;

	Math_Clamp(input_fovIndex, 1.0f, Camera.DefaultFov);
	return InputHandler_SetFOV((int)input_fovIndex);
}

static void InputHandler_CheckZoomFov(void* obj) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPerson) Camera_SetFov(Camera.DefaultFov);
}

static cc_bool HandleBlockKey(int key) {
	if (Gui.InputGrab) return false;

	if (KeyBind_Claims(KEYBIND_DELETE_BLOCK, key)) {
		MouseStatePress(MOUSE_LEFT);
		InputHandler_DeleteBlock();
	} else if (KeyBind_Claims(KEYBIND_PLACE_BLOCK, key)) {
		MouseStatePress(MOUSE_RIGHT);
		InputHandler_PlaceBlock();
	} else if (KeyBind_Claims(KEYBIND_PICK_BLOCK, key)) {
		MouseStatePress(MOUSE_MIDDLE);
		InputHandler_PickBlock();
	} else {
		return false;
	}
	return true;
}

static cc_bool HandleNonClassicKey(int key) {
	if (KeyBind_Claims(KEYBIND_HIDE_GUI, key)) {
		Game_HideGui = !Game_HideGui;
	} else if (KeyBind_Claims(KEYBIND_SMOOTH_CAMERA, key)) {
		InputHandler_Toggle(key, &Camera.Smooth,
			"  &eSmooth camera is &aenabled",
			"  &eSmooth camera is &cdisabled");
	} else if (KeyBind_Claims(KEYBIND_AXIS_LINES, key)) {
		InputHandler_Toggle(key, &AxisLinesRenderer_Enabled,
			"  &eAxis lines (&4X&e, &2Y&e, &1Z&e) now show",
			"  &eAxis lines no longer show");
	} else if (KeyBind_Claims(KEYBIND_AUTOROTATE, key)) {
		InputHandler_Toggle(key, &AutoRotate_Enabled,
			"  &eAuto rotate is &aenabled",
			"  &eAuto rotate is &cdisabled");
	} else if (KeyBind_Claims(KEYBIND_THIRD_PERSON, key)) {
		Camera_CycleActive();
	} else if (KeyBind_Claims(KEYBIND_DROP_BLOCK, key)) {
		if (Inventory_CheckChangeSelected() && Inventory_SelectedBlock != BLOCK_AIR) {
			/* Don't assign SelectedIndex directly, because we don't want held block
			switching positions if they already have air in their inventory hotbar. */
			Inventory_Set(Inventory.SelectedIndex, BLOCK_AIR);
			Event_RaiseVoid(&UserEvents.HeldBlockChanged);
		}
	} else if (KeyBind_Claims(KEYBIND_IDOVERLAY, key)) {
		TexIdsOverlay_Show();
	} else if (KeyBind_Claims(KEYBIND_BREAK_LIQUIDS, key)) {
		InputHandler_Toggle(key, &Game_BreakableLiquids,
			"  &eBreakable liquids is &aenabled",
			"  &eBreakable liquids is &cdisabled");
	} else {
		return false;
	}
	return true;
}

static cc_bool HandleCoreKey(int key) {
	if (KeyBind_Claims(KEYBIND_HIDE_FPS, key)) {
		Gui.ShowFPS = !Gui.ShowFPS;
	} else if (KeyBind_Claims(KEYBIND_FULLSCREEN, key)) {
		Game_ToggleFullscreen();
	} else if (KeyBind_Claims(KEYBIND_FOG, key)) {
		Game_CycleViewDistance();
	} else if (key == CCKEY_F5 && Game_ClassicMode) {
		int weather = Env.Weather == WEATHER_SUNNY ? WEATHER_RAINY : WEATHER_SUNNY;
		Env_SetWeather(weather);
	} else {
		if (Game_ClassicMode) return false;
		return HandleNonClassicKey(key);
	}
	return true;
}

static void HandleHotkeyDown(int key) {
	struct HotkeyData* hkey;
	cc_string text;
	int i = Hotkeys_FindPartial(key);

	if (i == -1) return;
	hkey = &HotkeysList[i];
	text = StringsBuffer_UNSAFE_Get(&HotkeysText, hkey->textIndex);

	if (!(hkey->flags & HOTKEY_FLAG_STAYS_OPEN)) {
		Chat_Send(&text, false);
	} else if (!Gui.InputGrab) {
		ChatScreen_OpenInput(&text);
	}
}

static cc_bool HandleLocalPlayerKey(int key) {
	if (KeyBind_Claims(KEYBIND_RESPAWN, key)) {
		return LocalPlayer_HandleRespawn();
	} else if (KeyBind_Claims(KEYBIND_SET_SPAWN, key)) {
		return LocalPlayer_HandleSetSpawn();
	} else if (KeyBind_Claims(KEYBIND_FLY, key)) {
		return LocalPlayer_HandleFly();
	} else if (KeyBind_Claims(KEYBIND_NOCLIP, key)) {
		return LocalPlayer_HandleNoclip();
	} else if (KeyBind_Claims(KEYBIND_JUMP, key)) {
		return LocalPlayer_HandleJump();
	}
	return false;
}


/*########################################################################################################################*
*-----------------------------------------------------Base handlers-------------------------------------------------------*
*#########################################################################################################################*/
static void OnMouseWheel(void* obj, float delta) {
	struct Screen* s;
	int i;
	
	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesMouseScroll(s, delta)) return;
	}
}

static void OnPointerMove(void* obj, int idx) {
	struct Screen* s;
	int i, x = Pointers[idx].x, y = Pointers[idx].y;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesPointerMove(s, 1 << idx, x, y)) return;
	}
}

static void OnPointerDown(void* obj, int idx) {
	struct Screen* s;
	int i, x, y, mask;
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode && !(touches[idx].type & TOUCH_TYPE_GUI)) return;
#endif
	x = Pointers[idx].x; y = Pointers[idx].y;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		mask = s->VTABLE->HandlesPointerDown(s, 1 << idx, x, y);

#ifdef CC_BUILD_TOUCH
		if (mask) {
			/* Using &= mask instead of = mask is to handle one specific case */
			/*  - when clicking 'Quit game' in android version, it will call  */
			/*  Game_Free, which will in turn call InputComponent.Free.       */
			/* That resets the type of all touches to 0 - however, since it is */
			/*  called DURING HandlesPointerDown, using = mask here would undo */
			/*  the resetting of type to 0 for one of the touches states,      */
			/*  causing problems later with Input_AddTouch as it will assume that */
			/*  the aforementioned touches state is wrongly still in use */
			touches[idx].type &= mask; return;
		}
#else
		if (mask) return;
#endif
	}
}

static void OnPointerUp(void* obj, int idx) {
	struct Screen* s;
	int i, x, y;
#ifdef CC_BUILD_TOUCH
	CheckBlockTap(idx);
	if (Input_TouchMode && !(touches[idx].type & TOUCH_TYPE_GUI)) return;
#endif
	x = Pointers[idx].x; y = Pointers[idx].y;

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		s->VTABLE->OnPointerUp(s, 1 << idx, x, y);
	}
}

static void OnInputDown(void* obj, int key, cc_bool was) {
	struct Screen* s;
	int i;

#ifndef CC_BUILD_WEB
	if (Input_IsEscapeButton(key) && (s = Gui_GetClosable())) {
		/* Don't want holding down escape to go in and out of pause menu */
		if (!was) Gui_Remove(s);
		return;
	}
#endif

	if (InputHandler_IsShutdown(key)) {
		/* TODO: Do we need a separate exit function in Game class? */
		Window_Close(); return;
	} else if (KeyBind_Claims(KEYBIND_SCREENSHOT, key) && !was) {
		Game_ScreenshotRequested = true; return;
	}
	
	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesInputDown(s, key)) return;
	}

	if (Input_IsPauseButton(key) && !Gui.InputGrab) {
#ifdef CC_BUILD_WEB
		/* Can't do this in KeyUp, because pressing escape without having */
		/* explicitly disabled mouse lock means a KeyUp event isn't sent. */
		/* But switching to pause screen disables mouse lock, causing a KeyUp */
		/* event to be sent, triggering the active->closable case which immediately */
		/* closes the pause screen. Hence why the next KeyUp must be supressed. */
		suppressEscape = true;
#endif
		Gui_ShowPauseMenu(); return;
	}

	/* These should not be triggered multiple times when holding down */
	if (was) return;
	if (HandleBlockKey(key)) {
	} else if (HandleCoreKey(key)) {
	} else if (HandleLocalPlayerKey(key)) {
	} else { HandleHotkeyDown(key); }
}

static void OnInputUp(void* obj, int key) {
	struct Screen* s;
	int i;

	if (KeyBind_Claims(KEYBIND_ZOOM_SCROLL, key)) Camera_SetFov(Camera.DefaultFov);
#ifdef CC_BUILD_WEB
	/* When closing menus (which reacquires mouse focus) in key down, */
	/* this still leaves the cursor visible. But if this is instead */
	/* done in key up, the cursor disappears as expected. */
	if (key == CCKEY_ESCAPE && (s = Gui_GetClosable())) {
		if (suppressEscape) { suppressEscape = false; return; }
		Gui_Remove(s); return;
	}
#endif

	for (i = 0; i < Gui.ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		s->VTABLE->OnInputUp(s, key);
	}

	if (Gui.InputGrab) return;
	if (KeyBind_Claims(KEYBIND_DELETE_BLOCK, key)) MouseStateRelease(MOUSE_LEFT);
	if (KeyBind_Claims(KEYBIND_PLACE_BLOCK,  key)) MouseStateRelease(MOUSE_RIGHT);
	if (KeyBind_Claims(KEYBIND_PICK_BLOCK,   key)) MouseStateRelease(MOUSE_MIDDLE);
}

static void OnFocusChanged(void* obj) { if (!WindowInfo.Focused) Input_Clear(); }
static void OnInit(void) {
	Event_Register_(&PointerEvents.Moved, NULL, OnPointerMove);
	Event_Register_(&PointerEvents.Down,  NULL, OnPointerDown);
	Event_Register_(&PointerEvents.Up,    NULL, OnPointerUp);
	Event_Register_(&InputEvents.Down,    NULL, OnInputDown);
	Event_Register_(&InputEvents.Up,      NULL, OnInputUp);
	Event_Register_(&InputEvents.Wheel,   NULL, OnMouseWheel);

	Event_Register_(&WindowEvents.FocusChanged,   NULL, OnFocusChanged);
	Event_Register_(&UserEvents.HackPermsChanged, NULL, InputHandler_CheckZoomFov);
	KeyBind_Init();
	StoredHotkeys_LoadAll();
	/* Fix issue with Android where if you double click in server list to join, a touch */
	/*  pointer is stuck down when the game loads (so you instantly start deleting blocks) */
	ClearTouches();
}

static void OnFree(void) {
	ClearTouches();
	HotkeysText.count = 0;
}

struct IGameComponent Input_Component = {
	OnInit, /* Init  */
	OnFree, /* Free  */
};
