#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"

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