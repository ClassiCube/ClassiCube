#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Utils.h"
#include "Logger.h"
#include "Platform.h"
#include "Chat.h"
#include "Utils.h"
#include "Server.h"
#include "HeldBlockRenderer.h"
#include "Game.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Camera.h"
#include "Inventory.h"
#include "World.h"
#include "Event.h"
#include "Window.h"
#include "Entity.h"
#include "Chat.h"
#include "Funcs.h"
#include "Screens.h"
#include "Block.h"
#include "Menus.h"
#include "Gui.h"
#include "Protocol.h"
#include "AxisLinesRenderer.h"

static cc_bool input_buttonsDown[3];
static int input_pickingId = -1;
static const short normDists[10]   = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
static const short classicDists[4] = { 8, 32, 128, 512 };
static TimeMS input_lastClick;
static float input_fovIndex = -1.0f;
#ifdef CC_BUILD_WEB
static cc_bool suppressEscape;
#endif
enum MouseButton_ { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE };


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
cc_bool Input_Placing;

/* Touch fingers are initially are all type, meaning they could */
/* trigger menu clicks, camera movement, or place/delete blocks */
/* But for example, after clicking on a menu button, you wouldn't */
/* want moving that finger anymore to move the camera */
#define TOUCH_TYPE_GUI    1
#define TOUCH_TYPE_CAMERA 2
#define TOUCH_TYPE_BLOCKS 4
#define TOUCH_TYPE_ALL (TOUCH_TYPE_GUI | TOUCH_TYPE_CAMERA | TOUCH_TYPE_BLOCKS)

static void DoDeleteBlock(void);
static void DoPlaceBlock(void);
static void MouseStatePress(int button);
static void MouseStateRelease(int button);

static cc_bool AnyBlockTouches(void) {
	int i;
	for (i = 0; i < Pointers_Count; i++) {
		if (touches[i].type & TOUCH_TYPE_BLOCKS) return true;
	}
	return false;
}

void Input_AddTouch(long id, int x, int y) {
	int i;
	for (i = 0; i < INPUT_MAX_POINTERS; i++) {
		if (touches[i].type) continue;

		touches[i].id   = id;
		touches[i].type = TOUCH_TYPE_ALL;
		touches[i].begX = x;
		touches[i].begY = y;

		touches[i].start = DateTime_CurrentUTC_MS();
		/* Also set last click time, otherwise quickly tapping */
		/* sometimes triggers a 'delete' in InputHandler_PickBlocks, */
		/* and then another 'delete' in CheckBlockTap. */
		input_lastClick  = touches[i].start;

		if (i == Pointers_Count) Pointers_Count++;
		Pointer_SetPosition(i, x, y);
		Pointer_SetPressed(i, true);
		return;
	}
}

static cc_bool MovedFromBeg(int i, int x, int y) {
	return Math_AbsI(x - touches[i].begX) > Display_ScaleX(5) ||
		   Math_AbsI(y - touches[i].begY) > Display_ScaleY(5);
}

void Input_UpdateTouch(long id, int x, int y) {
	int i;
	for (i = 0; i < Pointers_Count; i++) {
		if (touches[i].id != id || !touches[i].type) continue;
		
		if (Input_RawMode && (touches[i].type & TOUCH_TYPE_CAMERA)) {
			/* If the pointer hasn't been locked to gui or block yet, moving a bit */
			/* should cause the pointer to get locked to camera movement. */
			if (touches[i].type == TOUCH_TYPE_ALL && MovedFromBeg(i, x, y)) {
				/* Allow a little bit of leeway because though, because devices */
				/* might still report a few pixels of movement depending on how */
				/* user is holding the finger down on the touch surface */
				touches[i].type = TOUCH_TYPE_CAMERA;
			}
			Event_RaiseMove(&PointerEvents.RawMoved, i, x - Pointers[i].x, y - Pointers[i].y);
		}
		Pointer_SetPosition(i, x, y);
		return;
	}
}

