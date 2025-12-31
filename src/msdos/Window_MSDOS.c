#include "../_WindowBase.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Options.h"
#include "../Errors.h"

#include <dpmi.h>
#include <sys/nearptr.h>
#include <pc.h>
#include <bios.h>

#define INT_VGA            0x10
#define VGA_CMD_SETMODE  0x0000
#define VGA_MODE_TEXT_BW   0x02
#define VGA_MODE_320x200_8 0x13

#define INT_MOUSE         0x33
#define MOUSE_CMD_RESET 0x0000
#define MOUSE_CMD_SHOW  0x0001
#define MOUSE_CMD_HIDE  0x0002
#define MOUSE_CMD_POLL  0x0003

#define MOUSE_X(x) ((x) / 2)
#define MOUSE_Y(y) (y)


/*########################################################################################################################*
*------------------------------------------------------Mouse support------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool mouseSupported;
static void Mouse_Init(void) {
	__dpmi_regs regs;
	regs.x.ax = MOUSE_CMD_RESET;
	__dpmi_int(INT_MOUSE, &regs);

	if (regs.x.ax == 0) { mouseSupported = false; return; }
	mouseSupported = true;
	Cursor_DoSetVisible(true);
}

static void Mouse_Poll(void) {
	if (!mouseSupported) return;

	__dpmi_regs regs;
	regs.x.ax = MOUSE_CMD_POLL;
	__dpmi_int(INT_MOUSE, &regs);

	int b = regs.x.bx;
	int x = MOUSE_X(regs.x.cx);
	int y = MOUSE_Y(regs.x.dx);

	Input_SetNonRepeatable(CCMOUSE_L, b & 0x01);
	Input_SetNonRepeatable(CCMOUSE_R, b & 0x02);
	Input_SetNonRepeatable(CCMOUSE_M, b & 0x04);

	Pointer_SetPosition(0, x, y);
}

static void Cursor_GetRawPos(int* x, int* y) {
	*x = 0;
	*y = 0;
	if (!mouseSupported) return;

	__dpmi_regs regs;
	regs.x.ax = MOUSE_CMD_POLL;
	__dpmi_int(INT_MOUSE, &regs);

	*x = MOUSE_X(regs.x.cx);
	*y = MOUSE_Y(regs.x.dx);
}

void Cursor_SetPosition(int x, int y) { 
	if (!mouseSupported) return;

}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (!mouseSupported) return;

	__dpmi_regs regs;
	regs.x.ax = visible ? MOUSE_CMD_SHOW : MOUSE_CMD_HIDE;
	__dpmi_int(INT_MOUSE, &regs);
}


/*########################################################################################################################*
*----------------------------------------------------Keyboard support-----------------------------------------------------*
*#########################################################################################################################*/
// TODO use proper keyboard interrupts
static float event_time;
static float press_start[256];
// TODO missing numpad codes

static const cc_uint8 key_map[] = {
/* 0x00 */ 0, CCKEY_ESCAPE, '1', '2',  '3', '4', '5', '6',
/* 0x08 */ '7', '8', '9', '0',  CCKEY_MINUS, CCKEY_EQUALS, CCKEY_BACKSPACE, CCKEY_TAB,
/* 0x10 */ 'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I',
/* 0x18 */ 'O', 'P', CCKEY_LBRACKET, CCKEY_RBRACKET,  CCKEY_ENTER, 0, 'A', 'S',
/* 0x20 */ 'D', 'F', 'G', 'H',  'J', 'K', 'L', CCKEY_SEMICOLON,
/* 0x28 */ CCKEY_QUOTE, CCKEY_TILDE, 0, CCKEY_BACKSLASH,  'Z', 'X', 'C', 'V',
/* 0x30 */ 'B', 'N', 'M', CCKEY_COMMA,  CCKEY_PERIOD, CCKEY_SLASH, 0, 0,
/* 0x38 */ 0, CCKEY_SPACE, 0, CCKEY_F1,  CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5,
/* 0x40 */ CCKEY_F6, CCKEY_F7, CCKEY_F8, CCKEY_F9,  CCKEY_F10, 0, 0, CCKEY_HOME,
/* 0x48 */ CCKEY_UP, CCKEY_PAGEUP, 0, CCKEY_LEFT,  0, CCKEY_RIGHT, 0, CCKEY_END,
/* 0x50 */ CCKEY_DOWN, CCKEY_PAGEDOWN, 0, 0,  0, 0, 0, 0,
/* 0x58 */ 0, 0, 0, 0,  0, 0, 0, 0,
/* 0x60 */ 0, 0, 0, 0,  0, 0, 0, 0,
/* 0x68 */ 0, 0, 0, 0,  0, 0, 0, 0,
/* 0x70 */ 0, 0, 0, 0,  0, 0, 0, 0,
/* 0x78 */ 0, 0, 0, 0,  0, 0, 0, 0,
/* 0x80 */ 0, 0, 0, 0,  0, CCKEY_F11, CCKEY_F12, 0,
};
static int MapKey(unsigned key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }


static void Keyboard_UpdateState(float delta) {
	event_time += delta;
	// Auto release keys after a while
	for (int i = 0; i < 256; i++)
	{
		if (press_start[i] && (event_time - press_start[i]) > 1.0f) {
			Input_SetReleased(MapKey(i));
			press_start[i] = 0.0f;
		}
	}
}

