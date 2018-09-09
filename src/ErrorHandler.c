#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"
#include "Window.h"
#include "Funcs.h"

static void ErrorHandler_FailCore(ReturnCode result, const char* raw_msg, void* ctx);
static void ErrorHandler_Backtrace(STRING_TRANSIENT String* str, void* ctx);
static void ErrorHandler_Registers(STRING_TRANSIENT String* str, void* ctx);

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <dbghelp.h>

struct StackPointers { UInt64 Instruction, Frame, Stack; };
struct SymbolAndName { IMAGEHLP_SYMBOL64 Symbol; char Name[256]; };

static LONG WINAPI ErrorHandler_UnhandledFilter(struct _EXCEPTION_POINTERS* pInfo) {
	/* TODO: Write processor state to file*/
	char msgBuffer[128 + 1] = { 0 };
	String msg = { msgBuffer, 0, 128 };

	UInt32 code = (UInt32)pInfo->ExceptionRecord->ExceptionCode;
	UInt64 addr = (UInt64)pInfo->ExceptionRecord->ExceptionAddress;
	String_Format2(&msg, "Unhandled exception 0x%y at 0x%x", &code, &addr);

	ErrorHandler_FailCore(0, msg.buffer, pInfo->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void ErrorHandler_Init(const char* logFile) {
	SetUnhandledExceptionFilter(ErrorHandler_UnhandledFilter);
	/* TODO: Open log file */
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
	MessageBoxA(Window_GetWindowHandle(), msg, title, 0);
}

void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	CONTEXT ctx;
	RtlCaptureContext(&ctx);
	ErrorHandler_FailCore(result, raw_msg, &ctx);
}

static Int32 ErrorHandler_GetFrames(CONTEXT* ctx, struct StackPointers* pointers, Int32 max) {
	STACKFRAME64 frame = { 0 };
	frame.AddrPC.Mode     = AddrModeFlat;
	frame.AddrFrame.Mode  = AddrModeFlat;
	frame.AddrStack.Mode  = AddrModeFlat;	
	DWORD type;

#ifdef _M_IX86
	type = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset    = ctx->Eip;
	frame.AddrFrame.Offset = ctx->Ebp;
	frame.AddrStack.Offset = ctx->Esp;
#elif _M_X64
	type = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset    = ctx->Rip;
	frame.AddrFrame.Offset = ctx->Rsp;
	frame.AddrStack.Offset = ctx->Rsp;
#elif _M_IA64
	type = IMAGE_FILE_MACHINE_IA64;
	frame.AddrPC.Offset     = ctx->StIIP;
	frame.AddrFrame.Offset  = ctx->IntSp;
	frame.AddrBStore.Offset = ctx->RsBSP;
	frame.AddrStack.Offset  = ctx->IntSp;
	frame.AddrBStore.Mode   = AddrModeFlat;
#else
	#error "Unknown machine type"
#endif

	HANDLE process = GetCurrentProcess();
	HANDLE thread  = GetCurrentThread();
	Int32 count;
	CONTEXT copy = *ctx;

	for (count = 0; count < max; count++) {		
		if (!StackWalk64(type, process, thread, &frame, &copy, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
		if (!frame.AddrFrame.Offset) break;

		pointers[count].Instruction = frame.AddrPC.Offset;
		pointers[count].Frame       = frame.AddrFrame.Offset;
		pointers[count].Stack       = frame.AddrStack.Offset;
	}
	return count;
}

static void ErrorHandler_Backtrace(STRING_TRANSIENT String* str, void* ctx) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);

	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLength = 255;
	sym.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);

	struct StackPointers pointers[40];
	Int32 frames = ErrorHandler_GetFrames((CONTEXT*)ctx, pointers, 40);

	String_AppendConst(str, "\r\nBacktrace: \r\n");
	UInt32 i;

	for (i = 0; i < frames; i++) {
		Int32 number = i + 1;
		UInt64 addr = (UInt64)pointers[i].Instruction;

		/* TODO: Log ESP and EBP here too */
		/* TODO: SymGetLineFromAddr64 as well? */
		/* TODO: Log module here too */
		if (SymGetSymFromAddr64(process, addr, NULL, &sym.Symbol)) {
			String_Format3(str, "%i) 0x%x - %c\r\n", &number, &addr, sym.Symbol.Name);
		} else {
			String_Format2(str, "%i) 0x%x\r\n", &number, &addr);
		}
	}
	String_AppendConst(str, "\r\n");
}
#elif CC_BUILD_NIX
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

