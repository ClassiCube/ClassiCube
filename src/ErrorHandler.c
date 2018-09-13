#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"
#include "Window.h"
#include "Funcs.h"
#include "Stream.h"

static void ErrorHandler_FailCommon(ReturnCode result, const char* raw_msg, void* ctx);
static void ErrorHandler_DumpCommon(STRING_TRANSIENT String* str, void* ctx);

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <imagehlp.h>

struct StackPointers { UInt64 Instruction, Frame, Stack; };
struct SymbolAndName { IMAGEHLP_SYMBOL Symbol; char Name[256]; };


/*########################################################################################################################*
*-------------------------------------------------------Info dumping------------------------------------------------------*
*#########################################################################################################################*/
static Int32 ErrorHandler_GetFrames(CONTEXT* ctx, struct StackPointers* pointers, Int32 max) {
	STACKFRAME frame = { 0 };
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
		if (!StackWalk(type, process, thread, &frame, &copy, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) break;
		if (!frame.AddrFrame.Offset) break;

		pointers[count].Instruction = frame.AddrPC.Offset;
		pointers[count].Frame       = frame.AddrFrame.Offset;
		pointers[count].Stack       = frame.AddrStack.Offset;
	}
	return count;
}

static BOOL CALLBACK ErrorHandler_DumpModule(const char* name, ULONG_PTR base, ULONG size, void* ctx) {
	char buffer[STRING_SIZE * 4];
	String str = String_FromArray(buffer);
	DWORD64 start = base, end = base + (size - 1);

	String_Format3(&str, "%c = %x-%x\r\n", name, &start, &end);
	ErrorHandler_Log(&str);
	return true;
}

static void ErrorHandler_Backtrace(STRING_TRANSIENT String* backtrace, void* ctx) {
	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLength = 255;
	sym.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);

	HANDLE process = GetCurrentProcess();
	struct StackPointers pointers[40];
	Int32 i, frames = ErrorHandler_GetFrames((CONTEXT*)ctx, pointers, 40);

	for (i = 0; i < frames; i++) {
		Int32 number = i + 1;
		UInt64 addr = (UInt64)pointers[i].Instruction;

		char strBuffer[STRING_SIZE * 10];
		String str = String_FromArray(strBuffer);

		/* instruction pointer */
		if (SymGetSymFromAddr(process, addr, NULL, &sym.Symbol)) {
			String_Format3(&str, "%i) 0x%x - %c\r\n", &number, &addr, sym.Symbol.Name);
		} else {
			String_Format2(&str, "%i) 0x%x\r\n", &number, &addr);
		}

		/* frame and stack address */
		String_AppendString(backtrace, &str);
		String_Format2(&str, "  fp: %x, sp: %x\r\n", &pointers[i].Frame, &pointers[i].Stack);

		/* line number */
		IMAGEHLP_LINE line = { 0 }; DWORD lineOffset;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
		if (SymGetLineFromAddr(process, addr, &lineOffset, &line)) {
			String_Format2(&str, "  line %i in %c\r\n", &line.LineNumber, line.FileName);
		}

		/* module address is in */
		IMAGEHLP_MODULE module = { 0 };
		module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
		if (SymGetModuleInfo(process, addr, &module)) {
			String_Format2(&str, "  in module %c (%c)\r\n", module.ModuleName, module.ImageName);
		}
		ErrorHandler_Log(&str);
	}
	String_AppendConst(backtrace, "\r\n");
}

static void ErrorHandler_DumpCommon(STRING_TRANSIENT String* str, void* ctx) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);

	String backtrace = String_FromConst("-- backtrace --\r\n");
	ErrorHandler_Log(&backtrace);
	ErrorHandler_Backtrace(str, ctx);

	String modules = String_FromConst("-- modules --\r\n");
	ErrorHandler_Log(&modules);
	EnumerateLoadedModules(process, ErrorHandler_DumpModule, NULL);
}

