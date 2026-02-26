#include "Core.h"
#if CC_WIN_BACKEND == CC_WIN_BACKEND_TERMINAL
#include "_WindowBase.h"
#include "String_.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"
#include "Utils.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef CC_BUILD_WIN
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <sys/ioctl.h>
#endif

#ifdef CC_BUILD_LINUX
#include <sys/kd.h>
#include <linux/keyboard.h>
#endif


/*########################################################################################################################*
*------------------------------------------------------Console output-----------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
	#define OutputConsole(buf, len) WriteConsoleA(hStdout, buf, len, NULL, NULL)
	#define BOX_CHAR "\xE2\x96\x84"
#else
	#define OutputConsole(buf, len) !!write(STDOUT_FILENO, buf, len)
	#define BOX_CHAR "\xE2\x96\x84"
#endif

#ifdef CC_BUILD_MACOS
	// iTerm only displays trucolour properly with :
	#define SEP_STR  ":"
	#define SEP_CHAR ':'
#else
	#define SEP_STR  ";"
	#define SEP_CHAR ';'
#endif

static void SetMousePosition(int x, int y);
static cc_bool pendingResize, pendingClose;
static int supportsTruecolor;
#define CHARS_PER_CELL 2
#define CSI "\x1B["

#define ERASE_CMD(cmd)	  CSI cmd "J"
#define DEC_PM_SET(cmd)   CSI "?" cmd "h"
#define DEC_PM_RESET(cmd) CSI "?" cmd "l"

#define OutputConst(str) OutputConsole(str, sizeof(str) - 1)


/*########################################################################################################################*
*------------------------------------------------------Terminal backend----------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
static HANDLE hStdin, hStdout;
static DWORD inOldMode, outOldMode;

static void UpdateDimensions(void) {
	CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
    int cols, rows;

    GetConsoleScreenBufferInfo(hStdout, &csbi);
    cols = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
	Platform_Log2("RESIZE: %i, %i", &cols, &rows);

	DisplayInfo.Width  = cols;
	DisplayInfo.Height = rows * CHARS_PER_CELL;
	Window_Main.Width  = DisplayInfo.Width;
	Window_Main.Height = DisplayInfo.Height;
}

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING 
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static void HookTerminal(void) {
	hStdin  = GetStdHandle(STD_INPUT_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	
	GetConsoleMode(hStdin,  &inOldMode);
	GetConsoleMode(hStdout, &outOldMode);
	SetConsoleOutputCP(CP_UTF8);

	// https://stackoverflow.com/questions/37069599/cant-read-mouse-event-use-readconsoleinput-in-c
	SetConsoleMode(hStdin,  ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
	SetConsoleMode(hStdout, ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
	supportsTruecolor = true;
}

static void UnhookTerminal(void) {
	SetConsoleMode(hStdin,  inOldMode);
	SetConsoleMode(hStdout, outOldMode);
}

static BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) pendingClose = true;
    return true;
}

static void sigterm_handler(int sig) { pendingClose = true; UnhookTerminal(); }

static void HookSignals(void) {
	SetConsoleCtrlHandler(consoleHandler, TRUE);
	
	signal(SIGTERM,  sigterm_handler);
	signal(SIGINT,   sigterm_handler);
}
#else
// Inspired from https://github.com/Cubified/tuibox/blob/main/tuibox.h#L606
// Uses '▄' to double the vertical resolution
// (this trick was inspired from https://github.com/ichinaski/pxl/blob/master/main.go#L30)
static struct termios tio;
static struct winsize ws;
#ifdef CC_BUILD_LINUX
static int orig_KB = K_XLATE;
#endif

static void UpdateDimensions(void) {
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

	DisplayInfo.Width  = ws.ws_col;
	DisplayInfo.Height = ws.ws_row * CHARS_PER_CELL;
	Window_Main.Width  = DisplayInfo.Width;
	Window_Main.Height = DisplayInfo.Height;
}

static void HookTerminal(void) {
	struct termios raw;
	
	tcgetattr(STDIN_FILENO, &tio);
	raw = tio;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	
	// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Normal-tracking-mode
	OutputConst(DEC_PM_SET("1049"));
	OutputConst(CSI "0m");
	OutputConst(ERASE_CMD("2")); // Ps = 2  ⇒  Erase All.
	OutputConst(DEC_PM_SET("1003")); // Ps = 1 0 0 3  ⇒  Use All Motion Mouse Tracking, xterm.  See
	OutputConst(DEC_PM_SET("1015")); // Ps = 1 0 1 5  ⇒  Enable urxvt Mouse Mode.
	OutputConst(DEC_PM_SET("1006")); // Ps = 1 0 0 6  ⇒  Enable SGR Mouse Mode, xterm.
	OutputConst(DEC_PM_RESET("25")); // Ps = 2 5  ⇒  Show cursor (DECTCEM), VT220.

	supportsTruecolor = true;
}

static void UnhookTerminal(void) {
	//ioctl(STDIN_FILENO, KDSKBMODE, orig_KB);	
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);
	
	OutputConst(DEC_PM_RESET("1049")); // Return to Normal Screen Buffer and restore cursor
	OutputConst(CSI "0m");
	OutputConst(ERASE_CMD("2")); // Ps = 2  ⇒  Erase All.
	OutputConst(DEC_PM_SET("25"));
	OutputConst(DEC_PM_RESET("1003"));
	OutputConst(DEC_PM_RESET("1015"));
	OutputConst(DEC_PM_RESET("1006"));
}

static void sigwinch_handler(int sig) { pendingResize = true; }
static void sigterm_handler(int sig)  { pendingClose  = true; UnhookTerminal(); }

static void HookSignals(void) {
	signal(SIGWINCH, sigwinch_handler);
	signal(SIGTERM,  sigterm_handler);
	signal(SIGINT,   sigterm_handler);
}
#endif


/*########################################################################################################################*
*---------------------------------------------------------Input backend---------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
static const cc_uint8 key_map[] = {
/* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 
/* 08 */ CCKEY_BACKSPACE, CCKEY_TAB, 0, 0, CCKEY_F5, CCKEY_ENTER, 0, 0,
/* 10 */ 0, 0, 0, CCKEY_PAUSE, CCKEY_CAPSLOCK, 0, 0, 0, 
/* 18 */ 0, 0, 0, CCKEY_ESCAPE, 0, 0, 0, 0,
/* 20 */ CCKEY_SPACE, CCKEY_PAGEUP, CCKEY_PAGEDOWN, CCKEY_END, CCKEY_HOME, CCKEY_LEFT, CCKEY_UP, CCKEY_RIGHT, 
/* 28 */ CCKEY_DOWN, 0, CCKEY_PRINTSCREEN, 0, CCKEY_PRINTSCREEN, CCKEY_INSERT, CCKEY_DELETE, 0,
/* 30 */ '0', '1', '2', '3', '4', '5', '6', '7', 
/* 38 */ '8', '9', 0, 0, 0, 0, 0, 0,
/* 40 */ 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 
/* 48 */ 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
/* 50 */ 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 
/* 58 */ 'X', 'Y', 'Z', CCKEY_LWIN, CCKEY_RWIN, CCKEY_MENU, 0, CCKEY_SLEEP,
/* 60 */ CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP7, 
/* 68 */ CCKEY_KP8, CCKEY_KP9, CCKEY_KP_MULTIPLY, CCKEY_KP_PLUS, 0, CCKEY_KP_MINUS, CCKEY_KP_DECIMAL, CCKEY_KP_DIVIDE,
/* 70 */ CCKEY_F1, CCKEY_F2, CCKEY_F3, CCKEY_F4, CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F8, 
/* 78 */ CCKEY_F9, CCKEY_F10, CCKEY_F11, CCKEY_F12, CCKEY_F13, CCKEY_F14, CCKEY_F15, CCKEY_F16,
/* 80 */ CCKEY_F17, CCKEY_F18, CCKEY_F19, CCKEY_F20, CCKEY_F21, CCKEY_F22, CCKEY_F23, CCKEY_F24, 
/* 88 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* 90 */ CCKEY_NUMLOCK, CCKEY_SCROLLLOCK, 0, 0, 0, 0, 0, 0, 
/* 98 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* A0 */ CCKEY_LSHIFT, CCKEY_RSHIFT, CCKEY_LCTRL, CCKEY_RCTRL, CCKEY_LALT, CCKEY_RALT, CCKEY_BROWSER_PREV, CCKEY_BROWSER_NEXT, 
/* A8 */ CCKEY_BROWSER_REFRESH, CCKEY_BROWSER_STOP, CCKEY_BROWSER_SEARCH, CCKEY_BROWSER_FAVORITES, CCKEY_BROWSER_HOME, CCKEY_VOLUME_MUTE, CCKEY_VOLUME_DOWN, CCKEY_VOLUME_UP,
/* B0 */ CCKEY_MEDIA_NEXT, CCKEY_MEDIA_PREV, CCKEY_MEDIA_STOP, CCKEY_MEDIA_PLAY, CCKEY_LAUNCH_MAIL, CCKEY_LAUNCH_MEDIA, CCKEY_LAUNCH_APP1, CCKEY_LAUNCH_CALC, 
/* B8 */ 0, 0, CCKEY_SEMICOLON, CCKEY_EQUALS, CCKEY_COMMA, CCKEY_MINUS, CCKEY_PERIOD, CCKEY_SLASH,
/* C0 */ CCKEY_TILDE, 0, 0, 0, 0, 0, 0, 0, 
/* C8 */ 0, 0, 0, 0, 0, 0, 0, 0,
/* D0 */ 0, 0, 0, 0, 0, 0, 0, 0, 
/* D8 */ 0, 0, 0, CCKEY_LBRACKET, CCKEY_BACKSLASH, CCKEY_RBRACKET, CCKEY_QUOTE, 0,
};