/* Quickly tapping should trigger a block place/delete */
static void CheckBlockTap(int i) {
	int btn, pressed;
	if (DateTime_CurrentUTC_MS() > touches[i].start + 250) return;
	if (touches[i].type != TOUCH_TYPE_ALL) return;

	btn = Input_Placing ? MOUSE_RIGHT : MOUSE_LEFT;
	pressed = input_buttonsDown[btn];
	MouseStatePress(btn);

	if (btn == MOUSE_LEFT) { DoDeleteBlock(); }
	else { DoPlaceBlock(); }

	if (!pressed) MouseStateRelease(btn);
}

void Input_RemoveTouch(long id, int x, int y) {
	int i;
	for (i = 0; i < Pointers_Count; i++) {
		if (touches[i].id != id || !touches[i].type) continue;

		Pointer_SetPosition(i, x, y);
		Pointer_SetPressed(i, false);
		CheckBlockTap(i);

		/* found the touch, remove it */
		Pointer_SetPosition(i, -100000, -100000);
		touches[i].type = 0;

		if ((i + 1) == Pointers_Count) Pointers_Count--;
		return;
	}
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------------Key-----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Input_Pressed[INPUT_COUNT];

#define Key_Function_Names \
"F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",  "F8",  "F9",  "F10",\
"F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20",\
"F21", "F22", "F23", "F24", "F25", "F26", "F27", "F28", "F29", "F30",\
"F31", "F32", "F33", "F34", "F35"
#define Key_Ascii_Names \
"A", "B", "C", "D", "E", "F", "G", "H", "I", "J",\
"K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",\
"U", "V", "W", "X", "Y", "Z"

const char* const Input_Names[INPUT_COUNT] = {
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
	"XButton1", "XButton2", "LeftMouse", "RightMouse", "MiddleMouse"
};

/* TODO: Should this only be shown in GUI? not saved to disc? */
/*const char* Input_Names[INPUT_COUNT] = {
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
	"XBUTTON1", "XBUTTON2", "MMOUSE"
};*/

void Input_SetPressed(int key, cc_bool pressed) {
	cc_bool wasPressed = Input_Pressed[key];
	Input_Pressed[key] = pressed;

	if (pressed) {
		Event_RaiseInput(&InputEvents.Down, key, wasPressed);
	} else if (wasPressed) {
		Event_RaiseInt(&InputEvents.Up, key);
	}

	/* don't allow multiple left mouse down events */
	if (key != KEY_LMOUSE || pressed == wasPressed) return;
	Pointer_SetPressed(0, pressed);
}

void Input_Clear(void) {
	int i;
	for (i = 0; i < INPUT_COUNT; i++) {
		if (Input_Pressed[i]) Input_SetPressed(i, false);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------Mouse----------------------------------------------------------*
*#########################################################################################################################*/
int Mouse_X, Mouse_Y;
struct Pointer Pointers[INPUT_MAX_POINTERS];
cc_bool Input_RawMode, Input_TouchMode;

void Pointer_SetPressed(int idx, cc_bool pressed) {
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode && !(touches[idx].type & TOUCH_TYPE_GUI)) return;
#endif

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
	int deltaX = x - Mouse_X, deltaY = y - Mouse_Y;
	Mouse_X = x; Mouse_Y = y;
	if (x == Pointers[idx].x && y == Pointers[idx].y) return;
	/* TODO: reset to -1, -1 when pointer is removed */
	Pointers[idx].x = x; Pointers[idx].y = y;
	
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode && !(touches[idx].type & TOUCH_TYPE_GUI)) return;
#endif
	Event_RaiseMove(&PointerEvents.Moved, idx, deltaX, deltaY);
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
	KEY_LCTRL, KEY_LMOUSE, KEY_MMOUSE, KEY_RMOUSE, 
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
	"HalfSpeed", "DeleteBlock", "PickBlock", "PlaceBlock", 
	"AutoRotate", "HotbarSwitching", "SmoothCamera", 
	"DropBlock", "IDOverlay", "BreakableLiquids"
};

cc_bool KeyBind_IsPressed(KeyBind binding) { return Input_Pressed[KeyBinds[binding]]; }

static void KeyBind_Load(void) {
	String name; char nameBuffer[STRING_SIZE + 1];
	int mapping;
	int i;

	String_InitArray_NT(name, nameBuffer);
	for (i = 0; i < KEYBIND_COUNT; i++) {
		name.length = 0;
		String_Format1(&name, "key-%c", keybindNames[i]);
		name.buffer[name.length] = '\0';

		mapping = Options_GetEnum(name.buffer, KeyBind_Defaults[i], Input_Names, INPUT_COUNT);
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

		value = String_FromReadonly(Input_Names[KeyBinds[i]]);
		Options_SetString(&name, &value);
	}
}

void KeyBind_Set(KeyBind binding, int key) {
	KeyBinds[binding] = key;
	KeyBind_Save();
}

/* Initialises and loads key bindings from options */
static void KeyBind_Init(void) {
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

static void Hotkeys_AddNewHotkey(int trigger, cc_uint8 modifiers, const String* text, cc_bool more) {
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


void Hotkeys_Add(int trigger, cc_uint8 modifiers, const String* text, cc_bool more) {
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

cc_bool Hotkeys_Remove(int trigger, cc_uint8 modifiers) {
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

int Hotkeys_FindPartial(int key) {
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

/* Initialises and loads hotkeys from options. */
static void Hotkeys_Init(void) {
	static const String prefix = String_FromConst("hotkey-");
	String strKey, strMods, strMore, strText;
	String entry, key, value;
	int i;

	int trigger;
	cc_uint8 modifiers;
	cc_bool more;

	for (i = 0; i < Options.entries.count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&Options.entries, i);
		String_UNSAFE_Separate(&entry, Options.separator, &key, &value);

		if (!String_CaselessStarts(&key, &prefix)) continue;
		/* Format is: key&modifiers = more-input&text */
		key.length -= prefix.length; key.buffer += prefix.length;
	
		if (!String_UNSAFE_Separate(&key,   '&', &strKey,  &strMods)) continue;
		if (!String_UNSAFE_Separate(&value, '&', &strMore, &strText)) continue;

		trigger = Utils_ParseEnum(&strKey, KEY_NONE, Input_Names, INPUT_COUNT);
		if (trigger == KEY_NONE) continue; 
		if (!Convert_ParseUInt8(&strMods, &modifiers)) continue;
		if (!Convert_ParseBool(&strMore,  &more))      continue;

		Hotkeys_Add(trigger, modifiers, &strText, more);
	}
}

void Hotkeys_UserRemovedHotkey(int trigger, cc_uint8 modifiers) {
	String key; char keyBuffer[STRING_SIZE];
	String_InitArray(key, keyBuffer);

	String_Format2(&key, "hotkey-%c&%b", Input_Names[trigger], &modifiers);
	Options_SetString(&key, NULL);
}

void Hotkeys_UserAddedHotkey(int trigger, cc_uint8 modifiers, cc_bool moreInput, const String* text) {
	String key;   char keyBuffer[STRING_SIZE];
	String value; char valueBuffer[STRING_SIZE * 2];
	String_InitArray(key, keyBuffer);
	String_InitArray(value, valueBuffer);

	String_Format2(&key, "hotkey-%c&%b", Input_Names[trigger], &modifiers);
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

	LocationUpdate_MakePos(&update, pos, false);
	p->VTABLE->SetLocation(p, &update, false);
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
	
	nextPos = LocalPlayer_Instance.Interp.Next.Pos;
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
	LocationUpdate_MakePos(&update, nextPos, false);
	p->VTABLE->SetLocation(p, &update, false);
	return true;
}

static void DoDeleteBlock(void) {
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

static void DoPlaceBlock(void) {
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
	if (!CheckIsFree(block)) return;

	Game_ChangeBlock(pos.X, pos.Y, pos.Z, block);
	Event_RaiseBlock(&UserEvents.BlockChanged, pos, old, block);
}

static void DoPickBlock(void) {
	IVec3 pos;
	BlockID cur;
	pos = Game_SelectedPos.pos;
	if (!World_Contains(pos.X, pos.Y, pos.Z)) return;

	cur = World_GetBlock(pos.X, pos.Y, pos.Z);
	if (Blocks.Draw[cur] == DRAW_GAS) return;
	if (!(Blocks.CanPlace[cur] || Blocks.CanDelete[cur])) return;
	Inventory_PickBlock(cur);
}

void InputHandler_PickBlocks(void) {
	cc_bool left, middle, right;
	TimeMS now = DateTime_CurrentUTC_MS();
	int delta  = (int)(now - input_lastClick);

	if (delta < 250) return; /* 4 times per second */
	input_lastClick = now;
	if (Gui_GetInputGrab()) return;

	left   = KeyBind_IsPressed(KEYBIND_DELETE_BLOCK);
	middle = KeyBind_IsPressed(KEYBIND_PICK_BLOCK);
	right  = KeyBind_IsPressed(KEYBIND_PLACE_BLOCK);
	
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode) {
		left   = !Input_Placing && AnyBlockTouches();
		right  = Input_Placing  && AnyBlockTouches();
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
		DoDeleteBlock();
	} else if (right) {
		DoPlaceBlock();
	} else if (middle) {
		DoPickBlock();
	}
}


/*########################################################################################################################*
*------------------------------------------------------Key helpers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool InputHandler_IsShutdown(int key) {
	if (key == KEY_F4 && Key_IsAltPressed()) return true;

	/* On OSX, Cmd+Q should also terminate the process */
#ifdef CC_BUILD_OSX
	return key == 'Q' && Key_IsWinPressed();
#else
	return false;
#endif
}

static void InputHandler_Toggle(int key, cc_bool* target, const char* enableMsg, const char* disableMsg) {
	*target = !(*target);
	if (*target) {
		Chat_Add2("%c. &ePress &a%c &eto disable.",   enableMsg,  Input_Names[key]);
	} else {
		Chat_Add2("%c. &ePress &a%c &eto re-enable.", disableMsg, Input_Names[key]);
	}
}

static void CycleViewDistanceForwards(const short* viewDists, int count) {
	int i, dist;
	for (i = 0; i < count; i++) {
		dist = viewDists[i];

		if (dist > Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[0]);
}

static void CycleViewDistanceBackwards(const short* viewDists, int count) {
	int i, dist;
	for (i = count - 1; i >= 0; i--) {
		dist = viewDists[i];

		if (dist < Game_UserViewDistance) {
			Game_UserSetViewDistance(dist); return;
		}
	}
	Game_UserSetViewDistance(viewDists[count - 1]);
}

cc_bool InputHandler_SetFOV(int fov) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPerson) return false;

	Game_ZoomFov = fov;
	Game_SetFov(fov);
	return true;
}

static cc_bool InputHandler_DoFovZoom(float deltaPrecise) {
	struct HacksComp* h;
	if (!KeyBind_IsPressed(KEYBIND_ZOOM_SCROLL)) return false;

	h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPerson) return false;

	if (input_fovIndex == -1.0f) input_fovIndex = (float)Game_ZoomFov;
	input_fovIndex -= deltaPrecise * 5.0f;

	Math_Clamp(input_fovIndex, 1.0f, Game_DefaultFov);
	return InputHandler_SetFOV((int)input_fovIndex);
}

static void InputHandler_CheckZoomFov(void* obj) {
	struct HacksComp* h = &LocalPlayer_Instance.Hacks;
	if (!h->Enabled || !h->CanUseThirdPerson) Game_SetFov(Game_DefaultFov);
}

static cc_bool HandleBlockKey(int key) {
	if (Gui_GetInputGrab()) return false;

	if (key == KeyBinds[KEYBIND_DELETE_BLOCK]) {
		MouseStatePress(MOUSE_LEFT);
		DoDeleteBlock();
	} else if (key == KeyBinds[KEYBIND_PLACE_BLOCK]) {
		MouseStatePress(MOUSE_RIGHT);
		DoPlaceBlock();
	} else if (key == KeyBinds[KEYBIND_PICK_BLOCK]) {
		MouseStatePress(MOUSE_MIDDLE);
		DoPickBlock();
	} else {
		return false;
	}
	return true;
}

static cc_bool HandleNonClassicKey(int key) {
	if (key == KeyBinds[KEYBIND_HIDE_GUI]) {
		Game_HideGui = !Game_HideGui;
	} else if (key == KeyBinds[KEYBIND_SMOOTH_CAMERA]) {
		InputHandler_Toggle(key, &Camera.Smooth,
			"  &eSmooth camera is &aenabled",
			"  &eSmooth camera is &cdisabled");
	} else if (key == KeyBinds[KEYBIND_AXIS_LINES]) {
		InputHandler_Toggle(key, &AxisLinesRenderer_Enabled,
			"  &eAxis lines (&4X&e, &2Y&e, &1Z&e) now show",
			"  &eAxis lines no longer show");
	} else if (key == KeyBinds[KEYBIND_AUTOROTATE]) {
		InputHandler_Toggle(key, &AutoRotate_Enabled,
			"  &eAuto rotate is &aenabled",
			"  &eAuto rotate is &cdisabled");
	} else if (key == KeyBinds[KEYBIND_THIRD_PERSON]) {
		Camera_CycleActive();
	} else if (key == KeyBinds[KEYBIND_DROP_BLOCK]) {
		if (Inventory_CheckChangeSelected() && Inventory_SelectedBlock != BLOCK_AIR) {
			/* Don't assign SelectedIndex directly, because we don't want held block
			switching positions if they already have air in their inventory hotbar. */
			Inventory_Set(Inventory.SelectedIndex, BLOCK_AIR);
			Event_RaiseVoid(&UserEvents.HeldBlockChanged);
		}
	} else if (key == KeyBinds[KEYBIND_IDOVERLAY]) {
		TexIdsOverlay_Show();
	} else if (key == KeyBinds[KEYBIND_BREAK_LIQUIDS]) {
		InputHandler_Toggle(key, &Game_BreakableLiquids,
			"  &eBreakable liquids is &aenabled",
			"  &eBreakable liquids is &cdisabled");
	} else {
		return false;
	}
	return true;
}

static cc_bool HandleCoreKey(int key) {
	cc_result res;

	if (key == KeyBinds[KEYBIND_HIDE_FPS]) {
		Gui_ShowFPS = !Gui_ShowFPS;
	} else if (key == KeyBinds[KEYBIND_FULLSCREEN]) {
		int state = Window_GetWindowState();

		if (state == WINDOW_STATE_FULLSCREEN) {
			res = Window_ExitFullscreen();
			if (res) Logger_Warn(res, "leaving fullscreen");
		} else {
			res = Window_EnterFullscreen();
			if (res) Logger_Warn(res, "going fullscreen");
		}
	} else if (key == KeyBinds[KEYBIND_FOG]) {
		const short* viewDists = Gui_ClassicMenu ? classicDists : normDists;
		int count = Gui_ClassicMenu ? Array_Elems(classicDists) : Array_Elems(normDists);

		if (Key_IsShiftPressed()) {
			CycleViewDistanceBackwards(viewDists, count);
		} else {
			CycleViewDistanceForwards(viewDists, count);
		}
	} else if (key == KEY_F5 && Game_ClassicMode) {
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
	String text;
	int i = Hotkeys_FindPartial(key);

	if (i == -1) return;
	hkey = &HotkeysList[i];
	text = StringsBuffer_UNSAFE_Get(&HotkeysText, hkey->TextIndex);

	if (!hkey->StaysOpen) {
		Chat_Send(&text, false);
	} else if (!Gui_GetInputGrab()) {
		ChatScreen_OpenInput(&text);
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Base handlers-------------------------------------------------------*
*#########################################################################################################################*/
static void HandleMouseWheel(void* obj, float delta) {
	struct Screen* s;
	int i;
	struct Widget* widget;
	cc_bool hotbar;
	
	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesMouseScroll(s, delta)) return;
	}

	hotbar = Key_IsAltPressed() || Key_IsControlPressed() || Key_IsShiftPressed();
	if (!hotbar && Camera.Active->Zoom(delta)) return;
	if (InputHandler_DoFovZoom(delta) || !Inventory.CanChangeSelected) return;

	widget = HUDScreen_GetHotbar();
	Elem_HandlesMouseScroll(widget, delta);
	((struct Screen*)Gui_Chat)->dirty = true;
}

static void HandlePointerMove(void* obj, int idx, int xDelta, int yDelta) {
	struct Screen* s;
	int i, x = Pointers[idx].x, y = Pointers[idx].y;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesPointerMove(s, 1 << idx, x, y)) return;
	}
}

static void HandlePointerDown(void* obj, int idx) {
	struct Screen* s;
	int i, x = Pointers[idx].x, y = Pointers[idx].y;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
#ifdef CC_BUILD_TOUCH
		if (s->VTABLE->HandlesPointerDown(s, 1 << idx, x, y)) {
			touches[idx].type = TOUCH_TYPE_GUI; return;
		}
#else
		if (s->VTABLE->HandlesPointerDown(s, 1 << idx, x, y)) return;
#endif
	}
}