static void ErrorHandler_DumpRegisters(CONTEXT* ctx) {
	char strBuffer[STRING_SIZE * 8];
	String str = String_FromArray(strBuffer);
	String_AppendConst(&str, "-- registers --\r\n");

#ifdef _M_IX86	
	String_Format3(&str, "eax=%y ebx=%y ecx=%y\r\n", &ctx->Eax, &ctx->Ebx, &ctx->Ecx);
	String_Format3(&str, "edx=%y esi=%y edi=%y\r\n", &ctx->Edx, &ctx->Esi, &ctx->Edi);
	String_Format3(&str, "eip=%y ebp=%y esp=%y\r\n", &ctx->Eip, &ctx->Ebp, &ctx->Esp);
#elif _M_X64
	String_Format3(&str, "rax=%x rbx=%x rcx=%x\r\n", &ctx->Rax, &ctx->Rbx, &ctx->Rcx);
	String_Format3(&str, "rdx=%x rsi=%x rdi=%x\r\n", &ctx->Rdx, &ctx->Rsi, &ctx->Rdi);
	String_Format3(&str, "rip=%x rbp=%x rsp=%x\r\n", &ctx->Rip, &ctx->Rbp, &ctx->Rsp);
	String_Format3(&str, "r8 =%x r9 =%x r10=%x\r\n", &ctx->R8,  &ctx->R9,  &ctx->R10);
	String_Format3(&str, "r11=%x r12=%x r13=%x\r\n", &ctx->R11, &ctx->R12, &ctx->R13);
	String_Format2(&str, "r14=%x r15=%x\r\n"       , &ctx->R14, &ctx->R15);
#elif _M_IA64
	String_Format3(&str, "r1 =%x r2 =%x r3 =%x\r\n", &ctx->IntGp,  &ctx->IntT0,  &ctx->IntT1);
	String_Format3(&str, "r4 =%x r5 =%x r6 =%x\r\n", &ctx->IntS0,  &ctx->IntS1,  &ctx->IntS2);
	String_Format3(&str, "r7 =%x r8 =%x r9 =%x\r\n", &ctx->IntS3,  &ctx->IntV0,  &ctx->IntT2);
	String_Format3(&str, "r10=%x r11=%x r12=%x\r\n", &ctx->IntT3,  &ctx->IntT4,  &ctx->IntSp);
	String_Format3(&str, "r13=%x r14=%x r15=%x\r\n", &ctx->IntTeb, &ctx->IntT5,  &ctx->IntT6);
	String_Format3(&str, "r16=%x r17=%x r18=%x\r\n", &ctx->IntT7,  &ctx->IntT8,  &ctx->IntT9);
	String_Format3(&str, "r19=%x r20=%x r21=%x\r\n", &ctx->IntT10, &ctx->IntT11, &ctx->IntT12);
	String_Format3(&str, "r22=%x r23=%x r24=%x\r\n", &ctx->IntT13, &ctx->IntT14, &ctx->IntT15);
	String_Format3(&str, "r25=%x r26=%x r27=%x\r\n", &ctx->IntT16, &ctx->IntT17, &ctx->IntT18);
	String_Format3(&str, "r28=%x r29=%x r30=%x\r\n", &ctx->IntT19, &ctx->IntT20, &ctx->IntT21);
	String_Format3(&str, "r31=%x nat=%x pre=%x\r\n", &ctx->IntT22, &ctx->IntNats,&ctx->Preds);
#else
#error "Unknown machine type"
#endif
	ErrorHandler_Log(&str);
}