// TODO lshift, rshift
static int MapNativeKey(DWORD vk_key) {
	int key = vk_key < Array_Elems(key_map) ? key_map[vk_key] : 0;
	if (!key) Platform_Log1("Unknown key: %x", &vk_key);
	return key;
}

static void KeyEventProc(KEY_EVENT_RECORD ker) {
	int key = MapNativeKey(ker.wVirtualKeyCode);
	int uni = ker.uChar.UnicodeChar;

	if (ker.bKeyDown) {
		Input_SetPressed(key);
		if (uni) Event_RaiseInt(&InputEvents.Press, (cc_unichar)uni);
	} else {
		Input_SetReleased(key);
	}
}

static void MouseEventProc(MOUSE_EVENT_RECORD mer) {
	switch (mer.dwEventFlags)
	{
		case 0:
		case DOUBLE_CLICK:
			Input_Set(CCMOUSE_L, mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED);
			Input_Set(CCMOUSE_R, mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED);
			// TODO other mouse buttons
			break;
		case MOUSE_MOVED:
			SetMousePosition(mer.dwMousePosition.X, mer.dwMousePosition.Y * CHARS_PER_CELL);
			break;
		case MOUSE_WHEELED:
			Mouse_ScrollVWheel((int)mer.dwButtonState > 0 ? 1 : -1);
			break;
		default:
			Platform_LogConst("unknown mouse event");
			break;
	}
}

