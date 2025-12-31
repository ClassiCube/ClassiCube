#include "../Window.h"
#include "../Platform.h"
#include "../Input.h"
#include "../Event.h"
#include "../Graphics.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../ExtMath.h"
#include "../Options.h"
#include "../VirtualKeyboard.h"
#include <kos.h>

#include "../VirtualCursor.h"
cc_bool window_inited;

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) {
	vid_set_mode(DEFAULT_VID_MODE, DEFAULT_PIXEL_MODE);
	vid_flip(0);
}

void Window_Init(void) {
	DisplayInfo.Width  = vid_mode->width;
	DisplayInfo.Height = vid_mode->height;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	Window_Main.Width    = vid_mode->width;
	Window_Main.Height   = vid_mode->height;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;

	DisplayInfo.ContentOffsetX = Option_GetOffsetX(10);
	DisplayInfo.ContentOffsetY = Option_GetOffsetY(20);
	Window_Main.SoftKeyboard   = SOFT_KEYBOARD_VIRTUAL;

	window_inited = true;
}

void Window_Free(void) { }

void Window_Create2D(int width, int height) { 
	Window_Main.Is3D = false;
}

void Window_Create3D(int width, int height) { 
	Window_Main.Is3D = true;
}

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
// TODO: More intelligent diffing that uses less space
static cc_bool has_prevState;
static kbd_state_t prevState;

static int MapKey(int k) {
	if (k >= KBD_KEY_A  && k <= KBD_KEY_Z)   return 'A'      + (k - KBD_KEY_A);
	if (k >= KBD_KEY_0  && k <= KBD_KEY_9)   return '0'      + (k - KBD_KEY_0);
	if (k >= KBD_KEY_F1 && k <= KBD_KEY_F12) return CCKEY_F1 + (k - KBD_KEY_F1);
	// KBD_KEY_PAD_0 isn't before KBD_KEY_PAD_1
	if (k >= KBD_KEY_PAD_1 && k <= KBD_KEY_PAD_9) return CCKEY_KP1 + (k - KBD_KEY_PAD_1);
	
	switch (k) {
	case KBD_KEY_ENTER:     return CCKEY_ENTER;
	case KBD_KEY_ESCAPE:    return CCKEY_ESCAPE;
	case KBD_KEY_BACKSPACE: return CCKEY_BACKSPACE;
	case KBD_KEY_TAB:       return CCKEY_TAB;
	case KBD_KEY_SPACE:     return CCKEY_SPACE;
	case KBD_KEY_MINUS:     return CCKEY_MINUS;
	case KBD_KEY_PLUS:      return CCKEY_EQUALS;
	case KBD_KEY_LBRACKET:  return CCKEY_LBRACKET;
	case KBD_KEY_RBRACKET:  return CCKEY_RBRACKET;
	case KBD_KEY_BACKSLASH: return CCKEY_BACKSLASH;
	case KBD_KEY_SEMICOLON: return CCKEY_SEMICOLON;
	case KBD_KEY_QUOTE:     return CCKEY_QUOTE;
	case KBD_KEY_TILDE:     return CCKEY_TILDE;
	case KBD_KEY_COMMA:     return CCKEY_COMMA;
	case KBD_KEY_PERIOD:    return CCKEY_PERIOD;
	case KBD_KEY_SLASH:     return CCKEY_SLASH;
	case KBD_KEY_CAPSLOCK:  return CCKEY_CAPSLOCK;
	case KBD_KEY_PRINT:     return CCKEY_PRINTSCREEN;
	case KBD_KEY_SCRLOCK:   return CCKEY_SCROLLLOCK;
	case KBD_KEY_PAUSE:     return CCKEY_PAUSE;
	case KBD_KEY_INSERT:    return CCKEY_INSERT;
	case KBD_KEY_HOME:      return CCKEY_HOME;
	case KBD_KEY_PGUP:      return CCKEY_PAGEUP;
	case KBD_KEY_DEL:       return CCKEY_DELETE;
	case KBD_KEY_END:       return CCKEY_END;
	case KBD_KEY_PGDOWN:    return CCKEY_PAGEDOWN;
	case KBD_KEY_RIGHT:     return CCKEY_RIGHT;
	case KBD_KEY_LEFT:      return CCKEY_LEFT;
	case KBD_KEY_DOWN:      return CCKEY_DOWN;
	case KBD_KEY_UP:        return CCKEY_UP;
	
	case KBD_KEY_PAD_NUMLOCK:  return CCKEY_NUMLOCK;
	case KBD_KEY_PAD_DIVIDE:   return CCKEY_KP_DIVIDE;
	case KBD_KEY_PAD_MULTIPLY: return CCKEY_KP_MULTIPLY;
	case KBD_KEY_PAD_MINUS:    return CCKEY_KP_MINUS;
	case KBD_KEY_PAD_PLUS:     return CCKEY_KP_PLUS;
	case KBD_KEY_PAD_ENTER:    return CCKEY_KP_ENTER;
	case KBD_KEY_PAD_0:        return CCKEY_KP0;
	case KBD_KEY_PAD_PERIOD:   return CCKEY_KP_DECIMAL;
	}
	return INPUT_NONE;
}
#define ToggleKey(diff, cur, mask, btn) if (diff & mask) Input_Set(btn, cur & mask)
static void UpdateKeyboardState(kbd_state_t* state) {
	int cur_keys  = state->shift_keys;
	int diff_keys = prevState.shift_keys ^ state->shift_keys;
	
	if (diff_keys) {
		ToggleKey(diff_keys, cur_keys, KBD_MOD_LALT,   CCKEY_LALT);
		ToggleKey(diff_keys, cur_keys, KBD_MOD_RALT,   CCKEY_RALT);
		ToggleKey(diff_keys, cur_keys, KBD_MOD_LCTRL,  CCKEY_LCTRL);
		ToggleKey(diff_keys, cur_keys, KBD_MOD_RCTRL,  CCKEY_RCTRL);
		ToggleKey(diff_keys, cur_keys, KBD_MOD_LSHIFT, CCKEY_LSHIFT);
		ToggleKey(diff_keys, cur_keys, KBD_MOD_RSHIFT, CCKEY_RSHIFT);
	}
	
	// see keyboard.h, KEY_S3 seems to be highest used key
	for (int i = KBD_KEY_A; i < KBD_KEY_S3; i++)
	{
		if (state->matrix[i] == prevState.matrix[i]) continue;
		int btn = MapKey(i);
		if (btn) Input_Set(btn, state->matrix[i]);
	}
}

