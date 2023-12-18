#include "Core.h"
#if defined CC_BUILD_CARBON && !defined CC_BUILD_SDL
#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include <Carbon/Carbon.h>

static int windowX, windowY;
static WindowRef win_handle;
static cc_bool win_fullscreen, showingDialog;

/*########################################################################################################################*
*------------------------------------------------=---Shared with Cocoa----------------------------------------------------*
*#########################################################################################################################*/
/* NOTE: If code here is changed, don't forget to update corresponding code in interop_cocoa.m */
static void Window_CommonInit(void) {
	CGDirectDisplayID display = CGMainDisplayID();
	CGRect bounds = CGDisplayBounds(display);

	DisplayInfo.x      = (int)bounds.origin.x;
	DisplayInfo.y      = (int)bounds.origin.y;
	DisplayInfo.Width  = (int)bounds.size.width;
	DisplayInfo.Height = (int)bounds.size.height;
	DisplayInfo.Depth  = CGDisplayBitsPerPixel(display);
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

static pascal OSErr HandleQuitMessage(const AppleEvent* ev, AppleEvent* reply, long handlerRefcon) {
	Window_Close();
	return 0;
}

static void Window_CommonCreate(void) {
	/* for quit buttons in dock and menubar */
	AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
		NewAEEventHandlerUPP(HandleQuitMessage), 0, false);
}

/* Sourced from https://www.meandmark.com/keycodes.html */
static const cc_uint8 key_map[8 * 16] = {
	'A', 'S', 'D', 'F', 'H', 'G', 'Z', 'X', 'C', 'V', 0, 'B', 'Q', 'W', 'E', 'R',
	'Y', 'T', '1', '2', '3', '4', '6', '5', CCKEY_EQUALS, '9', '7', CCKEY_MINUS, '8', '0', CCKEY_RBRACKET, 'O',
	'U', CCKEY_LBRACKET, 'I', 'P', CCKEY_ENTER, 'L', 'J', CCKEY_QUOTE, 'K', CCKEY_SEMICOLON, CCKEY_BACKSLASH, CCKEY_COMMA, CCKEY_SLASH, 'N', 'M', CCKEY_PERIOD,
	CCKEY_TAB, CCKEY_SPACE, CCKEY_TILDE, CCKEY_BACKSPACE, 0, CCKEY_ESCAPE, 0, 0, 0, CCKEY_CAPSLOCK, 0, 0, 0, 0, 0, 0,
	0, CCKEY_KP_DECIMAL, 0, CCKEY_KP_MULTIPLY, 0, CCKEY_KP_PLUS, 0, CCKEY_NUMLOCK, 0, 0, 0, CCKEY_KP_DIVIDE, CCKEY_KP_ENTER, 0, CCKEY_KP_MINUS, 0,
	0, CCKEY_KP_ENTER, CCKEY_KP0, CCKEY_KP1, CCKEY_KP2, CCKEY_KP3, CCKEY_KP4, CCKEY_KP5, CCKEY_KP6, CCKEY_KP7, 0, CCKEY_KP8, CCKEY_KP9, 'N', 'M', CCKEY_PERIOD,
	CCKEY_F5, CCKEY_F6, CCKEY_F7, CCKEY_F3, CCKEY_F8, CCKEY_F9, 0, CCKEY_F11, 0, CCKEY_F13, 0, CCKEY_F14, 0, CCKEY_F10, 0, CCKEY_F12,
	'U', CCKEY_F15, CCKEY_INSERT, CCKEY_HOME, CCKEY_PAGEUP, CCKEY_DELETE, CCKEY_F4, CCKEY_END, CCKEY_F2, CCKEY_PAGEDOWN, CCKEY_F1, CCKEY_LEFT, CCKEY_RIGHT, CCKEY_DOWN, CCKEY_UP, 0,
};
static int MapNativeKey(UInt32 key) { return key < Array_Elems(key_map) ? key_map[key] : 0; }
/* TODO: Check these.. */
/*   case 0x37: return CCKEY_LWIN; */
/*   case 0x38: return CCKEY_LSHIFT; */
/*   case 0x3A: return CCKEY_LALT; */
/*   case 0x3B: return Key_ControlLeft; */

