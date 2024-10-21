#include "Core.h"
#if defined CC_BUILD_MACCLASSIC

#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"

#undef true
#undef false
#include <Quickdraw.h>
#include <Dialogs.h>
#include <Fonts.h>
#include <Events.h>
#include <LowMem.h>
#ifndef M68K_INLINE
#include <DiskInit.h>
#include <Scrap.h>
#endif
#include <Gestalt.h>
static WindowPtr win;
static cc_bool hasColorQD, useGWorld;


/*########################################################################################################################*
*--------------------------------------------------Console log window-----------------------------------------------------*
*#########################################################################################################################*/
static int con_cellSizeX, con_cellSizeY;
static int con_rows, con_cols;
static int cursorX, cursorY;
static WindowPtr con_win;
static Rect con_bounds;

static void Console_EraseLine(int y) {
	Rect r   = con_bounds;
	r.top    += y * con_cellSizeY;
	r.bottom = r.top + con_cellSizeY;

	MoveTo(r.left, r.bottom - 2);
	EraseRect(&r);
}

static void Console_Init(void) {
	Rect r = qd.screenBits.bounds;
	r.top += 40;
	InsetRect(&r, 5, 5);  

	con_win = NewWindow(NULL, &r, "\pConsole log", true, 0, (WindowPtr)-1, true, 0);
	GrafPtr savedPort;
	GetPort(&savedPort);
	SetPort(con_win);

	con_bounds = con_win->portRect;
	EraseRect(&con_bounds);

	TextFont(kFontIDMonaco);
	TextSize(9);

	InsetRect(&con_bounds, 2, 2);
	con_cellSizeX = CharWidth('M');
	con_cellSizeY = 12;

	con_rows = (con_bounds.bottom - con_bounds.top)  / con_cellSizeY;
	con_cols = (con_bounds.right  - con_bounds.left) / con_cellSizeX;

	Console_EraseLine(0);
	cursorX = cursorY = 0;
	SetPort(savedPort);
}

static void Console_NewLine(void) {
	Console_EraseLine(cursorY);
	cursorY++;
	cursorX = 0;
	if (cursorY >= con_rows) cursorY = 0;
}

void Console_Write(const char* msg, int len) {
	if (!con_win) Console_Init();

	GrafPtr savedPort;
	GetPort(&savedPort);
	SetPort(con_win);

	for (int i = 0; i < len; i++) 
	{
		DrawChar(msg[i]);
		cursorX++;
		if (cursorX >= con_cols) Console_NewLine();
	}
	Console_NewLine();

	SetPort(savedPort);
}


/*########################################################################################################################*
*---------------------------------------------------Imported headers------------------------------------------------------*
*#########################################################################################################################*/
// On 68k these are implemented using direct 68k opcodes
// On PPC these are implemented using function calls
#if TARGET_CPU_68K
	#define MAC_SYSAPI(_type) static _type
    #define MAC_ONEWORDINLINE(w1)           = w1
    #define MAC_TWOWORDINLINE(w1,w2)        = {w1, w2}
    #define MAC_THREEWORDINLINE(w1,w2,w3)   = {w1, w2, w3}
    #define MAC_FOURWORDINLINE(w1,w2,w3,w4) = {w1, w2, w3, w4}
#else
	#define MAC_SYSAPI(_type) extern pascal _type
    #define MAC_ONEWORDINLINE(w1)
    #define MAC_TWOWORDINLINE(w1,w2)
    #define MAC_THREEWORDINLINE(w1,w2,w3)
    #define MAC_FOURWORDINLINE(w1,w2,w3,w4)
#endif
typedef unsigned long MAC_FourCharCode;
typedef SInt16 MAC_WindowPartCode;
typedef UInt16 MAC_EventMask;

// Workaround issue in multiversal headers
#if defined M68K_INLINE && TARGET_CPU_68K

// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(void) _SetEventMask(MAC_EventMask value) MAC_TWOWORDINLINE(0x31DF, 0x0144);
#define SetEventMask _SetEventMask
#endif