static void ProcessKeyboardInput(void) {
	maple_device_t* kb_dev;
	kbd_state_t* state;

	kb_dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
	if (!kb_dev) return;
	state  = (kbd_state_t*)maple_dev_status(kb_dev);
	if (!state)  return;
	
	if (has_prevState) UpdateKeyboardState(state);
	has_prevState = true;
	prevState     = *state;
	
	Input.Sources |= INPUT_SOURCE_NORMAL;
	int ret = kbd_queue_pop(kb_dev, 1);
	if (ret < 0) return;
        
	// Ascii printable characters
	//  NOTE: Escape, Enter etc map to ASCII control characters
	if (ret >= ' ' && ret <= 0x7F)
		Event_RaiseInt(&InputEvents.Press, ret);
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void ProcessMouseInput(float delta) {
	maple_device_t* mouse;
	mouse_state_t*  state;

	mouse = maple_enum_type(0, MAPLE_FUNC_MOUSE);
	if (!mouse) return;
	state = (mouse_state_t*)maple_dev_status(mouse);
	if (!state) return;
	
	int mods = state->buttons;
	Input_SetNonRepeatable(CCMOUSE_L, mods & MOUSE_LEFTBUTTON);
	Input_SetNonRepeatable(CCMOUSE_R, mods & MOUSE_RIGHTBUTTON);
	Input_SetNonRepeatable(CCMOUSE_M, mods & MOUSE_SIDEBUTTON);
	Mouse_ScrollVWheel(-state->dz * 0.5f);

	if (!vc_hooked) {
		Pointer_SetPosition(0, Window_Main.Width / 2, Window_Main.Height / 2);
	}
	VirtualCursor_SetPosition(Pointers[0].x + state->dx, Pointers[0].y + state->dy);
	
	if (!Input.RawMode) return;	
	float scale = (delta * 60.0) / 2.0f;
	Event_RaiseRawMove(&PointerEvents.RawMoved, 
				state->dx * scale, state->dy * scale);
}

void Window_ProcessEvents(float delta) {
	ProcessKeyboardInput();
	ProcessMouseInput(delta);
}

void Cursor_SetPosition(int x, int y) {
	if (vc_hooked) VirtualCursor_SetPosition(x, y);
}

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*-------------------------------------------------------Gamepads----------------------------------------------------------*
*#########################################################################################################################*/
static const BindMapping defaults_dc[BIND_COUNT] = {
	[BIND_FORWARD]      = { CCPAD_UP    },  
	[BIND_BACK]         = { CCPAD_DOWN  },
	[BIND_LEFT]         = { CCPAD_LEFT  },  
	[BIND_RIGHT]        = { CCPAD_RIGHT },
	[BIND_JUMP]         = { CCPAD_1     },
	[BIND_SET_SPAWN]    = { CCPAD_START }, 
	[BIND_CHAT]         = { CCPAD_4     },
	[BIND_INVENTORY]    = { CCPAD_3     },
	[BIND_SEND_CHAT]    = { CCPAD_START },
	[BIND_PLACE_BLOCK]  = { CCPAD_L     },
	[BIND_DELETE_BLOCK] = { CCPAD_R     },
	[BIND_SPEED]        = { CCPAD_2, CCPAD_L },
	[BIND_FLY]          = { CCPAD_2, CCPAD_R },
	[BIND_NOCLIP]       = { CCPAD_2, CCPAD_3 },
	[BIND_FLY_UP]       = { CCPAD_2, CCPAD_UP },
	[BIND_FLY_DOWN]     = { CCPAD_2, CCPAD_DOWN },
	[BIND_HOTBAR_LEFT]  = { CCPAD_2, CCPAD_LEFT }, 
	[BIND_HOTBAR_RIGHT] = { CCPAD_2, CCPAD_RIGHT },
	[BIND_SCREENSHOT]   = { CCPAD_3 },
};

void Gamepads_PreInit(void) { }
void Gamepads_Init(void)    { }

static void HandleButtons(int port, int mods) {
	Gamepad_SetButton(port, CCPAD_1, mods & CONT_A);
	Gamepad_SetButton(port, CCPAD_2, mods & CONT_B);
	Gamepad_SetButton(port, CCPAD_3, mods & CONT_X);
	Gamepad_SetButton(port, CCPAD_4, mods & CONT_Y);
      
	Gamepad_SetButton(port, CCPAD_START,  mods & CONT_START);
	Gamepad_SetButton(port, CCPAD_SELECT, mods & CONT_D);

	Gamepad_SetButton(port, CCPAD_LEFT,   mods & CONT_DPAD_LEFT);
	Gamepad_SetButton(port, CCPAD_RIGHT,  mods & CONT_DPAD_RIGHT);
	Gamepad_SetButton(port, CCPAD_UP,     mods & CONT_DPAD_UP);
	Gamepad_SetButton(port, CCPAD_DOWN,   mods & CONT_DPAD_DOWN);
	
	// Buttons not on standard controller
	Gamepad_SetButton(port, CCPAD_6,       mods & CONT_C);
	Gamepad_SetButton(port, CCPAD_7,       mods & CONT_D);
	Gamepad_SetButton(port, CCPAD_5,       mods & CONT_Z);
	Gamepad_SetButton(port, CCPAD_CLEFT,   mods & CONT_DPAD2_LEFT);
	Gamepad_SetButton(port, CCPAD_CRIGHT,  mods & CONT_DPAD2_RIGHT);
	Gamepad_SetButton(port, CCPAD_CUP,     mods & CONT_DPAD2_UP);
	Gamepad_SetButton(port, CCPAD_CDOWN,   mods & CONT_DPAD2_DOWN);
}

#define AXIS_SCALE 8.0f
static void HandleJoystick(int port, int axis, int x, int y, float delta) {
	if (Math_AbsI(x) <= 8) x = 0;
	if (Math_AbsI(y) <= 8) y = 0;	
	
	Gamepad_SetAxis(port, axis, x / AXIS_SCALE, y / AXIS_SCALE, delta);
}

static void HandleController(int port, bool dual_analog, cont_state_t* state, float delta) {
	Gamepad_SetButton(port, CCPAD_L, state->ltrig > 10);
	Gamepad_SetButton(port, CCPAD_R, state->rtrig > 10);
	// TODO: verify values are right     
	if(dual_analog) 
	{
		HandleJoystick(port, PAD_AXIS_LEFT,  state->joyx,  state->joyy,  delta);
		HandleJoystick(port, PAD_AXIS_RIGHT, state->joy2x, state->joy2y, delta);
	}
	else
	{
		HandleJoystick(port, PAD_AXIS_RIGHT, state->joyx, state->joyy, delta);
	}
}

void Gamepads_Process(float delta) {
	maple_device_t* cont;
	cont_state_t*  state;

	for (int i = 0; i < 4; i++)
	{
		cont  = maple_enum_type(i, MAPLE_FUNC_CONTROLLER);
		if (!cont)  return;
		state = (cont_state_t*)maple_dev_status(cont);
		if (!state) return;

		int dual_analog = cont_has_capabilities(cont, CONT_CAPABILITIES_DUAL_ANALOG);
		if(dual_analog == -1) dual_analog = 0;

		int port = Gamepad_Connect(0xDC + i, defaults_dc);
		HandleButtons(port, state->buttons);
		HandleController(port, dual_analog, state, delta);
	}
}


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// TODO: double buffering ??
	//	https://dcemulation.org/phpBB/viewtopic.php?t=99999
	//	https://dcemulation.org/phpBB/viewtopic.php?t=43214
	vid_waitvbl();
	
	for (int y = r.y; y < r.y + r.height; y++)
	{
		BitmapCol* src = Bitmap_GetRow(bmp, y);
		uint16_t*  dst = vram_s + vid_mode->width * y;
		
		for (int x = r.x; x < r.x + r.width; x++)
		{
			BitmapCol color = src[x];
			// 888 to 565 (discard least significant bits)
			dst[x] = ((BitmapCol_R(color) & 0xF8) << 8) | ((BitmapCol_G(color) & 0xFC) << 3) | (BitmapCol_B(color) >> 3);
		}
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { 
	if (Input.Sources & INPUT_SOURCE_NORMAL) return;
	VirtualKeyboard_Open(args);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Close(void) {
	VirtualKeyboard_Close();
}


/*########################################################################################################################*
*-------------------------------------------------------Misc/Other--------------------------------------------------------*
*#########################################################################################################################*/
void Window_ShowDialog(const char* title, const char* msg) {
	/* TODO implement */
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}