/* TODO: Verify these differences from OpenTK */
/*Backspace = 51,  (0x33, CCKEY_DELETE according to that link)*/
/*Return = 52,     (0x34, ??? according to that link)*/
/*Menu = 110,      (0x6E, ??? according to that link)*/

void Cursor_SetPosition(int x, int y) {
	CGPoint point;
	point.x = x + windowX;
	point.y = y + windowY;
	CGDisplayMoveCursorToPoint(CGMainDisplayID(), point);
}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		CGDisplayShowCursor(CGMainDisplayID());
	} else {
		CGDisplayHideCursor(CGMainDisplayID());
	}
}

void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	CGAssociateMouseAndMouseCursorPosition(0);
}

void Window_UpdateRawMouse(void) { CentreMousePosition(); }
void Window_DisableRawMouse(void) {
	CGAssociateMouseAndMouseCursorPosition(1);
	DefaultDisableRawMouse();
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
/* NOTE: All Pasteboard functions are OSX 10.3 or later */
static PasteboardRef Window_GetPasteboard(void) {
	PasteboardRef pbRef;
	OSStatus err = PasteboardCreate(kPasteboardClipboard, &pbRef);

	if (err) Logger_Abort2(err, "Creating Pasteboard reference");
	PasteboardSynchronize(pbRef);
	return pbRef;
}

#define FMT_UTF8  CFSTR("public.utf8-plain-text")
#define FMT_UTF16 CFSTR("public.utf16-plain-text")

void Clipboard_GetText(cc_string* value) {
	PasteboardRef pbRef;
	ItemCount itemCount;
	PasteboardItemID itemID;
	CFDataRef outData;
	const UInt8* ptr;
	CFIndex len;
	OSStatus err;
	
	pbRef = Window_GetPasteboard();

	err = PasteboardGetItemCount(pbRef, &itemCount);
	if (err) Logger_Abort2(err, "Getting item count from Pasteboard");
	if (itemCount < 1) return;
	
	err = PasteboardGetItemIdentifier(pbRef, 1, &itemID);
	if (err) Logger_Abort2(err, "Getting item identifier from Pasteboard");
	
	if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF16, &outData))) {	
		ptr = CFDataGetBytePtr(outData);
		len = CFDataGetLength(outData);
		if (ptr) String_AppendUtf16(value, ptr, len);
	} else if (!(err = PasteboardCopyItemFlavorData(pbRef, itemID, FMT_UTF8, &outData))) {
		ptr = CFDataGetBytePtr(outData);
		len = CFDataGetLength(outData);
		if (ptr) String_AppendUtf8(value, ptr, len);
	}
}

void Clipboard_SetText(const cc_string* value) {
	PasteboardRef pbRef;
	CFDataRef cfData;
	UInt8 str[800];
	int len;
	OSStatus err;

	pbRef = Window_GetPasteboard();
	err   = PasteboardClear(pbRef);
	if (err) Logger_Abort2(err, "Clearing Pasteboard");
	PasteboardSynchronize(pbRef);

	len    = String_EncodeUtf8(str, value);
	cfData = CFDataCreate(NULL, str, len);
	if (!cfData) Logger_Abort("CFDataCreate() returned null pointer");

	PasteboardPutItemFlavor(pbRef, 1, FMT_UTF8, cfData, 0);
}


/*########################################################################################################################*
*----------------------------------------------------------Wwindow--------------------------------------------------------*
*#########################################################################################################################*/
static void RefreshWindowBounds(void) {
	Rect r;
	if (win_fullscreen) return;
	
	/* TODO: kWindowContentRgn ??? */
	GetWindowBounds(win_handle, kWindowGlobalPortRgn, &r);
	windowX = r.left; WindowInfo.Width  = r.right  - r.left;
	windowY = r.top;  WindowInfo.Height = r.bottom - r.top;
}

