#include "Input.h"
#include "String.h"
#include "Event.h"
#include "Funcs.h"
#include "Options.h"
#include "Logger.h"
#include "Platform.h"
#include "Utils.h"
#include "Game.h"
#include "ExtMath.h"
#include "Camera.h"
#include "Inventory.h"
#include "World.h"
#include "Event.h"
#include "Window.h"
#include "Screens.h"
#include "Block.h"

struct _InputState Input;
/* Raises PointerEvents.Up or PointerEvents.Down */
static void Pointer_SetPressed(int idx, cc_bool pressed);


/*########################################################################################################################*
*------------------------------------------------------Touch support------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_TOUCH
struct TouchPointer touches[INPUT_MAX_POINTERS];

int Pointers_Count;
int Input_TapMode  = INPUT_MODE_PLACE;
int Input_HoldMode = INPUT_MODE_DELETE;
cc_bool Input_TouchMode;

static void MouseStatePress(int button);
static void MouseStateRelease(int button);

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
	for (i = 0; i < Pointers_Count; i++) 
	{
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

	for (i = 0; i < INPUT_MAX_POINTERS; i++) 
	{
		if (touches[i].type) continue;

		touches[i].id    = id;
		touches[i].type  = TOUCH_TYPE_ALL;
		touches[i].begX  = x;
		touches[i].begY  = y;
		touches[i].start = Game.Time;

		if (i == Pointers_Count) Pointers_Count++;
		Pointer_SetPosition(i, x, y);
		Pointer_SetPressed(i, true);
		return;
	}
}
void Input_UpdateTouch(long id, int x, int y) { TryUpdateTouch(id, x, y); }

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
"PAD_Z", "PAD_C", "PAD_D", \
"PAD_LEFT", "PAD_RIGHT", "PAD_UP", "PAD_DOWN", \
"PAD_START", "PAD_SELECT", "PAD_ZL", "PAD_ZR", \
"PAD_LSTICK", "PAD_RSTICK", \
"PAD_CLEFT", "PAD_CRIGHT", "PAD_CUP", "PAD_CDOWN"

#define Pad_DisplayNames \
"A", "B", "X", "Y", "L", "R", \
"Z", "C", "D", \
"LEFT", "RIGHT", "UP", "DOWN", \
"START", "SELECT", "ZL", "ZR", \
"LSTICK", "RSTICK", \
"CLEFT", "CRIGHT", "CUP", "CDOWN"

/* Names for each input button when stored to disc */
const char* const Input_StorageNames[INPUT_COUNT] = {
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
	"WheelUp", "WheelDown", "WheelLeft", "WheelRight",
	"XButton3", "XButton4", "XButton5", "XButton6",
	
	"VolumeMute", "VolumeUp", "VolumeDown", "Sleep",
	"MediaNext", "MediaPrev", "MediaPlay", "MediaStop",
	"BrowserPrev", "BrowserNext", "BrowserRefresh", "BrowserStop", "BrowserSsearch", "BrowserFavorites", "BrowserHome",
	"LaunchMail", "LaunchMedia", "LaunchApp1", "LaunchCalc", 

	Pad_Names
};

const char* Input_DisplayNames[INPUT_COUNT] = {
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
	"WHEELUP", "WHEELDOWN", "WHEELLEFT", "WHEELRIGHT",
	"XBUTTON3", "XBUTTON4", "XBUTTON5", "XBUTTON6",
	
	"VOLUMEMUTE", "VOLUMEUP", "VOLUMEDOWN", "SLEEP",
	"MEDIANEXT", "MEDIAPREV", "MEDIAPLAY", "MEDIASTOP",
	"BROWSERPREV", "BROWSERNEXT", "BROWSERREFRESH", "BROWSERSTOP", "BROWSERSEARCH", "BROWSERFAVORITES", "BROWSERHOME",
	"LAUNCHMAIL", "LAUNCHMEDIA", "LAUNCHAPP1", "LAUNCHCALC", 

	Pad_DisplayNames
};

