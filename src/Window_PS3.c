#include "Core.h"
#if defined CC_BUILD_PS3
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
#include "Logger.h"
#include <io/pad.h>
#include <io/kb.h> 
#include <sysutil/sysutil.h>
#include <sysutil/video.h>

static cc_bool launcherMode;
static padInfo  pad_info;
static padData  pad_data;
static KbInfo   kb_info;
static KbData   kb_data;
static KbConfig kb_config;

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;
// no DPI scaling on PS Vita
int Display_ScaleX(int x) { return x; }
int Display_ScaleY(int y) { return y; }

static void sysutil_callback(u64 status, u64 param, void* usrdata) {
	switch (status) {
		case SYSUTIL_EXIT_GAME:
			WindowInfo.Exists = false;
			Window_Close();
			break;
	}
}

void Window_Init(void) {
	sysUtilRegisterCallback(0, sysutil_callback, NULL);
	
	videoState state;
	videoResolution resolution;
	
	videoGetState(0, 0, &state);
	videoGetResolution(state.displayMode.resolution, &resolution);
      
	DisplayInfo.Width  = resolution.width;
	DisplayInfo.Height = resolution.height;
	DisplayInfo.Depth  = 4; // 32 bit
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
	
	WindowInfo.Width   = resolution.width;
	WindowInfo.Height  = resolution.height;
	WindowInfo.Focused = true;
	WindowInfo.Exists  = true;

	Input.Sources = INPUT_SOURCE_GAMEPAD;
	DisplayInfo.ContentOffsetX = 20;
	DisplayInfo.ContentOffsetY = 20;

	ioPadInit(MAX_PORT_NUM);
	ioKbInit(MAX_KB_PORT_NUM);
	ioKbSetCodeType(0, KB_CODETYPE_RAW);
	ioKbGetConfiguration(0, &kb_config);
}

void Window_Create2D(int width, int height) { 
	launcherMode = true;
	Gfx_Create(); // launcher also uses RSX to draw
}

void Window_Create3D(int width, int height) { 
	launcherMode = false; 
}