static OSStatus Window_ProcessKeyboardEvent(EventRef inEvent) {
	UInt32 kind, code;
	int key;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
		case kEventRawKeyUp:
			res = GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32,
									NULL, sizeof(UInt32), NULL, &code);
			if (res) Logger_Abort2(res, "Getting key button");

			key = MapNativeKey(code);
			if (key) {
				Input_Set(key, kind != kEventRawKeyUp);
			} else {
				Platform_Log1("Ignoring unknown key %i", &code);
			}
			return eventNotHandledErr;
			
		case kEventRawKeyModifiersChanged:
			res = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, 
									NULL, sizeof(UInt32), NULL, &code);
			if (res) Logger_Abort2(res, "Getting key modifiers");
			
			Input_Set(CCKEY_LCTRL,    code & 0x1000);
			Input_Set(CCKEY_LALT,     code & 0x0800);
			Input_Set(CCKEY_LSHIFT,   code & 0x0200);
			Input_Set(CCKEY_LWIN,     code & 0x0100);			
			Input_Set(CCKEY_CAPSLOCK, code & 0x0400);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessWindowEvent(EventRef inEvent) {
	int oldWidth, oldHeight;
	
	switch (GetEventKind(inEvent)) {
		case kEventWindowClose:
			WindowInfo.Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);
			return eventNotHandledErr;
			
		case kEventWindowClosed:
			WindowInfo.Exists = false;
			return 0;
			
		case kEventWindowBoundsChanged:
			oldWidth = WindowInfo.Width; oldHeight = WindowInfo.Height;
			RefreshWindowBounds();
			
			if (oldWidth != WindowInfo.Width || oldHeight != WindowInfo.Height) {
				Event_RaiseVoid(&WindowEvents.Resized);
			}
			return eventNotHandledErr;
			
		case kEventWindowActivated:
			WindowInfo.Focused = true;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			return eventNotHandledErr;
			
		case kEventWindowDeactivated:
			WindowInfo.Focused = false;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			return eventNotHandledErr;
			
		case kEventWindowDrawContent:
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static int MapNativeMouse(EventMouseButton button) {
	if (button == kEventMouseButtonPrimary)   return CCMOUSE_L;
	if (button == kEventMouseButtonSecondary) return CCMOUSE_R;
	if (button == kEventMouseButtonTertiary)  return CCMOUSE_M;

	if (button == 4) return CCMOUSE_X1;
	if (button == 5) return CCMOUSE_X2;
	return 0;
}

static OSStatus Window_ProcessMouseEvent(EventRef inEvent) {
	int mouseX, mouseY;
	HIPoint pt, raw;
	UInt32 kind;
	cc_bool down;
	EventMouseButton button;
	SInt32 delta;
	OSStatus res;	
	int btn;
	
	res = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint,
							NULL, sizeof(HIPoint), NULL, &pt);
	/* this error comes up from the application event handler */
	if (res && res != eventParameterNotFoundErr) {
		Logger_Abort2(res, "Getting mouse position");
	}

	if (Input.RawMode) {
		raw.x = 0; raw.y = 0;
		GetEventParameter(inEvent, kEventParamMouseDelta, typeHIPoint, NULL, sizeof(HIPoint), NULL, &raw);
		Event_RaiseRawMove(&PointerEvents.RawMoved, raw.x, raw.y);
	}
	
	if (showingDialog) return eventNotHandledErr;
	mouseX = (int)pt.x; mouseY = (int)pt.y;

	/* kEventParamMouseLocation is in screen coordinates */
	/* So need to make sure mouse click lies inside window */
	/* Otherwise this breaks close/minimise/maximise in titlebar and dock */
	if (!win_fullscreen) {
		mouseX -= windowX; mouseY -= windowY;

		if (mouseX < 0 || mouseX >= WindowInfo.Width)  return eventNotHandledErr;
		if (mouseY < 0 || mouseY >= WindowInfo.Height) return eventNotHandledErr;
	}
	
	kind = GetEventKind(inEvent);
	switch (kind) {
		case kEventMouseDown:
		case kEventMouseUp:
			down = kind == kEventMouseDown;
			res  = GetEventParameter(inEvent, kEventParamMouseButton, typeMouseButton, 
									 NULL, sizeof(EventMouseButton), NULL, &button);
			if (res) Logger_Abort2(res, "Getting mouse button");
			
			btn = MapNativeMouse(button);
			if (btn) Input_Set(btn, down); break;
	
			return eventNotHandledErr;
			
		case kEventMouseWheelMoved:
			res = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeSInt32,
									NULL, sizeof(SInt32), NULL, &delta);
			if (res) Logger_Abort2(res, "Getting mouse wheel delta");
			Mouse_ScrollWheel(delta);
			return 0;
			
		case kEventMouseMoved:
		case kEventMouseDragged:
			Pointer_SetPosition(0, mouseX, mouseY);
			return eventNotHandledErr;
	}
	return eventNotHandledErr;
}