Display* dpy;
unsigned long X11_Col(UInt8 r, UInt8 g, UInt8 b) {
    Colormap cmap = XDefaultColormap(dpy, DefaultScreen(dpy));
    XColor col = { 0 };
    col.red   = r << 8;
    col.green = g << 8;
    col.blue  = b << 8;
    col.flags = DoRed | DoGreen | DoBlue;

    XAllocColor(dpy, cmap, &col);
    return col.pixel;
}

typedef struct {
    Window win;
    GC gc;
    unsigned long white, black, background;
    unsigned long btnBorder, highlight, shadow;
} X11Window;

static void X11Window_Init(X11Window* w) {
    w->black = BlackPixel(dpy, DefaultScreen(dpy));
    w->white = WhitePixel(dpy, DefaultScreen(dpy));
    w->background = X11_Col(206, 206, 206);

    w->btnBorder = X11_Col(60,  60,  60);
    w->highlight = X11_Col(144, 144, 144);
    w->shadow    = X11_Col(49,  49,  49);

    w->win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 100, 100,
                                0, w->black, w->background);
    XSelectInput(dpy, w->win, ExposureMask     | StructureNotifyMask |
                               KeyReleaseMask  | PointerMotionMask |
                               ButtonPressMask | ButtonReleaseMask );

    w->gc = XCreateGC(dpy, w->win, 0, NULL);
    XSetForeground(dpy, w->gc, w->black);
    XSetBackground(dpy, w->gc, w->background);
}

static void X11Window_Free(X11Window* w) {
    XFreeGC(dpy, w->gc);
    XDestroyWindow(dpy, w->win);
}

typedef struct {
    int X, Y, Width, Height;
    int LineHeight, Descent;
    const char* Text;
} X11Textbox;

static void X11Textbox_Measure(X11Textbox* t, XFontStruct* font) {
    String str = String_FromReadonly(t->Text);
    int direction, ascent, descent, end, lines = 0;
    XCharStruct overall;

    for (end = 0; end >= 0; lines++) {
        end = String_IndexOf(&str, '\n', 0);
        Int32 len = end == -1 ? str.length : end;

        XTextExtents(font, str.buffer, len, &direction, &ascent, &descent, &overall);
        t->Width = max(overall.width, t->Width);
        if (end >= 0) str = String_UNSAFE_SubstringAt(&str, end + 1);
    }

    t->LineHeight = ascent + descent;
    t->Descent    = descent;
    t->Height     = t->LineHeight * lines;
}

static void X11Textbox_Draw(X11Textbox* t, X11Window* w) {
    String str = String_FromReadonly(t->Text);
    int end, y = t->Y + t->LineHeight - t->Descent; /* TODO: is -Descent even right? */

    for (end = 0; end >= 0; y += t->LineHeight) {
        end = String_IndexOf(&str, '\n', 0);
        Int32 len = end == -1 ? str.length : end;

        XDrawString(dpy, w->win, w->gc, t->X, y, str.buffer, len);
        if (end >= 0) str = String_UNSAFE_SubstringAt(&str, end + 1);
    }
}

typedef struct {
    int X, Y, Width, Height;
    bool Clicked;
    X11Textbox Text;
} X11Button;

static void X11Button_Draw(X11Button* b, X11Window* w) {
    XSetForeground(dpy, w->gc, w->btnBorder);
    XDrawRectangle(dpy, w->win, w->gc, b->X, b->Y,
                    b->Width, b->Height);

    X11Textbox* t = &b->Text;
    int begX = b->X + 1, endX = b->X + b->Width  - 1;
    int begY = b->Y + 1, endY = b->Y + b->Height - 1;

    if (b->Clicked) {
        XSetForeground(dpy, w->gc, w->highlight);
        XDrawRectangle(dpy, w->win, w->gc, begX, begY,
                        endX - begX, endY - begY);
    } else {
        XSetForeground(dpy, w->gc, w->white);
        XDrawLine(dpy, w->win, w->gc, begX, begY,
                    endX - 1, begY);
        XDrawLine(dpy, w->win, w->gc, begX, begY,
                    begX, endY - 1);

        XSetForeground(dpy, w->gc, w->highlight);
        XDrawLine(dpy, w->win, w->gc, begX + 1, endY - 1,
                    endX - 1, endY - 1);
        XDrawLine(dpy, w->win, w->gc, endX - 1, begY + 1,
                    endX - 1, endY - 1);

        XSetForeground(dpy, w->gc, w->shadow);
        XDrawLine(dpy, w->win, w->gc, begX, endY, endX, endY);
        XDrawLine(dpy, w->win, w->gc, endX, begY, endX, endY);
    }

    XSetForeground(dpy, w->gc, w->black);
    t->X = b->X + b->Clicked + (b->Width  - t->Width)  / 2;
    t->Y = b->Y + b->Clicked + (b->Height - t->Height) / 2;
    X11Textbox_Draw(t, w);
}

