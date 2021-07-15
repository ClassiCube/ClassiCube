#include "Window.h"
#include "Input.h"
#include "Event.h"
#include "Logger.h"

struct _DisplayData DisplayInfo;
struct _WinData WindowInfo;

int Display_ScaleX(int x) { return (int)(x * DisplayInfo.ScaleX); }
int Display_ScaleY(int y) { return (int)(y * DisplayInfo.ScaleY); }

#if defined CC_BUILD_IOS
/* iOS implements these functions in external interop_ios.m file */
#define CC_MAYBE_OBJC1 extern
#define CC_MAYBE_OBJC2 extern
#define CC_OBJC_VISIBLE
#elif defined CC_BUILD_COCOA
/* Cocoa implements some functions in external interop_cocoa.m file */
#define CC_MAYBE_OBJC1 extern
#define CC_MAYBE_OBJC2 static
#define CC_OBJC_VISIBLE
#else
/* All other platforms implement internally in this file */
#define CC_MAYBE_OBJC1 static
#define CC_MAYBE_OBJC2 static
#define CC_OBJC_VISIBLE static
#endif


static int cursorPrevX, cursorPrevY;
static cc_bool cursorVisible = true;
/* Gets the position of the cursor in screen or window coordinates. */
CC_MAYBE_OBJC1 void Cursor_GetRawPos(int* x, int* y);
CC_MAYBE_OBJC2 void Cursor_DoSetVisible(cc_bool visible);

void Cursor_SetVisible(cc_bool visible) {
	if (cursorVisible == visible) return;
	cursorVisible = visible;
	Cursor_DoSetVisible(visible);
}

static void CentreMousePosition(void) {
	Cursor_SetPosition(WindowInfo.Width / 2, WindowInfo.Height / 2);
	/* Fixes issues with large DPI displays on Windows >= 8.0. */
	Cursor_GetRawPos(&cursorPrevX, &cursorPrevY);
}

static void RegrabMouse(void) {
	if (!WindowInfo.Focused || !WindowInfo.Exists) return;
	CentreMousePosition();
}

static void DefaultEnableRawMouse(void) {
	Input_RawMode = true;
	RegrabMouse();
	Cursor_SetVisible(false);
}

static void DefaultUpdateRawMouse(void) {
	int x, y;
	Cursor_GetRawPos(&x, &y);
	Event_RaiseRawMove(&PointerEvents.RawMoved, x - cursorPrevX, y - cursorPrevY);
	CentreMousePosition();
}

static void DefaultDisableRawMouse(void) {
	Input_RawMode = false;
	RegrabMouse();
	Cursor_SetVisible(true);
}

/* The actual windowing system specific method to display a message box */
CC_MAYBE_OBJC1 void ShowDialogCore(const char* title, const char* msg);
void Window_ShowDialog(const char* title, const char* msg) {
	/* Ensure cursor is visible while showing message box */
	cc_bool visible = cursorVisible;

	if (!visible) Cursor_SetVisible(true);
	ShowDialogCore(title, msg);
	if (!visible) Cursor_SetVisible(false);
}

void OpenKeyboardArgs_Init(struct OpenKeyboardArgs* args, STRING_REF const cc_string* text, int type) {
	args->text = text;
	args->type = type;
	args->placeholder = "";
}


struct GraphicsMode { int R, G, B, A, IsIndexed; };
/* Creates a GraphicsMode compatible with the default display device */
static void InitGraphicsMode(struct GraphicsMode* m) {
	int bpp = DisplayInfo.Depth;
	m->IsIndexed = bpp < 15;

	m->A = 0;
	switch (bpp) {
	case 32:
		m->R = 8; m->G = 8; m->B = 8; m->A = 8; break;
	case 24:
		m->R = 8; m->G = 8; m->B = 8; break;
	case 16:
		m->R = 5; m->G = 6; m->B = 5; break;
	case 15:
		m->R = 5; m->G = 5; m->B = 5; break;
	case 8:
		m->R = 3; m->G = 3; m->B = 2; break;
	case 4:
		m->R = 2; m->G = 2; m->B = 1; break;
	default:
		/* mode->R = 0; mode->G = 0; mode->B = 0; */
		Logger_Abort2(bpp, "Unsupported bits per pixel"); break;
	}
}
