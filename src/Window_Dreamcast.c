#include "Core.h"
#if defined CC_BUILD_DREAMCAST
#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "Event.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "ExtMath.h"
#include <kos.h>
static cc_bool launcherMode;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on 3DS
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

void Window_Init(void) {
	DisplayInfo.Width  = vid_mode->width;
	DisplayInfo.Height = vid_mode->height;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = vid_mode->width;
	WindowInfo.Height  = vid_mode->height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 10;
	DisplayInfo.ContentOffsetY = 20;
}

void Window_Create2D(int width, int height) { 
	launcherMode = true;
	vid_set_mode(DEFAULT_VID_MODE, PM_RGB888);
	vid_flip(0);
}

void Window_Create3D(int width, int height) { 
	launcherMode = false;
	vid_set_mode(DEFAULT_VID_MODE, DEFAULT_PIXEL_MODE);
	vid_flip(0);
	// TODO: Why doesn't 32 bit work on real hardware?
}

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void)  { return 0; }
int Window_IsObscured(void)            { return 0; }

void Window_Show(void) { }
void Window_SetSize(int width, int height) { }

void Window_Close(void) {
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
static void HandleButtons(int mods) {
	// TODO CONT_Z
      
	Input_SetNonRepeatable(CCPAD_A, mods & CONT_A);
	Input_SetNonRepeatable(CCPAD_B, mods & CONT_B);
	Input_SetNonRepeatable(CCPAD_X, mods & CONT_X);
	Input_SetNonRepeatable(CCPAD_Y, mods & CONT_Y);
      
	Input_SetNonRepeatable(CCPAD_START,  mods & CONT_START);
	Input_SetNonRepeatable(CCPAD_SELECT, mods & CONT_D);

	Input_SetNonRepeatable(CCPAD_LEFT,   mods & CONT_DPAD_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  mods & CONT_DPAD_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     mods & CONT_DPAD_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   mods & CONT_DPAD_DOWN);
}

static void HandleController(cont_state_t* state, double delta) {
	Input_SetNonRepeatable(CCPAD_L, state->ltrig > 10);
	Input_SetNonRepeatable(CCPAD_R, state->rtrig > 10);
	// TODO CONT_Z, joysticks
	// TODO: verify values are right
      
	if (Input.RawMode) {
		float scale = (delta * 60.0) / 8.0f;
		int dx = state->joyx, dy = state->joyy;
		if (Math_AbsI(dx) <= 8) dx = 0;
		if (Math_AbsI(dy) <= 8) dy = 0;
		
		Event_RaiseRawMove(&PointerEvents.RawMoved, dx * scale, dy * scale);
	}
}

static void ProcessControllerInput(double delta) {
	maple_device_t* cont;
	cont_state_t* state;

	cont  = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if (!cont)  return;
	state = (cont_state_t*)maple_dev_status(cont);
	if (!state) return;
	
	HandleButtons(state->buttons);
	HandleController(state, delta);
}

void Window_ProcessEvents(double delta) {
	ProcessControllerInput(delta);
	ProcessKeyboardInput();
}

void Cursor_SetPosition(int x, int y) { } /* TODO: Dreamcast mouse support */

void Window_EnableRawMouse(void)  { Input.RawMode = true;  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }
void Window_UpdateRawMouse(void)  { }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	// TODO probably bogus..
	// TODO: Don't redraw everything
	int size = fb_bmp.width * fb_bmp.height * 4;
	
	// TODO: double buffering ??
	//	https://dcemulation.org/phpBB/viewtopic.php?t=99999
	//	https://dcemulation.org/phpBB/viewtopic.php?t=43214
	vid_waitvbl();
	sq_cpy(vram_l, fb_bmp.scan0, size);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}


/*########################################################################################################################*
*------------------------------------------------------Soft keyboard------------------------------------------------------*
*#########################################################################################################################*/
void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { /* TODO implement */ }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { /* TODO implement */ }


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
#endif