static void Keyboard_PollInput(void) {
	if (_bios_keybrd(_NKEYBRD_READY) == 0) return;
	unsigned raw = _bios_keybrd(_NKEYBRD_READ);

	// Lower 8 bits contain ascii code
	unsigned code = raw & 0xFF;
	if (code >= 32 && code < 127) {
		Event_RaiseInt(&InputEvents.Press, code);
	}

	// Higher 8 bits contain raw code
	raw = (raw >> 8) & 0xFF;
	int key = MapKey(raw);
	if (!key) return;

	Input_SetPressed(key);
	press_start[raw] = event_time;
}

static void Keyboard_UpdateModifiers(void) {
	unsigned mods = _bios_keybrd(_KEYBRD_SHIFTSTATUS);

	Input_SetNonRepeatable(CCKEY_RSHIFT, mods & 0x02);
	Input_SetNonRepeatable(CCKEY_LCTRL,  mods & 0x04);
	Input_SetNonRepeatable(CCKEY_LALT,   mods & 0x08);

	Input_SetNonRepeatable(CCKEY_SCROLLLOCK, mods & 0x10);
	Input_SetNonRepeatable(CCKEY_NUMLOCK,    mods & 0x20);
	Input_SetNonRepeatable(CCKEY_CAPSLOCK,   mods & 0x40);
	Input_SetNonRepeatable(CCKEY_INSERT,     mods & 0x80);
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { }

#define DISP_WIDTH  320
#define DISP_HEIGHT 200

void Window_Init(void) {
	DisplayInfo.Width  = DISP_WIDTH;
	DisplayInfo.Height = DISP_HEIGHT;

	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;

	// Change VGA mode to 0x13 (320x200x8bpp)
	__dpmi_regs regs;
	regs.x.ax = VGA_CMD_SETMODE | VGA_MODE_320x200_8;
	__dpmi_int(INT_VGA, &regs);

	// Change VGA colour palette (NOTE: only lower 6 bits are used)
	// Fake a linear RGB palette
	outportb(0x3c8, 0);
	for (int i = 0; i < 256; i++) {
    	outportb(0x3c9, ((i >> 0) & 0x03) << 4);
    	outportb(0x3c9, ((i >> 2) & 0x07) << 3);
    	outportb(0x3c9, ((i >> 5) & 0x07) << 3);	
	}

	Mouse_Init();
}

void Window_Free(void) {
	if (__djgpp_nearptr_enable() == 0) return;

	char* screen = (char*)0xa0000 + __djgpp_conventional_base;
	Mem_Set(screen, 0, DISP_WIDTH * DISP_HEIGHT);

	__djgpp_nearptr_disable();

	// Restore VGA to text mode
	__dpmi_regs regs;
	regs.x.ax = VGA_CMD_SETMODE | VGA_MODE_TEXT_BW;
	__dpmi_int(INT_VGA, &regs);
}

static void DoCreateWindow(int width, int height) {
	Window_Main.Width    = 320;
	Window_Main.Height   = 200;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }

void Clipboard_GetText(cc_string* value) { }

void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) { }

void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
	Mouse_Poll();
	Keyboard_PollInput();
	Keyboard_UpdateModifiers();
	Keyboard_UpdateState(delta);
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }

static void ShowDialogCore(const char* title, const char* msg) {
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

static CC_INLINE void DrawFramebuffer(Rect2D r, struct Bitmap* bmp, char* screen) {
    for (int y = r.y; y < r.y + r.height; ++y) 
	{
        BitmapCol* row = Bitmap_GetRow(bmp, y);
        for (int x = r.x; x < r.x + r.width; ++x) 
		{
            // TODO optimise
            BitmapCol	col = row[x];
			cc_uint8 R = BitmapCol_R(col);
			cc_uint8 G = BitmapCol_G(col);
			cc_uint8 B = BitmapCol_B(col);

			screen[y*320+x] = (R >> 6) | ((G >> 5) << 2) | ((B >> 5) << 5);
        }
    }
}

static CC_INLINE void DrawDirect(struct Bitmap* bmp, char* screen) {
	BitmapCol* src = bmp->scan0;

	for (int i = 0; i < DISP_WIDTH * DISP_HEIGHT; i++) 
	{
		BitmapCol col = src[i];
		cc_uint8 R = BitmapCol_R(col);
		cc_uint8 G = BitmapCol_G(col);
		cc_uint8 B = BitmapCol_B(col);

		screen[i] = (R >> 6) | ((G >> 5) << 2) | ((B >> 5) << 5);
	}
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	char *screen;
	if (__djgpp_nearptr_enable() == 0) return;

	screen = (char*)0xa0000 + __djgpp_conventional_base;

	if (r.x == 0 && r.y == 0 && r.width == DISP_WIDTH && r.height == DISP_HEIGHT && bmp->width == DISP_WIDTH) {
		DrawDirect(bmp, screen);
	} else {
		DrawFramebuffer(r, bmp, screen);
	}

	__djgpp_nearptr_disable();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { }

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
}

void Window_UpdateRawMouse(void) {
	DefaultUpdateRawMouse();
}

void Window_DisableRawMouse(void) { 
	DefaultDisableRawMouse();
}