/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
static const cc_uint8 key_map[8 * 16] = {
/* 0x00 */ 'A', 'S', 'D', 'F', 'H', 'G', 'Z', 'X',
/* 0x08 */ 'C', 'V',   0, 'B', 'Q', 'W', 'E', 'R',
/* 0x10 */ 'Y', 'T', '1', '2', '3', '4', '6', '5',
/* 0x18 */ CCKEY_EQUALS, '9', '7', CCKEY_MINUS, '8', '0', CCKEY_RBRACKET, 'O',
/* 0x20 */ 'U', CCKEY_LBRACKET, 'I', 'P', CCKEY_ENTER, 'L', 'J', CCKEY_QUOTE,
/* 0x28 */ 'K', CCKEY_SEMICOLON, CCKEY_BACKSLASH, CCKEY_COMMA, CCKEY_SLASH, 'N', 'M', CCKEY_PERIOD,
/* 0x30 */ CCKEY_TAB, CCKEY_SPACE, CCKEY_TILDE, CCKEY_BACKSPACE, 0, CCKEY_ESCAPE, 0, CCKEY_LWIN,
/* 0x38 */ CCKEY_LSHIFT, CCKEY_CAPSLOCK, CCKEY_LALT, CCKEY_LCTRL, 0, 0, 0, 0,
/* 0x40 */ 0, CCKEY_KP_DECIMAL, 0, CCKEY_KP_MULTIPLY, 0, CCKEY_KP_PLUS, 0, CCKEY_NUMLOCK,
/* 0x48 */ CCKEY_VOLUME_UP, CCKEY_VOLUME_DOWN, CCKEY_VOLUME_MUTE, CCKEY_KP_DIVIDE, CCKEY_KP_ENTER, 0, CCKEY_KP_MINUS, 0,
/* 0x50 */ 0, CCKEY_KP_ENTER, CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5,
/* 0x58 */ CCKEY_KP6, CCKEY_KP7, 0, CCKEY_KP8, CCKEY_KP9, 'N', 'M', CCKEY_PERIOD,
/* 0x60 */ CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F3, CCKEY_F8, CCKEY_F9, 0, CCKEY_F11,
/* 0x68 */ 0, CCKEY_F13, 0, CCKEY_F14, 0, CCKEY_F10, 0, CCKEY_F12,
/* 0x70 */ 'U', CCKEY_F15, CCKEY_INSERT, CCKEY_HOME, CCKEY_PAGEUP, CCKEY_DELETE, CCKEY_F4, CCKEY_END,
/* 0x78 */ CCKEY_F2, CCKEY_PAGEDOWN, CCKEY_F1, CCKEY_LEFT, CCKEY_RIGHT, CCKEY_DOWN, CCKEY_UP, 0,
};
static int MapNativeKey(UInt32 key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }

void Window_PreInit(void) {
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	InitDialogs(NULL);
	InitCursor();

	EventRecord event;
	for (int i = 0; i < 5; i++)
		EventAvail(everyEvent, &event);
	FlushEvents(everyEvent, 0);
	SetEventMask(everyEvent);

    long tmpLong = 0;
    Gestalt(gestaltQuickdrawVersion, &tmpLong);
    hasColorQD = tmpLong >= gestalt32BitQD;
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	Rect r = qd.screenBits.bounds;
	DisplayInfo.x      = r.left;
	DisplayInfo.y      = r.top;
	DisplayInfo.Width  = r.right - r.left;
	DisplayInfo.Height = r.bottom - r.top;

	DisplayInfo.ScaleX = 1.0f;
	DisplayInfo.ScaleY = 1.0f;
}

void Window_Free(void) { }

static void DoCreateWindow(int width, int height) {
	if (Window_Main.Exists) return;

	Rect r = qd.screenBits.bounds;
    r.top += 40;
    InsetRect(&r, 100, 100);

	if (hasColorQD) {
    	win = NewCWindow(NULL, &r, "\pClassiCube", true, documentProc, (WindowPtr)-1, true, 0);
	} else {
		win = NewWindow( NULL, &r, "\pClassiCube", true, documentProc, (WindowPtr)-1, true, 0);
	}

	if (hasColorQD) {
		Platform_LogConst("BLITTING IN FAST MODE");
	} else {
		Platform_LogConst("BLITTING IN SLOW MODE");
	}

	useGWorld = hasColorQD;
	SetPort(win);
	r = win->portRect;
	
	Window_Main.Width    = r.right  - r.left;
	Window_Main.Height   = r.bottom - r.top;
	Window_Main.Focused  = true;
	
	Window_Main.Exists   = true;
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
	Handle tmp = NewHandle(0);
	HLock(tmp);
	int dataSize = GetScrap(tmp, 'TEXT', 0);
	HUnlock(tmp);

	String_AppendAll(value, (char*)tmp, dataSize);
    DisposeHandle(tmp);
	// TODO
}

void Clipboard_SetText(const cc_string* value) {
	PutScrap(value->length, 'TEXT', value->buffer);
	// TODO
}

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
	// TODO
}

cc_result Window_EnterFullscreen(void) {
	// TODO
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	// TODO
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
	// TODO
}