/*########################################################################################################################*
*------------------------------------------------------Error handling-----------------------------------------------------*
*#########################################################################################################################*/
static LONG WINAPI ErrorHandler_UnhandledFilter(struct _EXCEPTION_POINTERS* pInfo) {
	char msgBuffer[128 + 1] = { 0 };
	String msg = { msgBuffer, 0, 128 };

	UInt32 code = (UInt32)pInfo->ExceptionRecord->ExceptionCode;
	UInt64 addr = (UInt64)pInfo->ExceptionRecord->ExceptionAddress;
	String_Format2(&msg, "Unhandled exception 0x%y at 0x%x", &code, &addr);

	ErrorHandler_DumpRegisters(pInfo->ContextRecord);
	ErrorHandler_FailCommon(0, msg.buffer, pInfo->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void ErrorHandler_Init(const char* logFile) {
	SetUnhandledExceptionFilter(ErrorHandler_UnhandledFilter);
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
	MessageBoxA(Window_GetWindowHandle(), msg, title, 0);
}

/* Don't want compiler doing anything fancy with registers */
#if _MSC_VER
#pragma optimize ("", off)
#endif
void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	CONTEXT ctx;
#ifndef _M_IX86
	/* This method is guaranteed to exist on 64 bit windows */
	/* It is missing in 32 bit Windows 2000 however */
	RtlCaptureContext(&ctx);
#elif _MSC_VER
	/* Stack frame layout on x86: */
	/* [ebp] is previous frame's EBP */
	/* [ebp+4] is previous frame's EIP (return address) */
	/* address of [ebp+8] is previous frame's ESP */
	__asm {		
		mov eax, [ebp]
		mov [ctx.Ebp], eax	
		mov eax, [ebp+4]
		mov [ctx.Eip], eax	
		lea eax, [ebp+8]	
		mov [ctx.Esp], eax
		mov [ctx.ContextFlags], CONTEXT_CONTROL
	}
#else
	Int32 _ebp, _eip, _esp;
	/* TODO: I think this is right, not sure.. */
	__asm__(
		"mov 0(%%ebp), %%eax \n\t"
		"mov %%eax, %0       \n\t"
		"mov 4(%%ebp), %%eax \n\t"
		"mov %%eax, %1       \n\t"
		"lea 8(%%ebp), %%eax \n\t"
		"mov %%eax, %2"
		: "=m" (_ebp), "=m" (_eip), "=m" (_esp)
		:
		: "eax", "memory");

	ctx.Ebp = _ebp;
	ctx.Eip = _eip;
	ctx.Esp = _esp;
	ctx.ContextFlags = CONTEXT_CONTROL;
#endif

	ErrorHandler_FailCommon(result, raw_msg, &ctx);
}
#if _MSC_VER
#pragma optimize ("", on)
#endif
#elif CC_BUILD_NIX
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


/*########################################################################################################################*
*-----------------------------------------------------X11 message box-----------------------------------------------------*
*#########################################################################################################################*/
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


/*########################################################################################################################*
*-------------------------------------------------------Info dumping------------------------------------------------------*
*#########################################################################################################################*/
static void ErrorHandler_Backtrace(STRING_TRANSIENT String* backtrace_, void* ctx) {
	void* addrs[40];
	Int32 i, frames = backtrace(addrs, 40);
	char** strings  = backtrace_symbols(addrs, frames);

	for (i = 0; i < frames; i++) {
		Int32 number = i + 1;
		UInt64 addr = (UInt64)addrs[i];

		char strBuffer[STRING_SIZE * 5];
		String str = String_FromArray(strBuffer);

		/* instruction pointer */
		if (strings && strings[i]) {
			String_Format3(&str, "%i) 0x%x - %c\n", &number, &addr, strings[i]);
		} else {
			String_Format2(&str, "%i) 0x%x\n", &number, &addr);
		}

		String_AppendString(backtrace_, &str);
		ErrorHandler_Log(&str);
	}

	String_AppendConst(backtrace_, "\n");
	free(strings);
}

static void ErrorHandler_DumpRegisters(void* ctx) {
	if (!ctx) return;
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;

	char strBuffer[STRING_SIZE * 8];
	String str = String_FromArray(strBuffer);
	String_AppendConst(&str, "-- registers --\n");

	/* TODO: There must be a better way of getting these.. */
#ifdef __i386__
	String_Format3(&str, "eax=%y ebx=%y ecx=%y\n", &r.gregs[11], &r.gregs[8], &r.gregs[10]);
	String_Format3(&str, "edx=%y esi=%y edi=%y\n", &r.gregs[9],  &r.gregs[5], &r.gregs[4]);
	String_Format3(&str, "eip=%y ebp=%y esp=%y\n", &r.gregs[14], &r.gregs[6], &r.gregs[7]);
#elif __x86_64__
	String_Format3(&str, "rax=%x rbx=%x rcx=%x\n", &r.gregs[13], &r.gregs[11], &r.gregs[14]);
	String_Format3(&str, "rdx=%x rsi=%x rdi=%x\n", &r.gregs[12], &r.gregs[9],  &r.gregs[8]);
	String_Format3(&str, "rip=%x rbp=%x rsp=%x\n", &r.gregs[16], &r.gregs[10], &r.gregs[15]);
	String_Format3(&str, "r8 =%x r9 =%x r10=%x\n", &r.gregs[0],  &r.gregs[1],  &r.gregs[2]);
	String_Format3(&str, "r11=%x r12=%x r13=%x\n", &r.gregs[3],  &r.gregs[4],  &r.gregs[5]);
	String_Format2(&str, "r14=%x r15=%x\n",        &r.gregs[6],  &r.gregs[7]);
#else
#error "Unknown machine type"
#endif
	ErrorHandler_Log(&str);
}

static void ErrorHandler_DumpMemoryMap(void) {
	int n, fd = open("/proc/self/maps", O_RDONLY);
	if (fd < 0) return;

	char buffer[STRING_SIZE * 5];
	String str = String_FromArray(buffer);

	while ((n = read(fd, str.buffer, str.capacity)) > 0) {
		str.length = n;
		ErrorHandler_Log(&str);
	}

	close(fd);
}

static void ErrorHandler_DumpCommon(STRING_TRANSIENT String* str, void* ctx) {
	String backtrace = String_FromConst("-- backtrace --\n");
	ErrorHandler_Log(&backtrace);
	ErrorHandler_Backtrace(str, ctx);

	String memMap = String_FromConst("-- memory map --\n");
	ErrorHandler_Log(&memMap);
	ErrorHandler_DumpMemoryMap();
}


/*########################################################################################################################*
*------------------------------------------------------Error handling-----------------------------------------------------*
*#########################################################################################################################*/
static void ErrorHandler_SignalHandler(int sig, siginfo_t* info, void* ctx) {
	/* Uninstall handler to avoid chance of infinite loop */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE, SIG_DFL);

	/* TODO: Write processor state to file*/
	char msgBuffer[128 + 1] = { 0 };
	String msg = { msgBuffer, 0, 128 };

	Int32  type = info->si_signo, code = info->si_code;
	UInt64 addr = (UInt64)info->si_addr;
	String_Format3(&msg, "Unhandled signal %i (code %i) at 0x%x", &type, &code, &addr);

	ErrorHandler_DumpRegisters(ctx);
	ErrorHandler_FailCommon(0, msg.buffer, ctx);
}