static int X11Button_Contains(X11Button* b, int x, int y) {
    return x >= b->X && x < (b->X + b->Width) &&
           y >= b->Y && y < (b->Y + b->Height);
}

static void X11_MessageBox(const char* title, const char* text, X11Window* w) {
    X11Button ok    = { 0 };
    X11Textbox body = { 0 };

    X11Window_Init(w);
    XMapWindow(dpy, w->win);
    XStoreName(dpy, w->win, title);

    Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, w->win, &wmDelete, 1);

    XFontStruct* font = XQueryFont(dpy, XGContextFromGC(w->gc));
    if (!font) return;

    /* Compute size of widgets */
    body.Text = text;
    X11Textbox_Measure(&body, font);
    ok.Text.Text = "OK";
    X11Textbox_Measure(&ok.Text, font);
    ok.Width  = ok.Text.Width  + 70;
    ok.Height = ok.Text.Height + 10;

    /* Compute size and position of window */
    int width  = body.Width                   + 20;
    int height = body.Height + 20 + ok.Height + 20;
    int x = DisplayWidth (dpy, DefaultScreen(dpy))/2 -  width/2;
    int y = DisplayHeight(dpy, DefaultScreen(dpy))/2 - height/2;
    XMoveResizeWindow(dpy, w->win, x, y, width, height);

    /* Adjust bounds of widgets */
    body.X = 10; body.Y = 10;
    ok.X = width/2 - ok.Width/2;
    ok.Y = height  - ok.Height - 10;

    XFreeFontInfo(NULL, font, 1);
    XUnmapWindow(dpy, w->win); /* Make window non resizeable */

    XSizeHints hints = { 0 };
    hints.flags      = PSize | PMinSize | PMaxSize;
    hints.min_width  = hints.max_width  = hints.base_width  = width;
    hints.min_height = hints.max_height = hints.base_height = height;

    XSetWMNormalHints(dpy, w->win, &hints);
    XMapRaised(dpy, w->win);
    XFlush(dpy);

    XEvent e;
    int mouseX = -1, mouseY = -1;

    for (;;) {
        XNextEvent(dpy, &e);

        switch (e.type)
        {
        case ButtonPress:
        case ButtonRelease:
            if (e.xbutton.button != Button1) break;
            int over = X11Button_Contains(&ok, mouseX, mouseY);

            if (ok.Clicked && e.type == ButtonRelease) {
                if (over) return;
            }
            ok.Clicked = e.type == ButtonPress && over;
            /* fallthrough to redraw window */

        case Expose:
        case MapNotify:
            XClearWindow(dpy, w->win);
            X11Textbox_Draw(&body, w);
            X11Button_Draw(&ok, w);
            XFlush(dpy);
            break;

        case KeyRelease:
            if (XLookupKeysym(&e.xkey, 0) == XK_Escape) return;
            break;

        case ClientMessage:
            if (e.xclient.data.l[0] == wmDelete) return;
            break;

        case MotionNotify:
            mouseX = e.xmotion.x; mouseY = e.xmotion.y;
            break;
        }
    }
}

void ErrorHandler_Init(const char* logFile) {
	/* TODO: Implement this */
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
    X11Window w     = { 0 };
    dpy = DisplayDevice_Meta[0];

    X11_MessageBox(title, msg, &w);
	X11Window_Free(&w);
}

void ErrorHandler_Backtrace(STRING_TRANSIENT String* str, void* ctx) {
	/* TODO: Implement this */
}

void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	/* TODO: Implement this */
	ErrorHandler_FailCore(result, raw_msg, NULL);
}
#endif

static void ErrorHandler_FailCore(ReturnCode result, const char* raw_msg, void* ctx) {
	char logMsgBuffer[3070 + 1] = { 0 };
	String logMsg = { logMsgBuffer, 0, 3070 };

	String_Format1(&logMsg, "ClassiCube crashed.\nMessge: %c\n", raw_msg);
	if (result) { String_Format1(&logMsg, "%y\n", &result); } else { result = 1; }

	//ErrorHandler_Registers(ctx, &logMsg);
	ErrorHandler_Backtrace(&logMsg, ctx);
	/* TODO: write to log file */

	String_AppendConst(&logMsg,
		"Please report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.");
	ErrorHandler_ShowDialog("We're sorry", logMsg.buffer);
	Platform_Exit(result);
}

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: write to log file */
}

void ErrorHandler_Fail(const char* raw_msg) { ErrorHandler_FailWithCode(0, raw_msg); }