static OSStatus Window_ProcessTextEvent(EventRef inEvent) {
	UInt32 kind;
	EventRef keyEvent = NULL;
	UniChar chars[17] = { 0 };
	char keyChar;
	int i;
	OSStatus res;
	
	kind = GetEventKind(inEvent);
	if (kind != kEventTextInputUnicodeForKeyEvent) return eventNotHandledErr;

	/* When command key is pressed, text input should be ignored */
	res = GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent,
							typeEventRef, NULL, sizeof(keyEvent), NULL, &keyEvent);
	if (!res && keyEvent) {
		UInt32 modifiers = 0;
		GetEventParameter(keyEvent, kEventParamKeyModifiers,
							typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
		if (modifiers & 0x0100) return eventNotHandledErr;
	}
	
	/* TODO: is the assumption we only get 1-4 characters always valid */
	res = GetEventParameter(inEvent, kEventParamTextInputSendText,
							typeUnicodeText, NULL, 16 * sizeof(UniChar), NULL, chars);
	if (res) Logger_Abort2(res, "Getting text chars");

	for (i = 0; i < 16 && chars[i]; i++) {
		/* TODO: UTF16 to codepoint conversion */
		Event_RaiseInt(&InputEvents.Press, chars[i]);
	}
	return eventNotHandledErr;
}

static OSStatus Window_EventHandler(EventHandlerCallRef inCaller, EventRef inEvent, void* userData) {
	EventRecord record;
	
	switch (GetEventClass(inEvent)) {
		case kEventClassAppleEvent:
			/* Only event here is the apple event. */
			Platform_LogConst("Processing apple event.");
			ConvertEventRefToEventRecord(inEvent, &record);
			AEProcessAppleEvent(&record);
			break;
			
		case kEventClassKeyboard:
			return Window_ProcessKeyboardEvent(inEvent);
		case kEventClassMouse:
			return Window_ProcessMouseEvent(inEvent);
		case kEventClassWindow:
			return Window_ProcessWindowEvent(inEvent);
		case kEventClassTextInput:
			return Window_ProcessTextEvent(inEvent);
	}
	return eventNotHandledErr;
}

typedef EventTargetRef (*GetMenuBarEventTarget_Func)(void);

static void HookEvents(void) {
	static const EventTypeSpec winEventTypes[] = {
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowDrawContent },

		{ kEventClassTextInput,  kEventTextInputUnicodeForKeyEvent },
		{ kEventClassAppleEvent, kEventAppleEvent }
	};
	static const EventTypeSpec appEventTypes[] = {
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, kEventMouseWheelMoved }
	};
	GetMenuBarEventTarget_Func getMenuBarEventTarget;
	EventTargetRef target;
	
	target = GetWindowEventTarget(win_handle);
	InstallEventHandler(target, NewEventHandlerUPP(Window_EventHandler),
						Array_Elems(winEventTypes), winEventTypes, NULL, NULL);

	target = GetApplicationEventTarget();
	InstallEventHandler(target, NewEventHandlerUPP(Window_EventHandler),
						Array_Elems(appEventTypes), appEventTypes, NULL, NULL);

	/* The code below is to get the menubar working. */
	/* The documentation for 'RunApplicationEventLoop' states that it installs */
	/* the standard application event handler which lets the menubar work. */
	/* However, we cannot use that since the event loop is managed by us instead. */
	/* Unfortunately, there is no proper API to duplicate that behaviour, so rely */
	/* on the undocumented GetMenuBarEventTarget to achieve similar behaviour. */
	/* TODO: This is wrong for PowerPC. But at least it doesn't crash or anything. */
#define _RTLD_DEFAULT ((void*)-2)
	getMenuBarEventTarget = DynamicLib_Get2(_RTLD_DEFAULT, "GetMenuBarEventTarget");
	InstallStandardEventHandler(GetApplicationEventTarget());

	/* TODO: Why does this not work properly until user activates another window */
	/* Followup: Seems to be a bug in OSX http://www.sheepsystems.com/developers_blog/transformprocesstype--bette.html */
	if (getMenuBarEventTarget) {
		InstallStandardEventHandler(getMenuBarEventTarget());
	} else {
		Platform_LogConst("MenuBar won't work!");
	}
	/* MenuRef menu = AcquireRootMenu(); */
	/* InstallStandardEventHandler(GetMenuEventTarget(menu)); */
}