void ErrorHandler_Init(const char* logFile) {
	struct sigaction sa, old;
	sa.sa_handler = ErrorHandler_SignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags   = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, &old);
	sigaction(SIGBUS,  &sa, &old);
	sigaction(SIGILL,  &sa, &old);
	sigaction(SIGABRT, &sa, &old);
	sigaction(SIGFPE,  &sa, &old);
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
	X11Window w = { 0 };
	dpy = DisplayDevice_Meta[0];

	X11_MessageBox(title, msg, &w);
	X11Window_Free(&w);
}

void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	ucontext_t ctx;
	getcontext(&ctx);
	ErrorHandler_FailCommon(result, raw_msg, &ctx);
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Common---------------------------------------------------------*
*#########################################################################################################################*/
void* logFile;
struct Stream logStream;
bool logOpen;

void ErrorHandler_Log(STRING_PURE String* msg) {
	if (!logOpen) {
		logOpen = true;
		String path = String_FromConst("client.log");

		ReturnCode res = File_Append(&logFile, &path);
		if (!res) Stream_FromFile(&logStream, logFile);
	}

	if (!logStream.Meta.File) return;
	Stream_Write(&logStream, msg->buffer, msg->length);
}

static void ErrorHandler_FailCommon(ReturnCode result, const char* raw_msg, void* ctx) {
	char logMsgBuffer[3070 + 1] = { 0 };
	String msg = { logMsgBuffer, 0, 3070 };

	String_Format3(&msg, "ClassiCube crashed.%cMessge: %c%c", Platform_NewLine, raw_msg, Platform_NewLine);
	if (result) { 
		String_Format2(&msg, "%y%c", &result, Platform_NewLine); 
	} else { result = 1; }

	ErrorHandler_Log(&msg);
	ErrorHandler_DumpCommon(&msg, ctx);
	if (logStream.Meta.File) File_Close(logFile);

	String_AppendConst(&msg, "Full details of the crash have been logged to 'client.log'.\n");
	String_AppendConst(&msg,
		"Please report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.");
	ErrorHandler_ShowDialog("We're sorry", msg.buffer);
	Platform_Exit(result);
}

void ErrorHandler_Fail(const char* raw_msg) { ErrorHandler_FailWithCode(0, raw_msg); }