void Input_SetPressed(int key) {
	cc_bool wasPressed = Input.Pressed[key];
	Input.Pressed[key] = true;

	if (key <= CCMOUSE_M) Event_RaiseInput(&InputEvents._down, key, wasPressed, &NormDevice);
	Event_RaiseInput(&InputEvents.Down2, key, wasPressed, &NormDevice);

	if (key == 'C' && Input_IsActionPressed()) Event_RaiseInput(&InputEvents.Down2, INPUT_CLIPBOARD_COPY,  0, &NormDevice);
	if (key == 'V' && Input_IsActionPressed()) Event_RaiseInput(&InputEvents.Down2, INPUT_CLIPBOARD_PASTE, 0, &NormDevice);

	/* don't allow multiple left mouse down events */
	if (key != CCMOUSE_L || wasPressed) return;
	Pointer_SetPressed(0, true);
}

void Input_SetReleased(int key) {
	if (!Input.Pressed[key]) return;
	Input.Pressed[key] = false;

	if (key <= CCMOUSE_M) Event_RaiseInput(&InputEvents._up, key, true, &NormDevice);
	Event_RaiseInput(&InputEvents.Up2, key, true, &NormDevice);

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

void Input_CalcDelta(int key, struct InputDevice* device, int* horDelta, int* verDelta) {
	*horDelta = 0; *verDelta = 0;

	if (key == device->leftButton  || key == CCKEY_KP4) *horDelta = -1;
	if (key == device->rightButton || key == CCKEY_KP6) *horDelta = +1;
	if (key == device->upButton    || key == CCKEY_KP8) *verDelta = -1;
	if (key == device->downButton  || key == CCKEY_KP2) *verDelta = +1;
}

static cc_bool NormDevice_IsPressed(struct InputDevice* device, int key) { 
	return Input.Pressed[key]; 
}

static cc_bool PadDevice_IsPressed(struct InputDevice* device, int key) {
	struct GamepadDevice* gamepad = (struct GamepadDevice*)device;
	if (!Input_IsPadButton(key)) return false;
	
	return gamepad->pressed[key - GAMEPAD_BEG_BTN];
}

static cc_bool TouchDevice_IsPressed(struct InputDevice* device, int key) { 
	return false; 
}

struct InputDevice NormDevice = {
	INPUT_DEVICE_NORMAL, 0, 0,
	NormDevice_IsPressed,
	/* General buttons */
	CCKEY_UP, CCKEY_DOWN, CCKEY_LEFT, CCKEY_RIGHT,
	CCKEY_ENTER,  CCKEY_KP_ENTER,
	CCKEY_ESCAPE, CCKEY_PAUSE,
	CCKEY_ESCAPE,
	CCKEY_PAGEUP, CCKEY_PAGEDOWN,
	/* Launcher buttons */
	CCKEY_TAB,
	/* Bindings */
	"key-%c", KeyBind_Defaults, KeyBind_Mappings
};
static const struct InputDevice padDevice = {
	INPUT_DEVICE_GAMEPAD, 0, 0,
	PadDevice_IsPressed,
	/* General buttons */
	CCPAD_UP, CCPAD_DOWN, CCPAD_LEFT, CCPAD_RIGHT,
	CCPAD_START, CCPAD_1,
	CCPAD_START, 0,
	CCPAD_SELECT,
	0, 0,
	/* Launcher buttons */
	CCPAD_3,
	/* Bindings */
	"pad-%c", NULL, NULL
};
struct InputDevice TouchDevice = {
	INPUT_DEVICE_TOUCH, 0, 0,
	TouchDevice_IsPressed
};


/*########################################################################################################################*
*----------------------------------------------------------Mouse----------------------------------------------------------*
*#########################################################################################################################*/
struct Pointer Pointers[INPUT_MAX_POINTERS];
struct _PointerHooks PointerHooks;

void Pointer_SetPressed(int idx, cc_bool pressed) {
	if (pressed) {
		if (PointerHooks.DownHook && PointerHooks.DownHook(idx)) return;

		Event_RaiseInt(&PointerEvents.Down, idx);
	} else {
		if (PointerHooks.UpHook && PointerHooks.UpHook(idx)) return;

		Event_RaiseInt(&PointerEvents.Up,   idx);
	}
}

static float scrollingVAcc;
void Mouse_ScrollVWheel(float delta) {
	int steps = Utils_AccumulateWheelDelta(&scrollingVAcc, delta);
	Event_RaiseFloat(&InputEvents.Wheel, delta);
	
	if (steps > 0) {
		for (; steps != 0; steps--) 
			Input_SetPressed(CCWHEEL_UP);
		Input_SetReleased(CCWHEEL_UP);
	} else if (steps < 0) {
		for (; steps != 0; steps++) 
			Input_SetPressed(CCWHEEL_DOWN);
		Input_SetReleased(CCWHEEL_DOWN);
	}
}

static float scrollingHAcc;
void Mouse_ScrollHWheel(float delta) {
	int steps = Utils_AccumulateWheelDelta(&scrollingHAcc, delta);
	
	if (steps > 0) {
		for (; steps != 0; steps--) 
			Input_SetPressed(CCWHEEL_RIGHT);
		Input_SetReleased(CCWHEEL_RIGHT);
	} else if (steps < 0) {
		for (; steps != 0; steps++) 
			Input_SetPressed(CCWHEEL_LEFT);
		Input_SetReleased(CCWHEEL_LEFT);
	}
}

void Pointer_SetPosition(int idx, int x, int y) {
	if (x == Pointers[idx].x && y == Pointers[idx].y) return;
	/* TODO: reset to -1, -1 when pointer is removed */
	Pointers[idx].x = x; Pointers[idx].y = y;

	if (PointerHooks.MoveHook && PointerHooks.MoveHook(idx)) return;
	
#ifdef CC_BUILD_TOUCH
	if (Input_TouchMode && !(touches[idx].type & TOUCH_TYPE_GUI)) return;
#endif
	Event_RaiseInt(&PointerEvents.Moved, idx);
}


/*########################################################################################################################*
*---------------------------------------------------------Keybinds--------------------------------------------------------*
*#########################################################################################################################*/
/* The gamepad buttons that are bound to each input binding */
static BindMapping padBind_Mappings[INPUT_MAX_GAMEPADS][BIND_COUNT];
BindMapping KeyBind_Mappings[BIND_COUNT];

const BindMapping PadBind_Defaults[BIND_COUNT] = {
	{ CCPAD_UP,   0 },  { CCPAD_DOWN,  0 }, /* BIND_FORWARD, BIND_BACK */
	{ CCPAD_LEFT, 0 },  { CCPAD_RIGHT, 0 }, /* BIND_LEFT, BIND_RIGHT */
	{ CCPAD_1, 0 },     { 0, 0 },           /* BIND_JUMP, BIND_RESPAWN */
	{ CCPAD_START, 0 }, { CCPAD_4,     0 }, /* BIND_SET_SPAWN, BIND_CHAT */
	{ CCPAD_3, 0     }, { 0, 0 },           /* BIND_INVENTORY, BIND_FOG */
	{ CCPAD_START, 0 }, { 0, 0 },           /* BIND_SEND_CHAT, BIND_TABLIST */
	{ CCPAD_2, CCPAD_L},{ CCPAD_2, CCPAD_3},/* BIND_SPEED, BIND_NOCLIP */ 
	{ CCPAD_2, CCPAD_R },                   /* BIND_FLY */ 
	{CCPAD_2,CCPAD_UP},{CCPAD_2,CCPAD_DOWN},/* BIND_FLY_UP, BIND_FLY_DOWN */
	{ 0, 0 }, { 0, 0 },                     /* BIND_EXT_INPUT, BIND_HIDE_FPS */
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, /* BIND_SCREENSHOT, BIND_FULLSCREEN, BIND_THIRD_PERSON, BIND_HIDE_GUI */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },           /* BIND_AXIS_LINES, BIND_ZOOM_SCROLL, BIND_HALF_SPEED */
	{ CCPAD_L, 0 }, { 0, 0 },{ CCPAD_R, 0 },/* BIND_DELETE_BLOCK, BIND_PICK_BLOCK, BIND_PLACE_BLOCK */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },           /* BIND_AUTOROTATE, BIND_HOTBAR_SWITCH, BIND_SMOOTH_CAMERA */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },           /* BIND_DROP_BLOCK, BIND_IDOVERLAY, BIND_BREAK_LIQUIDS */
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, /* BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_RIGHT, BIND_LOOK_LEFT */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },           /* BIND_HOTBAR_1, BIND_HOTBAR_2, BIND_HOTBAR_3 */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },           /* BIND_HOTBAR_4, BIND_HOTBAR_5, BIND_HOTBAR_6 */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },           /* BIND_HOTBAR_7, BIND_HOTBAR_8, BIND_HOTBAR_9 */
	{ CCPAD_ZL, 0 }, { CCPAD_ZR, 0 }        /* BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT */
};

