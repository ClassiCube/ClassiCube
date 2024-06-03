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
static WindowPtr win;


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

// ==================================== IMPORTS FROM DISKINIT.H ====================================
// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(void) DILoad(void)                                MAC_THREEWORDINLINE(0x7002, 0x3F00, 0xA9E9);

// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(void) DIUnload(void)                              MAC_THREEWORDINLINE(0x7004, 0x3F00, 0xA9E9);

// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(short) DIBadMount(Point where, UInt32 evtMessage) MAC_THREEWORDINLINE(0x7000, 0x3F00, 0xA9E9);


// ==================================== IMPORTS FROM SCRAP.H ====================================
// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(long) GetScrap(Handle dst, MAC_FourCharCode type, SInt32* offset)         MAC_ONEWORDINLINE(0xA9FD);

// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(OSStatus) PutScrap(SInt32 srcLen, MAC_FourCharCode type, const void* src) MAC_ONEWORDINLINE(0xA9FE);


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { }
void Window_Init(void) {
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

    win = NewWindow(NULL, &r, "\pClassiCube", true, 0, (WindowPtr)-1, false, 0);
	SetPort(win);
	r = win->portRect;
	
	Window_Main.Width   = r.right  - r.left;
	Window_Main.Height  = r.bottom - r.top;
	Window_Main.Focused = true;
	Window_Main.Exists  = true;
}

void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

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
	// TODO
}


static void HandleMouseDown(EventRecord* event) {
	WindowPartCode part;
	WindowPtr      window;
	Point localPoint;
                    
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

			x = localPoint.h;
			y = localPoint.v;
			Pointer_SetPosition(0, x, y);
			Input_SetPressed(CCMOUSE_L);
			break;
		case inDrag:
			DragWindow(window, event->where, &qd.screenBits.bounds);
			break;
		case inGoAway:
			if (TrackGoAway(window, event->where))
 				Window_RequestClose();
	}
}


static void HandleMouseUp(EventRecord* event) {
	Input_SetReleased(CCMOUSE_L);
}

static void HandleKeyPress(EventRecord* event) {
	if ((event->modifiers & cmdKey))
		HiliteMenu(0);
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
                HandleKeyPress(&event);
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

short isPressed(unsigned short k) {
	unsigned char km[16];

	GetKeys((long *)km);
	return ((km[k>>3] >> (k&7) ) &1);
}

void Window_ProcessGamepads(float delta) {
	Gamepad_SetButton(0, CCPAD_UP,			isPressed(0x0D));
	Gamepad_SetButton(0, CCPAD_DOWN,		isPressed(0x01));
	Gamepad_SetButton(0, CCPAD_START,		isPressed(0x24));
}

static void Cursor_GetRawPos(int* x, int* y) {
	// TODO
	*x=0;*y=0;
}

void Cursor_SetPosition(int x, int y) { 
	// TODO
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
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	// TODO
	return 1;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	// TODO
	return 1;
}

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
    GrafPtr thePort = (CGrafPtr)win;
	SetPort(win);
	int ww = bmp->width;
	int hh = bmp->height;

    // Iterate through each pixel
    for (int y = r.y; y < r.y + r.height; ++y) {
        BitmapCol* row = Bitmap_GetRow(bmp, y);
        for (int x = r.x; x < r.x + r.width; ++x) {

            // TODO optimise
            BitmapCol	col = row[x];
			cc_uint8 R = BitmapCol_R(col);
			cc_uint8 G = BitmapCol_G(col);
			cc_uint8 B = BitmapCol_B(col);

            // Set the pixel color in the window
            RGBColor	pixelColor;
						pixelColor.red   = R * 256;
						pixelColor.green = G * 256;
						pixelColor.blue  = B * 256;
            RGBForeColor(&pixelColor);
            MoveTo(x, y);
            Line(0, 0);
//SetCPixel(x, y, &pixelColor);
        }
    }
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Draw2D(Rect2D* r, struct Bitmap* bmp) { }
void OnscreenKeyboard_Draw3D(void) { }
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
#if (CC_GFX_BACKEND == CC_GFX_BACKEND_GL) && !defined CC_BUILD_EGL
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

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	// TODO
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