void Window_SetSize(int width, int height) {
	// TODO
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


static void HandleMouseDown(EventRecord* event) {
	MAC_WindowPartCode part;
	WindowPtr window;
	Point localPoint;
	long res;        
	int x, y;

	part = FindWindow(event->where, &window);
	switch (part)
	{
		 case inMenuBar:
			HiliteMenu(0);
			break;
		case inSysWindow:
			SystemClick(event, window);
			break;
		case inContent:
			SetPt(&localPoint, event->where.h, event->where.v);
			GlobalToLocal(&localPoint);
			if (window != win) break;

			x = localPoint.h;
			y = localPoint.v;
			Pointer_SetPosition(0, x, y);
			Input_SetPressed(CCMOUSE_L);
			break;
		case inDrag:
			DragWindow(window, event->where, &qd.screenBits.bounds);
			break;
		case inGoAway:
			if (TrackGoAway(window, event->where)) {
 				Window_RequestClose();
				Window_Main.Exists = false;
			}
			break;
		case inGrow:
			res = GrowWindow(window, event->where, &qd.screenBits.bounds);
			x   = res & 0xFFFF;
			y   = res >> 16;
			SizeWindow(window, x, y, false);
			if (window != win) break;

			Window_Main.Width  = x;
			Window_Main.Height = y;
			Event_RaiseVoid(&WindowEvents.Resized);
			break;
	}
}

static void HandleMouseUp(EventRecord* event) {
	Input_SetReleased(CCMOUSE_L);
}

static void HandleKeyDown(EventRecord* event) {
	if ((event->modifiers & cmdKey))
		HiliteMenu(0);

	int ch  = event->message & 0xFF;
	int key = (event->message >> 8) & 0xFF;

	if (ch >= 32 && ch <= 127)
		Event_RaiseInt(&InputEvents.Press, (cc_unichar)ch);

	key = MapNativeKey(key);
	if (key) Input_SetPressed(key);
}

static void HandleKeyUp(EventRecord* event) {
	int key = (event->message >> 8) & 0xFF;

	key = MapNativeKey(key);
	if (key) Input_SetReleased(key);
}

void Window_ProcessEvents(float delta) {
	EventRecord	event;
	SystemTask();

	while (GetNextEvent(everyEvent, &event)) {
		switch (event.what)
        {
            case mouseDown:
                HandleMouseDown(&event);
                break;
            case mouseUp:
                HandleMouseUp(&event);
                break;
            case keyDown:
            case autoKey:
                HandleKeyDown(&event);
                break;
            case keyUp:
                HandleKeyUp(&event);
                break;
            case updateEvt:
                BeginUpdate((WindowPtr)event.message);
                EndUpdate(  (WindowPtr)event.message);
				Event_RaiseVoid(&WindowEvents.RedrawNeeded);
                break;
            case diskEvt:
                if ((event.message & 0xFFFF0000) != noErr)
                {
                    Point pt = { 100, 100 };
                    DILoad();
                    DIBadMount(pt, event.message);
                    DIUnload();
                }
                break;
            case kHighLevelEvent:
                AEProcessAppleEvent(&event);
                break;
            default:
                break;
        }
	}
}

void Gamepads_Init(void) {

}

void Gamepads_Process(float delta) { }

static void Cursor_GetRawPos(int* x, int* y) {
	Point point;
	GetMouse(&point);

	*x = point.h;
	*y = point.v;
}

void Cursor_SetPosition(int x, int y) { 
	// TODO
	Point where;
	where.h = x;
	where.v = y;
	//LMSetRawMouseLocation(where);
}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		ShowCursor();
	} else {
		HideCursor();
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	// TODO
	for (int i = 0; i < 20; i++) 
	{
		Platform_LogConst(title);
		Platform_LogConst(msg);
	}
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

static GWorldPtr fb_world;
static PixMapHandle fb_pixmap;

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	if (!useGWorld) {
		bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
		bmp->width  = width;
		bmp->height = height;
		return;
	}

	QDErr err = NewGWorld(&fb_world, 32, &win->portRect, 0, 0, 0);
	if (err != noErr) Process_Abort2(err, "Failed to allocate GWorld");
	
	fb_pixmap = GetGWorldPixMap(fb_world);
	if (!fb_pixmap) Process_Abort("Failed to allocate pixmap");

	LockPixels(fb_pixmap);
	int stride = (*fb_pixmap)->rowBytes & 0x3FFF;

	bmp->scan0  = (BitmapCol*)GetPixBaseAddr(fb_pixmap);
	bmp->width  = stride >> 2;
	bmp->height = height;
}

static void DrawFramebufferBulk(Rect2D r, struct Bitmap* bmp) {
    GrafPtr thePort = (GrafPtr)win;
	BitMap* memBits = &((GrafPtr)fb_world)->portBits;
	BitMap* winBits = &thePort->portBits;

	Rect update;
	update.left   = r.x;
	update.right  = r.x + r.width;
	update.top    = r.y;
	update.bottom = r.y + r.height;

    CopyBits(memBits, winBits, &update, &update, srcCopy, nil);
}

static void DrawFramebufferSlow(Rect2D r, struct Bitmap* bmp) {
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

            RGBColor pixelColor;
			pixelColor.red   = R << 8;
			pixelColor.green = G << 8;
			pixelColor.blue  = B << 8;

            RGBForeColor(&pixelColor);
            MoveTo(x, y);
            Line(0, 0);
			//SetCPixel(x, y, &pixelColor);
        }
    }
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	SetPort(win);

	if (useGWorld) {
		DrawFramebufferBulk(r, bmp);
	} else {
		DrawFramebufferSlow(r, bmp);
	}
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	if (!useGWorld) return;

	UnlockPixels(fb_pixmap);
	DisposeGWorld(fb_world);
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
*-------------------------------------------------------WGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#if CC_GFX_BACKEND_IS_GL() && !defined CC_BUILD_EGL
void GLContext_Create(void) {
	// TODO
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	// TODO
}

void* GLContext_GetAddress(const char* function) {
	return NULL;
}

cc_bool GLContext_SwapBuffers(void) {
	// TODO
	return true;
}

void GLContext_SetVSync(cc_bool vsync) {
	// TODO
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