static void ProcessConsoleEvents(float delta) {
	DWORD events = 0;
	GetNumberOfConsoleInputEvents(hStdin, &events);
	if (!events) return;
	
	INPUT_RECORD buffer[128];
	if (!ReadConsoleInput(hStdin, buffer, 128, &events)) return;

	for (int i = 0; i < events; i++)
	{
		switch (buffer[i].EventType)
		{
			case KEY_EVENT:
				KeyEventProc(buffer[i].Event.KeyEvent);
				break;
			case MOUSE_EVENT:
				MouseEventProc(buffer[i].Event.MouseEvent);
				break;
			case WINDOW_BUFFER_SIZE_EVENT:
				pendingResize = true;
				break;
		}
	}
}

#else
static int MapNativeMouse(int button) {
	if (button == 1) return CCMOUSE_L;
	if (button == 2) return CCMOUSE_M;
	if (button == 3) return CCMOUSE_R;

	if (button == 8) return CCMOUSE_X1;
	if (button == 9) return CCMOUSE_X2;

	/* Mouse horizontal and vertical scroll */
	if (button >= 4 && button <= 7) return 0;
	Platform_Log1("Unknown mouse button: %i", &button);
	return 0;
}

static int stdin_available(void) {
	struct pollfd pfd;
	pfd.fd	 = STDIN_FILENO;
	pfd.events = POLLIN;

	if (poll(&pfd, 1, 0)) {
		if (pfd.revents & POLLIN) return 1;
	}
	return 0;
}