/*########################################################################################################################*
 *--------------------------------------------------Public implementation--------------------------------------------------*
 *#########################################################################################################################*/
void Window_Init(void) { Window_CommonInit(); }

/* Private CGS/CGL stuff */
typedef int CGSConnectionID;
typedef int CGSWindowID;
extern CGSConnectionID _CGSDefaultConnection(void);
extern CGSWindowID GetNativeWindowFromWindowRef(WindowRef window);
extern CGContextRef CGWindowContextCreate(CGSConnectionID conn, CGSWindowID win, void* opts);

static CGSConnectionID conn;
static CGSWindowID winId;

#ifndef kCGBitmapByteOrder32Host
/* Undefined in < 10.4 SDK. No issue since < 10.4 is only Big Endian PowerPC anyways */
#define kCGBitmapByteOrder32Host 0
#endif

#ifdef CC_BUILD_ICON
/* See misc/mac_icon_gen.cs for how to generate this file */
#include "_CCIcon_mac.h"

static void ApplyIcon(void) {
	CGColorSpaceRef colSpace;
	CGDataProviderRef provider;
	CGImageRef image;

	colSpace = CGColorSpaceCreateDeviceRGB();
	provider = CGDataProviderCreateWithData(NULL, CCIcon_Data,
					Bitmap_DataSize(CCIcon_Width, CCIcon_Height), NULL);
	image    = CGImageCreate(CCIcon_Width, CCIcon_Height, 8, 32, CCIcon_Width * 4, colSpace,
					kCGBitmapByteOrder32Host | kCGImageAlphaLast, provider, NULL, 0, 0);

	SetApplicationDockTileImage(image);
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colSpace);
}
#else
static void ApplyIcon(void) { }
#endif

static void DoCreateWindow(int width, int height) {
	Rect r;
	OSStatus res;
	ProcessSerialNumber psn;
	
	r.left = Display_CentreX(width);  r.right  = r.left + width; 
	r.top  = Display_CentreY(height); r.bottom = r.top  + height;
	res = CreateNewWindow(kDocumentWindowClass,
						  kWindowStandardDocumentAttributes | kWindowStandardHandlerAttribute |
						  kWindowInWindowMenuAttribute | kWindowLiveResizeAttribute, &r, &win_handle);

	if (res) Logger_Abort2(res, "Creating window failed");
	RefreshWindowBounds();

	AcquireRootMenu();	
	GetCurrentProcess(&psn);
	SetFrontProcess(&psn);
	
	/* TODO: Use BringWindowToFront instead.. (look in the file which has RepositionWindow in it) !!!! */
	HookEvents();
	Window_CommonCreate();
	WindowInfo.Exists = true;
	WindowInfo.Handle = win_handle;
	/* CGAssociateMouseAndMouseCursorPosition implicitly grabs cursor */

	conn  = _CGSDefaultConnection();
	winId = GetNativeWindowFromWindowRef(win_handle);
	ApplyIcon();
}
void Window_Create2D(int width, int height) { DoCreateWindow(width, height); }
void Window_Create3D(int width, int height) { DoCreateWindow(width, height); }

void Window_SetTitle(const cc_string* title) {
	UInt8 str[NATIVE_STR_LEN];
	CFStringRef titleCF;
	int len;
	
	/* TODO: This leaks memory, old title isn't released */
	len     = String_EncodeUtf8(str, title);
	titleCF = CFStringCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, false);
	SetWindowTitleWithCFString(win_handle, titleCF);
}

int Window_GetWindowState(void) {
	if (win_fullscreen)                return WINDOW_STATE_FULLSCREEN;
	if (IsWindowCollapsed(win_handle)) return WINDOW_STATE_MINIMISED;
	return WINDOW_STATE_NORMAL;
}

int Window_IsObscured(void) {
	/* TODO: This isn't a complete fix because it doesn't check for occlusion - */
	/*  so you'll still get 100% CPU usage when window is hidden in background */
	return IsWindowCollapsed(win_handle);
}

void Window_Show(void) {
	ShowWindow(win_handle);
	/* TODO: Do we actually need to reposition */
	RepositionWindow(win_handle, NULL, kWindowCenterOnMainScreen);
	SelectWindow(win_handle);
}