static void HandlePointerUp(void* obj, int idx) {
	struct Screen* s;
	int i, x = Pointers[idx].x, y = Pointers[idx].y;

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesPointerUp(s, 1 << idx, x, y)) return;
	}
}

static void HandleInputDown(void* obj, int key, cc_bool was) {
	struct Screen* s;
	int i;

#ifndef CC_BUILD_WEB
	if (key == KEY_ESCAPE && (s = Gui_GetClosable())) {
		/* Don't want holding down escape to go in and out of pause menu */
		if (!was) Gui_Remove(s);
		return;
	}
#endif

	if (InputHandler_IsShutdown(key)) {
		/* TODO: Do we need a separate exit function in Game class? */
		Window_Close(); return;
	} else if (key == KeyBinds[KEYBIND_SCREENSHOT] && !was) {
		Game_ScreenshotRequested = true; return;
	}
	
	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesInputDown(s, key)) return;
	}

	if ((key == KEY_ESCAPE || key == KEY_PAUSE) && !Gui_GetInputGrab()) {
#ifdef CC_BUILD_WEB
		/* Can't do this in KeyUp, because pressing escape without having */
		/* explicitly disabled mouse lock means a KeyUp event isn't sent. */
		/* But switching to pause screen disables mouse lock, causing a KeyUp */
		/* event to be sent, triggering the active->closable case which immediately */
		/* closes the pause screen. Hence why the next KeyUp must be supressed. */
		suppressEscape = true;
#endif
		PauseScreen_Show(); return;
	}

	/* These should not be triggered multiple times when holding down */
	if (was) return;
	if (HandleBlockKey(key)) {
	} else if (HandleCoreKey(key)) {
	} else if (LocalPlayer_HandlesKey(key)) {
	} else { HandleHotkeyDown(key); }
}