const BindMapping KeyBind_Defaults[BIND_COUNT] = {
	{ 'W', 0 }, { 'S', 0 }, { 'A', 0 }, { 'D', 0 }, /* BIND_FORWARD - BIND_RIGHT */
	{ CCKEY_SPACE, 0 },  { 'R', 0 },                /* BIND_JUMP, BIND_RESPAWN */
	{ CCKEY_ENTER, 0 },  { 'T', 0 },                /* BIND_SET_SPAWN, BIND_CHAT */
	{ 'B', 0 },          { 'F', 0 },                /* BIND_INVENTORY, BIND_FOG */
	{ CCKEY_ENTER, 0 },  { CCKEY_TAB, 0 },          /* BIND_SEND_CHAT, BIND_TABLIST */
	{ CCKEY_LSHIFT, 0 }, { 'X', 0}, { 'Z', 0 },     /* BIND_SPEED, BIND_NOCLIP, BIND_FLY */ 
	{ 'Q', 0 },          { 'E', 0 },                /* BIND_FLY_UP, BIND_FLY_DOWN */
	{ CCKEY_LALT, 0 },   { CCKEY_F3, 0 },           /* BIND_EXT_INPUT, BIND_HIDE_FPS */
	{ CCKEY_F12, 0 },    { CCKEY_F11, 0 },          /* BIND_SCREENSHOT, BIND_FULLSCREEN */
	{ CCKEY_F5, 0 },     { CCKEY_F1, 0 },           /* BIND_THIRD_PERSON, BIND_HIDE_GUI */ 
	{ CCKEY_F7, 0 }, { 'C', 0 }, { CCKEY_LCTRL, 0 },/* BIND_AXIS_LINES, BIND_ZOOM_SCROLL, BIND_HALF_SPEED */
	{ CCMOUSE_L, 0},{ CCMOUSE_M, 0},{ CCMOUSE_R, 0},/* BIND_DELETE_BLOCK, BIND_PICK_BLOCK, BIND_PLACE_BLOCK */
	{ CCKEY_F6, 0 },     { CCKEY_LALT, 0 },         /* BIND_AUTOROTATE, BIND_HOTBAR_SWITCH */

	{ CCKEY_F8, 0 },     { 'G', 0 },                /* BIND_SMOOTH_CAMERA, BIND_DROP_BLOCK */
	{ CCKEY_F10, 0 },    { 0, 0 },                  /* BIND_IDOVERLAY, BIND_BREAK_LIQUIDS */
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },         /* BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_RIGHT, BIND_LOOK_LEFT */
	{ '1', 0 }, { '2', 0 }, { '3', 0 },             /* BIND_HOTBAR_1, BIND_HOTBAR_2, BIND_HOTBAR_3 */
	{ '4', 0 }, { '5', 0 }, { '6', 0 },             /* BIND_HOTBAR_4, BIND_HOTBAR_5, BIND_HOTBAR_6 */
	{ '7', 0 }, { '8', 0 }, { '9', 0 },             /* BIND_HOTBAR_7, BIND_HOTBAR_8, BIND_HOTBAR_9 */
	{ 0, 0 }, { 0, 0 }                              /* BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT */
};