static void UpdatePointerPosition(char* tok) {
	int x, y;
	tok = strtok(NULL, ";");
	x   = atoi(tok);
	tok = strtok(NULL, ";");
	y   = atoi(tok) * CHARS_PER_CELL;

	SetMousePosition(x, y);
}

static void ProcessMouse(char* buf, int n) {
	char cpy[256 + 2];
	strncpy(cpy, buf, n);
	char* tok = strtok(cpy + 3, ";");
	int mouse;
	if (!tok) return;

	switch (tok[0]) {
	case '0':
		mouse = strchr(buf, 'm') == NULL;
		UpdatePointerPosition(tok);
		Input_SetNonRepeatable(CCMOUSE_L, mouse);
		break;
	case '1':
		mouse = strchr(buf, 'm') == NULL;
		UpdatePointerPosition(tok);
		Input_SetNonRepeatable(CCMOUSE_M, mouse);
		break;
	case '2':
		mouse = strchr(buf, 'm') == NULL;
		UpdatePointerPosition(tok);
		Input_SetNonRepeatable(CCMOUSE_R, mouse);
		break;
	case '3':
		mouse = (strcmp(tok, "32") == 0);
		UpdatePointerPosition(tok);
		break;
	}
}

static int MapKey(int key) {
	if (key == ' ') return CCKEY_SPACE;
	
	if (key >= 'a' && key <= 'z') key -= 32;
	if (key >= 'A' && key <= 'Z') return key;
	
	Platform_Log1("Unknown key: %i", &key);
	return 0;
}

static float event_time;
static float press_start[256];
static void ProcessKey(int raw) {
	int key = MapKey(raw);
	if (key) {
		Input_SetPressed(key);
		press_start[raw] = event_time;
	}
	
	if (raw >= 32 && raw < 127) {
		Event_RaiseInt(&InputEvents.Press, raw);
	}
}

static void ProcessConsoleInput(void) {
	char buf[256];

	int n = read(STDIN_FILENO, buf, sizeof(buf));
	int A = buf[0];
	//Platform_Log2("IN: %i, %i", &n, &A);

	if (n >= 4 && buf[0] == '\x1b' && buf[1] == '[' && buf[2] == '<') {
		ProcessMouse(buf, n);
	} else if (buf[0] >= 32 && buf[0] < 127) {
		ProcessKey(buf[0]);
	}
}