void Window_SetTitle(const cc_string* title) { }
void Clipboard_GetText(cc_string* value) { } // TODO sceClipboardGetText
void Clipboard_SetText(const cc_string* value) { } // TODO sceClipboardSetText

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
*--------------------------------------------------Keyboard processing----------------------------------------------------*
*#########################################################################################################################*/
#define MAX_KEYCODE_MAPPINGS 148
static char now_pressed[MAX_KEYCODE_MAPPINGS], was_pressed[MAX_KEYCODE_MAPPINGS];
static int MapKey(int k) {
	if (k >= KB_RAWKEY_A      && k <= KB_RAWKEY_Z)      return 'A'       + (k - KB_RAWKEY_A);
	if (k >= KB_RAWKEY_1      && k <= KB_RAWKEY_9)      return '1'       + (k - KB_RAWKEY_1);
	if (k >= KB_RAWKEY_F1     && k <= KB_RAWKEY_F12)    return CCKEY_F1  + (k - KB_RAWKEY_F1);
	if (k >= KB_RAWKEY_KPAD_1 && k <= KB_RAWKEY_KPAD_9) return CCKEY_KP1 + (k - KB_RAWKEY_KPAD_1);
	switch (k) {
	case KB_RAWKEY_PRINTSCREEN: return CCKEY_PRINTSCREEN;
	case KB_RAWKEY_SCROLL_LOCK: return CCKEY_SCROLLLOCK;
	case KB_RAWKEY_PAUSE:       return CCKEY_PAUSE;
	case KB_RAWKEY_INSERT:      return CCKEY_INSERT;
	case KB_RAWKEY_HOME:        return CCKEY_HOME;
	case KB_RAWKEY_PAGE_UP:     return CCKEY_PAGEUP;
	case KB_RAWKEY_DELETE:      return CCKEY_DELETE;
	case KB_RAWKEY_END:         return CCKEY_END;
	case KB_RAWKEY_PAGE_DOWN:   return CCKEY_PAGEDOWN;
	case KB_RAWKEY_RIGHT_ARROW: return CCKEY_RIGHT;
	case KB_RAWKEY_LEFT_ARROW:  return CCKEY_LEFT;
	case KB_RAWKEY_DOWN_ARROW:  return CCKEY_DOWN;
	case KB_RAWKEY_UP_ARROW:    return CCKEY_UP;
	case KB_RAWKEY_0:         return '0';
	case KB_RAWKEY_ENTER:     return CCKEY_ENTER;
	case KB_RAWKEY_ESCAPE:    return CCKEY_ESCAPE;
	case KB_RAWKEY_BS:        return CCKEY_BACKSPACE;
	case KB_RAWKEY_TAB:       return CCKEY_TAB;
	case KB_RAWKEY_SPACE:     return CCKEY_SPACE;
	case KB_RAWKEY_MINUS:     return CCKEY_MINUS;
	case KB_RAWKEY_EQUAL_101: return CCKEY_EQUALS;
	//case KB_RAWKEY_ACCENT_CIRCONFLEX_106: return CCKEY_TILDE;
	//case KB_RAWKEY_LEFT_BRACKET_101:  return CCKEY_LBRACKET;
	//case KB_RAWKEY_ATMARK_106
	//case KB_RAWKEY_RIGHT_BRACKET_101: return CCKEY_RBRACKET;
	case KB_RAWKEY_LEFT_BRACKET_106:  return CCKEY_LBRACKET;
	case KB_RAWKEY_BACKSLASH_101:     return CCKEY_BACKSLASH;
	case KB_RAWKEY_RIGHT_BRACKET_106: return CCKEY_RBRACKET;
	case KB_RAWKEY_SEMICOLON:         return CCKEY_SEMICOLON;
	case KB_RAWKEY_QUOTATION_101:     return CCKEY_QUOTE;
	//case KB_RAWKEY_COLON_106:         return CCKEY_SEMICOLON;
	case KB_RAWKEY_COMMA:             return CCKEY_COMMA;
	case KB_RAWKEY_PERIOD:            return CCKEY_PERIOD;
	case KB_RAWKEY_SLASH:             return CCKEY_SLASH;
	case KB_RAWKEY_CAPS_LOCK:         return CCKEY_CAPSLOCK;
	
	case KB_RAWKEY_KPAD_NUMLOCK:  return CCKEY_NUMLOCK;
	case KB_RAWKEY_KPAD_SLASH:    return CCKEY_KP_DIVIDE;
	case KB_RAWKEY_KPAD_ASTERISK: return CCKEY_KP_MULTIPLY;
	case KB_RAWKEY_KPAD_MINUS:    return CCKEY_KP_MINUS;
	case KB_RAWKEY_KPAD_PLUS:     return CCKEY_KP_PLUS;
	case KB_RAWKEY_KPAD_ENTER:    return CCKEY_KP_ENTER;
	case KB_RAWKEY_KPAD_0:        return CCKEY_KP0;
	case KB_RAWKEY_KPAD_PERIOD:   return CCKEY_KP_DECIMAL;
	case KB_RAWKEY_BACKSLASH_106: return CCKEY_BACKSLASH;
	
	case 147: return CCKEY_TILDE;
	}
	return 0;
}
static cc_bool kb_deferredClear;
static void ProcessKBButtons(void) {
	// PS3 keyboard APIs only seem to return current keys pressed,
	//  which is a massive pain to work with
	// 
	// The API is really strange and when pressing two keys produces e.g.
	//   - Event 1) pressed 82
	//   - Event 2) pressed 46
	// instead of
	//   - Event 1) pressed 82
	//   - Event 2) pressed 82 46
	// 
	// Additionally on real hardware, the following events when observed
	//   - Releasing key: [key] [0]
	//   - Holding key: [key] [0] [key] [0] [key] [0]
	// I don't really know why this happens, so try to detect this by
	//  deferring resetting all keys to next Window_ProcessEvents
	// TODO properly investigate this	
	
	if (kb_deferredClear && (kb_data.nb_keycode == 0 || kb_data.keycode[0] == 0)) {
		Mem_Set(now_pressed, 0, sizeof(now_pressed));
		kb_deferredClear = false;
	} else {
		kb_deferredClear = false;
		if (!kb_data.nb_keycode) return;
	}
	
	// possibly unpress all keys next time around
	if (kb_data.keycode[0] == 0) kb_deferredClear = true;
	
	for (int i = 0; i < kb_data.nb_keycode; i++)
	{
		int rawcode = kb_data.keycode[i];
		if (rawcode > 0 && rawcode < MAX_KEYCODE_MAPPINGS) 
			now_pressed[rawcode] = true;
	}
	
	for (int i = 0; i < MAX_KEYCODE_MAPPINGS; i++)
	{
		if (now_pressed[i] == was_pressed[i]) continue;
		
		int key = MapKey(i);
		if (key) Input_SetNonRepeatable(key, now_pressed[i]);
		//if (key) Platform_Log3("UPDATE %h: %c = %t", &i, Input_DisplayNames[key], &now_pressed[i]);
	}
	
	Mem_Copy(was_pressed, now_pressed, sizeof(now_pressed));
}

static KbMkey old_mods;
#define ToggleMod(field, btn) if (diff._KbMkeyU._KbMkeyS. field) Input_Set(btn, mods->_KbMkeyU._KbMkeyS. field);

static void ProcessKBModifiers(KbMkey* mods) {
	KbMkey diff;
	diff._KbMkeyU.mkeys = mods->_KbMkeyU.mkeys ^ old_mods._KbMkeyU.mkeys;
	
	ToggleMod(l_alt,   CCKEY_LALT);
	ToggleMod(r_alt,   CCKEY_RALT);
	ToggleMod(l_ctrl,  CCKEY_LCTRL);
	ToggleMod(r_ctrl,  CCKEY_RCTRL);
	ToggleMod(l_shift, CCKEY_LSHIFT);
	ToggleMod(r_shift, CCKEY_RSHIFT);
	ToggleMod(l_win,   CCKEY_LWIN);
	ToggleMod(r_win,   CCKEY_RWIN);
	
	old_mods = *mods;
}