static const char* const bindNames[BIND_COUNT] = {
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


#define BindMapping2_Claims(mapping, btn) (device->IsPressed(device, (mapping)->button1) && (mapping)->button2 == btn)
cc_bool InputBind_Claims(InputBind binding, int btn, struct InputDevice* device) {
	BindMapping* mappings = device->currentBinds;
	BindMapping* bind = &mappings[binding];
	int i;
	if (bind->button2) return BindMapping2_Claims(bind, btn);
	
	/* Two button mappings takes priority over one button mappings */
	for (i = 0; i < BIND_COUNT; i++)
	{
		if (mappings[i].button2 && BindMapping2_Claims(&mappings[i], btn)) return false;
	}
	return bind->button1 == btn;
}

void InputBind_Load(const struct InputDevice* device) {
	cc_string name; char nameBuffer[STRING_SIZE + 1];
	const BindMapping* defaults = device->defaultBinds;
	BindMapping* keybinds = device->currentBinds;
	BindMapping mapping;
	cc_string str, part1, part2;
	int i;

	String_InitArray_NT(name, nameBuffer);
	for (i = 0; i < BIND_COUNT; i++) 
	{
		name.length = 0;
		String_Format1(&name, device->bindPrefix, bindNames[i]);
		name.buffer[name.length] = '\0';
		
		if (!Options_UNSAFE_Get(name.buffer, &str)) {
			keybinds[i] = defaults[i];
			continue;
		}

		String_UNSAFE_Separate(&str, ',', &part1, &part2); 
		mapping.button1 = Utils_ParseEnum(&part1, defaults[i].button1, Input_StorageNames, INPUT_COUNT);
		mapping.button2 = Utils_ParseEnum(&part2, defaults[i].button2, Input_StorageNames, INPUT_COUNT);
		
		if (mapping.button1 == CCKEY_ESCAPE) keybinds[i] = defaults[i];
		keybinds[i] = mapping;
	}
}

void InputBind_Set(InputBind binding, int btn, const struct InputDevice* device) {
	cc_string name; char nameBuffer[STRING_SIZE];
	cc_string value;
	String_InitArray(name, nameBuffer);

	String_Format1(&name, device->bindPrefix, bindNames[binding]);
	value = String_FromReadonly(Input_StorageNames[btn]);
	Options_SetString(&name, &value);
	
	BindMapping_Set(&device->currentBinds[binding], btn, 0);
}

void InputBind_Reset(InputBind binding, const struct InputDevice* device) {
	cc_string name; char nameBuffer[STRING_SIZE];
	String_InitArray(name, nameBuffer);
	
	String_Format1(&name, device->bindPrefix, bindNames[binding]);
	Options_SetString(&name, &String_Empty);
	
	device->currentBinds[binding] = device->defaultBinds[binding];
}


/*########################################################################################################################*
*---------------------------------------------------------Gamepad---------------------------------------------------------*
*#########################################################################################################################*/
int Gamepad_AxisBehaviour[2]   = { AXIS_BEHAVIOUR_MOVEMENT, AXIS_BEHAVIOUR_CAMERA };
int Gamepad_AxisSensitivity[2] = { AXIS_SENSI_NORMAL, AXIS_SENSI_NORMAL };

static const float axis_sensiFactor[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
struct GamepadDevice Gamepad_Devices[INPUT_MAX_GAMEPADS];

static void Gamepad_Apply(int port, int btn, cc_bool was, int pressed) {
	struct InputDevice* device = &Gamepad_Devices[port].base;
	device->mappedIndex = Game_MapState(device->rawIndex);
	
	if (pressed) {
		Event_RaiseInput(&InputEvents.Down2, btn + GAMEPAD_BEG_BTN, was, device);
	} else {
		Event_RaiseInput(&InputEvents.Up2,   btn + GAMEPAD_BEG_BTN, was, device);
	}
}

static void Gamepad_Update(int port, float delta) {
	struct GamepadDevice* pad = &Gamepad_Devices[port];
	int btn;
	if (!pad->deviceID) return;

	for (btn = 0; btn < GAMEPAD_BTN_COUNT; btn++)
	{
		if (!pad->pressed[btn]) continue;
		pad->holdtime[btn] += delta;
		if (pad->holdtime[btn] < 1.0f) continue;

		/* Held for over a second, trigger a repeated press */
		pad->holdtime[btn] = 0;
		Gamepad_Apply(port, btn, true, true);
	}
}

void Gamepad_SetButton(int port, int btn, int pressed) {
	struct GamepadDevice* pad = &Gamepad_Devices[port];
	pressed = pressed != 0;

	btn -= GAMEPAD_BEG_BTN;
	/* Repeat down is handled in Gamepad_Update instead */
	if (pressed == pad->pressed[btn]) return;

	/* Reset hold tracking time */
	if (pressed && !pad->pressed[btn]) pad->holdtime[btn] = 0;
	pad->pressed[btn] = pressed;

	Gamepad_Apply(port, btn, false, pressed);
}

void Gamepad_SetAxis(int port, int axis, float x, float y, float delta) {
	float scale;
	int sensi;

	Gamepad_Devices[port].axisX[axis] = x;
	Gamepad_Devices[port].axisY[axis] = y;
	if (x == 0 && y == 0) return;

	sensi = Gamepad_AxisSensitivity[axis];
	scale = delta * 60.0f * axis_sensiFactor[sensi];
	Event_RaisePadAxis(&ControllerEvents.AxisUpdate, port, axis, x * scale, y * scale);
}

void Gamepad_Tick(float delta) {
	int port;
	Gamepads_Process(delta);
	
	for (port = 0; port < INPUT_MAX_GAMEPADS; port++)
	{
		Gamepad_Update(port, delta);
	}
}

static CC_NOINLINE void Gamepad_Add(int i, long deviceID, const struct BindMapping_* defaults) {
	Mem_Copy(&Gamepad_Devices[i].base, &padDevice, sizeof(struct InputDevice));
	Gamepad_Devices[i].base.rawIndex     = i;
	Gamepad_Devices[i].base.currentBinds = padBind_Mappings[i];
	Gamepad_Devices[i].base.defaultBinds = defaults;
	Gamepad_Devices[i].deviceID          = deviceID;

	InputBind_Load(&Gamepad_Devices[i].base);
}

int Gamepad_Connect(long deviceID, const struct BindMapping_* defaults) {
	int i;
	for (i = 0; i < INPUT_MAX_GAMEPADS; i++)
	{
		if (Gamepad_Devices[i].deviceID == deviceID) return i;
		if (Gamepad_Devices[i].deviceID != 0) continue;
		
		Gamepad_Add(i, deviceID, defaults);
		return i;
	}

	Process_Abort("Not enough controllers");
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Base handlers-------------------------------------------------------*
*#########################################################################################################################*/
static void OnFocusChanged(void* obj) { if (!Window_Main.Focused) Input_Clear(); }

static void OnInit(void) {
	Event_Register_(&WindowEvents.FocusChanged, NULL, OnFocusChanged);
	/* Fix issue with Android where if you double click in server list to join, a touch */
	/*  pointer is stuck down when the game loads (so you instantly start deleting blocks) */
	ClearTouches();
	
	InputBind_Load(&NormDevice);
}

static void OnFree(void) {
	ClearTouches();
}

struct IGameComponent Input_Component = {
	OnInit, /* Init  */
	OnFree, /* Free  */
};