void Window_SetSize(int width, int height) {
	SizeWindow(win_handle, width, height, true);
}

void Window_Close(void) {
	/* DisposeWindow only sends a kEventWindowClosed */
	Event_RaiseVoid(&WindowEvents.Closing);
	if (WindowInfo.Exists) DisposeWindow(win_handle);
	WindowInfo.Exists = false;
}

void Window_ProcessEvents(double delta) {
	EventRef theEvent;
	EventTargetRef target = GetEventDispatcherTarget();
	OSStatus res;
	
	for (;;) {
		res = ReceiveNextEvent(0, NULL, 0.0, true, &theEvent);
		if (res == eventLoopTimedOutErr) break;
		
		if (res) {
			Platform_Log1("Message Loop status: %i", &res); break;
		}
		if (!theEvent) break;
		
		SendEventToEventTarget(theEvent, target);
		ReleaseEvent(theEvent);
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	Point point;
	GetGlobalMouse(&point);
	*x = (int)point.h; *y = (int)point.v;
}

static void ShowDialogCore(const char* title, const char* msg) {
	CFStringRef titleCF = CFStringCreateWithCString(NULL, title, kCFStringEncodingASCII);
	CFStringRef msgCF   = CFStringCreateWithCString(NULL, msg,   kCFStringEncodingASCII);
	DialogRef dialog;
	DialogItemIndex itemHit;

	showingDialog = true;
	CreateStandardAlert(kAlertPlainAlert, titleCF, msgCF, NULL, &dialog);
	CFRelease(titleCF);
	CFRelease(msgCF);

	RunStandardAlert(dialog, NULL, &itemHit);
	showingDialog = false;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

static CGrafPtr fb_port;
static struct Bitmap fb_bmp;
static CGColorSpaceRef colorSpace;

void Window_AllocFramebuffer(struct Bitmap* bmp) {
	if (!fb_port) fb_port = GetWindowPort(win_handle);

	bmp->scan0 = Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	colorSpace = CGColorSpaceCreateDeviceRGB();
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	CGContextRef context = NULL;
	CGDataProviderRef provider;
	CGImageRef image;
	CGRect rect;
	OSStatus err;

	/* Unfortunately CGImageRef is immutable, so changing the */
	/* underlying data doesn't change what shows when drawing. */
	/* TODO: Use QuickDraw alternative instead */

	/* TODO: Only update changed bit.. */
	rect.origin.x = 0; rect.origin.y = 0;
	rect.size.width  = WindowInfo.Width;
	rect.size.height = WindowInfo.Height;

	err = QDBeginCGContext(fb_port, &context);
	if (err) Logger_Abort2(err, "Begin draw");
	/* TODO: REPLACE THIS AWFUL HACK */

	provider = CGDataProviderCreateWithData(NULL, fb_bmp.scan0,
		Bitmap_DataSize(fb_bmp.width, fb_bmp.height), NULL);
	image    = CGImageCreate(fb_bmp.width, fb_bmp.height, 8, 32, fb_bmp.width * 4, colorSpace,
				kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);

	CGContextDrawImage(context, rect, image);
	CGContextSynchronize(context);
	err = QDEndCGContext(fb_port, &context);
	if (err) Logger_Abort2(err, "End draw");

	CGImageRelease(image);
	CGDataProviderRelease(provider);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
	CGColorSpaceRelease(colorSpace);
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }


/*########################################################################################################################*
*-------------------------------------------------------AGL OpenGL--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_GL && !defined CC_BUILD_EGL
#include <AGL/agl.h>

static AGLContext ctx_handle;
static int ctx_windowWidth, ctx_windowHeight;

static void GLContext_Check(int code, const char* place) {
	cc_result res;
	if (code) return;

	res = aglGetError();
	if (res) Logger_Abort2(res, place);
}

static void GLContext_MakeCurrent(void) {
	int code = aglSetCurrentContext(ctx_handle);
	GLContext_Check(code, "Setting GL context");
}

static void GLContext_SetDrawable(void) {
	CGrafPtr windowPort = GetWindowPort(win_handle);
	int code = aglSetDrawable(ctx_handle, windowPort);
	GLContext_Check(code, "Attaching GL context");
}

cc_result Window_EnterFullscreen(void) {
	int width  = DisplayInfo.Width;
	int height = DisplayInfo.Height;
	int code;

	/* TODO: Does aglSetFullScreen capture the screen anyways? */
	CGDisplayCapture(CGMainDisplayID());

	if (!aglSetFullScreen(ctx_handle, width, height, 0, 0)) {
		code = aglGetError();
		Window_ExitFullscreen();
		return code;
	}

	win_fullscreen   = true;
	ctx_windowWidth  = WindowInfo.Width;
	ctx_windowHeight = WindowInfo.Height;

	windowX = DisplayInfo.x; WindowInfo.Width  = DisplayInfo.Width;
	windowY = DisplayInfo.y; WindowInfo.Height = DisplayInfo.Height;	
	
	Event_RaiseVoid(&WindowEvents.Resized);
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	int code;

	code = aglSetDrawable(ctx_handle, NULL);
	if (!code) return aglGetError();

	CGDisplayRelease(CGMainDisplayID());
	GLContext_SetDrawable();

	win_fullscreen = false;
	/* TODO: Eliminate this if possible */
	Window_SetSize(ctx_windowWidth, ctx_windowHeight);
	
	RefreshWindowBounds();
	Event_RaiseVoid(&WindowEvents.Resized);
	return 0;
}

static void GLContext_GetAttribs(struct GraphicsMode* mode, GLint* attribs, cc_bool fullscreen) {
	int i = 0;

	attribs[i++] = AGL_RGBA;
	attribs[i++] = AGL_RED_SIZE;   attribs[i++] = mode->R;
	attribs[i++] = AGL_GREEN_SIZE; attribs[i++] = mode->G;
	attribs[i++] = AGL_BLUE_SIZE;  attribs[i++] = mode->B;
	attribs[i++] = AGL_ALPHA_SIZE; attribs[i++] = mode->A;
	attribs[i++] = AGL_DEPTH_SIZE; attribs[i++] = GLCONTEXT_DEFAULT_DEPTH;

	attribs[i++] = AGL_DOUBLEBUFFER;
	if (fullscreen) { attribs[i++] = AGL_FULLSCREEN; }
	attribs[i++] = 0;
}

void GLContext_Create(void) {
	GLint attribs[20];
	AGLPixelFormat fmt;
	GDHandle gdevice;
	OSStatus res;
	struct GraphicsMode mode;
	InitGraphicsMode(&mode);

	/* Initially try creating fullscreen compatible context */	
	res = DMGetGDeviceByDisplayID(CGMainDisplayID(), &gdevice, false);
	if (res) Logger_Abort2(res, "Getting display device failed");

	GLContext_GetAttribs(&mode, attribs, true);
	fmt = aglChoosePixelFormat(&gdevice, 1, attribs);
	res = aglGetError();

	/* Try again with non-compatible context if that fails */
	if (!fmt || res == AGL_BAD_PIXELFMT) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");

		GLContext_GetAttribs(&mode, attribs, false);
		fmt = aglChoosePixelFormat(NULL, 0, attribs);
		res = aglGetError();
	}
	if (res) Logger_Abort2(res, "Choosing pixel format");

	ctx_handle = aglCreateContext(fmt, NULL);
	GLContext_Check(0, "Creating GL context");
	aglDestroyPixelFormat(fmt);

	GLContext_SetDrawable();
	GLContext_MakeCurrent();
}

void GLContext_Update(void) {
	if (win_fullscreen) return;
	aglUpdateContext(ctx_handle);
}
cc_bool GLContext_TryRestore(void) { return true; }

void GLContext_Free(void) {
	if (!ctx_handle) return;
	aglSetCurrentContext(NULL);
	aglDestroyContext(ctx_handle);
	ctx_handle = NULL;
}

void* GLContext_GetAddress(const char* function) {
	static const cc_string glPath = String_FromConst("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL");
	static void* lib;

	if (!lib) lib = DynamicLib_Load2(&glPath);
	return DynamicLib_Get2(lib, function);
}

cc_bool GLContext_SwapBuffers(void) {
	aglSwapBuffers(ctx_handle);
	GLContext_Check(0, "Swapping buffers");
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int value = vsync ? 1 : 0;
	aglSetInteger(ctx_handle, AGL_SWAP_INTERVAL, &value);
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