static void HandleInputUp(void* obj, int key) {
	struct Screen* s;
	int i;

	if (key == KeyBinds[KEYBIND_ZOOM_SCROLL]) Game_SetFov(Game_DefaultFov);
#ifdef CC_BUILD_WEB
	/* When closing menus (which reacquires mouse focus) in key down, */
	/* this still leaves the cursor visible. But if this is instead */
	/* done in key up, the cursor disappears as expected. */
	if (key == KEY_ESCAPE && (s = Gui_GetClosable())) {
		if (suppressEscape) { suppressEscape = false; return; }
		Gui_Remove(s); return;
	}
#endif

	for (i = 0; i < Gui_ScreensCount; i++) {
		s = Gui_Screens[i];
		s->dirty = true;
		if (s->VTABLE->HandlesInputUp(s, key)) return;
	}

	if (Gui_GetInputGrab()) return;
	if (key == KeyBinds[KEYBIND_DELETE_BLOCK]) MouseStateRelease(MOUSE_LEFT);
	if (key == KeyBinds[KEYBIND_PLACE_BLOCK])  MouseStateRelease(MOUSE_RIGHT);
	if (key == KeyBinds[KEYBIND_PICK_BLOCK])   MouseStateRelease(MOUSE_MIDDLE);
}

static void HandleFocusChanged(void* obj) { if (!Window_Focused) Input_Clear(); }
void InputHandler_Init(void) {
	Event_RegisterMove(&PointerEvents.Moved, NULL, HandlePointerMove);
	Event_RegisterInt(&PointerEvents.Down,   NULL, HandlePointerDown);
	Event_RegisterInt(&PointerEvents.Up,     NULL, HandlePointerUp);
	Event_RegisterInt(&InputEvents.Down,     NULL, HandleInputDown);
	Event_RegisterInt(&InputEvents.Up,       NULL, HandleInputUp);
	Event_RegisterFloat(&InputEvents.Wheel,  NULL, HandleMouseWheel);

	Event_RegisterVoid(&WindowEvents.FocusChanged,         NULL, HandleFocusChanged);
	Event_RegisterVoid(&UserEvents.HackPermissionsChanged, NULL, InputHandler_CheckZoomFov);
	KeyBind_Init();
	Hotkeys_Init();
}