static void ProcessKBTextInput(void) {
	for (int i = 0; i < kb_data.nb_keycode; i++)
	{
		int rawcode = kb_data.keycode[i];
		if (!rawcode) continue;
		int unicode = ioKbCnvRawCode(kb_config.mapping, kb_data.mkey, kb_data.led, rawcode);
		
		if (unicode && unicode <= 0xFF) 
			Event_RaiseInt(&InputEvents.Press, (cc_unichar)unicode);
			
		//char C = unicode;
		//Platform_Log4("%i --> %i / %h / %r", &rawcode, &unicode, &unicode, &C);
	}
}


/*########################################################################################################################*
*----------------------------------------------------Input processing-----------------------------------------------------*
*#########################################################################################################################*/
static void HandleButtons(padData* data) {
	//Platform_Log2("BUTTONS: %h (%h)", &data->button[2], &data->button[0]);
	Input_SetNonRepeatable(CCPAD_A, data->BTN_TRIANGLE);
	Input_SetNonRepeatable(CCPAD_B, data->BTN_SQUARE);
	Input_SetNonRepeatable(CCPAD_X, data->BTN_CROSS);
	Input_SetNonRepeatable(CCPAD_Y, data->BTN_CIRCLE);
      
	Input_SetNonRepeatable(CCPAD_START,  data->BTN_START);
	Input_SetNonRepeatable(CCPAD_SELECT, data->BTN_SELECT);

	Input_SetNonRepeatable(CCPAD_LEFT,   data->BTN_LEFT);
	Input_SetNonRepeatable(CCPAD_RIGHT,  data->BTN_RIGHT);
	Input_SetNonRepeatable(CCPAD_UP,     data->BTN_UP);
	Input_SetNonRepeatable(CCPAD_DOWN,   data->BTN_DOWN);
	
	Input_SetNonRepeatable(CCPAD_L,  data->BTN_L1);
	Input_SetNonRepeatable(CCPAD_R,  data->BTN_R1);
	Input_SetNonRepeatable(CCPAD_ZL, data->BTN_L2);
	Input_SetNonRepeatable(CCPAD_ZR, data->BTN_R2);
}

static void HandleJoystick_Left(int x, int y) {
	if (Math_AbsI(x) <= 32) x = 0;
	if (Math_AbsI(y) <= 32) y = 0;	
	
	if (x == 0 && y == 0) return;
	Input.JoystickMovement = true;
	Input.JoystickAngle    = Math_Atan2(x, -y);
}

static void HandleJoystick_Right(int x, int y, double delta) {
	float scale = (delta * 60.0) / 32.0f;
	
	if (Math_AbsI(x) <= 32) x = 0;
	if (Math_AbsI(y) <= 32) y = 0;
	
	Event_RaiseRawMove(&PointerEvents.RawMoved, x * scale, y * scale);	
}

static void ProcessPadInput(double delta, padData* pad) {
	HandleButtons(pad);
	HandleJoystick_Left( pad->ANA_L_H - 0x80, pad->ANA_L_V - 0x80);
	HandleJoystick_Right(pad->ANA_R_H - 0x80, pad->ANA_R_V - 0x80, delta);
}


void Window_ProcessEvents(double delta) {
	Input.JoystickMovement = false;
	
	ioPadGetInfo(&pad_info);
	if (pad_info.status[0]) {
		ioPadGetData(0, &pad_data);
		ProcessPadInput(delta, &pad_data);
	}
	
	// TODO set InputSource keyboard
	ioKbGetInfo(&kb_info);
	if (kb_info.status[0]) {
		int res = ioKbRead(0, &kb_data);
		if (res == 0 && kb_data.nb_keycode > 0) {
			ProcessKBButtons();
			ProcessKBModifiers(&kb_data.mkey);
			ProcessKBTextInput();
		}
	}
}

void Cursor_SetPosition(int x, int y) { } // Makes no sense for PS Vita

void Window_EnableRawMouse(void)  { Input.RawMode = true; }
void Window_UpdateRawMouse(void)  {  }
void Window_DisableRawMouse(void) { Input.RawMode = false; }


/*########################################################################################################################*
*------------------------------------------------------Framebuffer--------------------------------------------------------*
*#########################################################################################################################*/
static struct Bitmap fb_bmp;
static u32 fb_offset;

extern u32* Gfx_AllocImage(u32* offset, s32 w, s32 h);
extern void Gfx_TransferImage(u32 offset, s32 w, s32 h);

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	u32* pixels = Gfx_AllocImage(&fb_offset, bmp->width, bmp->height);
	bmp->scan0  = pixels;
	fb_bmp      = *bmp;
	
	Gfx_ClearCol(PackedCol_Make(0x40, 0x60, 0x80, 0xFF));
}

void Window_DrawFramebuffer(Rect2D r) {
	// TODO test
	Gfx_BeginFrame();
	Gfx_Clear();
	// TODO: Only transfer dirty region instead of the entire bitmap
	Gfx_TransferImage(fb_offset, fb_bmp.width, fb_bmp.height);
	Gfx_EndFrame();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	//Mem_Free(bmp->scan0);
	/* TODO free framebuffer */
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