static void ProcessConsoleEvents(float delta) {
	if (stdin_available()) ProcessConsoleInput();
	
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
#endif


/*########################################################################################################################*
*-------------------------------------------------------Window common-----------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { 
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	Input.Sources = INPUT_SOURCE_NORMAL;
	DisplayInfo.Depth  = 4;
	DisplayInfo.ScaleX = 0.5f;
	DisplayInfo.ScaleY = 0.5f;
	
	//ioctl(STDIN_FILENO , KDGKBMODE, &orig_KB);
	//ioctl(STDIN_FILENO,  KDSKBMODE, K_MEDIUMRAW);
	HookTerminal();
	UpdateDimensions();
	HookSignals();
	Platform_Flags |= PLAT_FLAG_SINGLE_PROCESS;
}

void Window_Free(void) {
	UnhookTerminal();
}

static void DoCreateWindow(int width, int height) {
	Window_Main.Exists   = true;
	Window_Main.Focused  = true;
	
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) {
	// TODO
}

void Clipboard_GetText(cc_string* value) {
	// TODO
}

void Clipboard_SetText(const cc_string* value) {
	// TODO
}

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

void Window_SetSize(int width, int height) {
	// TODO
}

void Window_RequestClose(void) {
	pendingClose = true;
}

void Window_ProcessEvents(float delta) {
	if (pendingResize) {
		pendingResize = false;
		UpdateDimensions();
		Event_RaiseVoid(&WindowEvents.Resized);
	}
	
	if (pendingClose) {
		pendingClose = false;
		Window_Main.Exists = false;
		Event_RaiseVoid(&WindowEvents.Closing);
		return;
	}
	
	ProcessConsoleEvents(delta);
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }

static int mouseX, mouseY;
static void SetMousePosition(int x, int y) {
	mouseX = x;
	mouseY = y;
	Pointer_SetPosition(0, x, y);
}

static void Cursor_GetRawPos(int* x, int* y) {
	*x = mouseX;
	*y = mouseY;
}

void Cursor_SetPosition(int x, int y) {
	// TODO
}

static void Cursor_DoSetVisible(cc_bool visible) {
	// TODO
}


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


/*########################################################################################################################*
*-------------------------------------------------------Console output-----------------------------------------------------*
*#########################################################################################################################*/
// TODO still wrong
static void AppendByteFast(cc_string* str, int value) {
	if (value >= 100) { 
		String_Append(str, '0' + (value / 100)); value %= 100;
		String_Append(str, '0' + (value /  10)); value %=  10;
	} else if (value >=  10) { 
		String_Append(str, '0' + (value /  10)); value %=  10; 
	}
	String_Append(str, '0' + value);
}

static int Index256(int value) {
	if (value <= 0x5F) return value;
	// Add 20 to round to nearest
	return (value - 0x5F + 20) / 40;
}

static int CalcIndex(BitmapCol rgb) {
	int r = Index256(BitmapCol_R(rgb));
	int g = Index256(BitmapCol_G(rgb));
	int b = Index256(BitmapCol_B(rgb));

	return 16 + 36 * r + 6 * g + b;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	char buf[256];
	cc_string str;
	int len;
	String_InitArray(str, buf);
	
	for (int y = r.y & ~0x01; y < r.y + r.height; y += 2)
	{
		//len = sprintf(buf, CSI "%i;%iH", y / 2, r.x); // move cursor to start
		//OutputConsole(buf, len);
		str.length = 0;
		String_AppendConst(&str, CSI);
		String_AppendInt(  &str, y / CHARS_PER_CELL);
		String_Append(     &str, ';');
		String_AppendInt(  &str, r.x);
		String_Append(     &str, 'H');
		OutputConsole(buf, str.length);
		
		for (int x = r.x; x < r.x + r.width; x++)
		{
			BitmapCol top = Bitmap_GetPixel(bmp, x, y + 0);
			BitmapCol bot = Bitmap_GetPixel(bmp, x, y + 1);
	
			// Use '▄' so each cell can use a background and foreground colour
			// This essentially doubles the vertical resolution of the displayed image
			//printf(CSI "48;2;%i;%i;%im", BitmapCol_R(top), BitmapCol_G(top), BitmapCol_B(top));
			//printf(CSI "38;2;%i;%i;%im", BitmapCol_R(bot), BitmapCol_G(bot), BitmapCol_B(bot));
			//printf("\xE2\x96\x84");
			//len = sprintf(buf, CSI "48;2;%i;%i;%im" CSI "38;2;%i;%i;%im" BOX_CHAR, 
			//				BitmapCol_R(top), BitmapCol_G(top), BitmapCol_B(top),
			//				BitmapCol_R(bot), BitmapCol_G(bot), BitmapCol_B(bot));
			
			// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
			str.length = 0;
			if (supportsTruecolor) {
				String_AppendConst(&str, CSI "48" SEP_STR "2" SEP_STR);
				String_AppendInt(  &str, BitmapCol_R(top));
				String_Append(     &str, SEP_CHAR);
				String_AppendInt(  &str, BitmapCol_G(top));
				String_Append(     &str, SEP_CHAR);
				String_AppendInt(  &str, BitmapCol_B(top));
				String_Append(     &str, 'm');
				
				String_AppendConst(&str, CSI "38" SEP_STR "2" SEP_STR);
				String_AppendInt(  &str, BitmapCol_R(bot));
				String_Append(     &str, SEP_CHAR);
				String_AppendInt(  &str, BitmapCol_G(bot));
				String_Append(     &str, SEP_CHAR);
				String_AppendInt(  &str, BitmapCol_B(bot));
				String_Append(     &str, 'm');
			} else {
				String_AppendConst(&str, CSI "48" SEP_STR "5" SEP_STR);
				String_AppendInt(  &str, CalcIndex(top));
				String_Append(     &str, 'm');
				
				String_AppendConst(&str, CSI "38" SEP_STR "5" SEP_STR);
				String_AppendInt(  &str, CalcIndex(bot));
				String_Append(     &str, 'm');
			}
			
			String_AppendConst(&str, BOX_CHAR);
			OutputConsole(buf, str.length);
		}		
	}
}
#endif
