#include "Core.h"
#if defined CC_BUILD_SYMBIAN

#include <include/bitmap.h>
#include <e32base.h>
#include <coemain.h>
#include <e32keys.h>
#include <w32std.h>
#include <aknutils.h>
#include <aknnotewrappers.h>
#include <apgwgnam.h>
#include <stdlib.h>
#include <apgcli.h>
extern "C" {
#include <stdapis/string.h>
#include <gles/egl.h>
#include "_WindowBase.h"
#include "Errors.h"
#include "Logger.h"
#include "String.h"
#include "Gui.h"
#include "Graphics.h"
#include "Game.h"
#include "VirtualKeyboard.h"
}

static cc_bool launcherMode;
static const BindMapping symbian_binds[BIND_COUNT] = {
	{ CCKEY_UP, 0 },    // BIND_FORWARD
	{ CCKEY_DOWN, 0 },  // BIND_BACKWARDS
	{ CCKEY_LEFT, 0 },  // BIND_LEFT
	{ CCKEY_RIGHT, 0 }, // BIND_RIGHT
	{ CCKEY_SPACE, 0 }, // BIND_JUMP  
	{ 'R', 0 }, // BIND_RESPAWN
	{ CCKEY_ENTER, 0 }, // BIND_SET_SPAWN
	{ 'T', 0 }, // BIND_CHAT
	{ 'B', 0 }, // BIND_INVENTORY
	{ 'F', 0 }, // BIND_FOG
	{ CCKEY_ENTER, 0 }, // BIND_SEND_CHAT
	{ CCKEY_TAB, 0 }, // BIND_TABLIST
	{ '7', 0 }, // BIND_SPEED
	{ '8', 0 }, // BIND_NOCLIP
	{ '9', 0 }, // BIND_FLY 
	{ CCKEY_Q, 0 }, // BIND_FLY_UP
	{ CCKEY_E, 0 }, // BIND_FLY_DOWN
	{ CCKEY_LALT, 0 }, // BIND_EXT_INPUT
	{ 0, 0 }, // BIND_HIDE_FPS
	{ CCKEY_F12, 0 }, // BIND_SCREENSHOT
	{ 0, 0 }, // BIND_FULLSCREEN
	{ CCKEY_F5, 0 }, // BIND_THIRD_PERSON
	{ 0, 0 }, // BIND_HIDE_GUI
	{ CCKEY_F7, 0 }, // BIND_AXIS_LINES
	{ 'C', 0 }, // BIND_ZOOM_SCROLL
	{ CCKEY_LCTRL, 0 },// BIND_HALF_SPEED
	{ '4', 0 }, // BIND_DELETE_BLOCK
	{ '5', 0 }, // BIND_PICK_BLOCK
	{ '6', 0 }, // BIND_PLACE_BLOCK
	
	{ 0, 0 }, { 0, 0 },         /* BIND_AUTOROTATE, BIND_HOTBAR_SWITCH */
	{ 0, 0 }, { 0, 0 },                  /* BIND_SMOOTH_CAMERA, BIND_DROP_BLOCK */
	{ 0, 0 }, { 0, 0 },                  /* BIND_IDOVERLAY, BIND_BREAK_LIQUIDS */
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },         /* BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_RIGHT, BIND_LOOK_LEFT */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },             /* BIND_HOTBAR_1, BIND_HOTBAR_2, BIND_HOTBAR_3 */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },             /* BIND_HOTBAR_4, BIND_HOTBAR_5, BIND_HOTBAR_6 */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },             /* BIND_HOTBAR_7, BIND_HOTBAR_8, BIND_HOTBAR_9 */
	{ '1', 0 }, { '3', 0 }                              /* BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT */
};


class CWindow;

CWindow* window;

static bool ConvertToUnicode(const char* str, size_t length, TDes& destBuf) {
	if (str) {
		wchar_t* dest = reinterpret_cast <wchar_t*> (const_cast <TUint16*> (destBuf.Ptr()));
		TInt len = mbstowcs(dest, str, length);
		if (len > 0) {
			destBuf.SetLength(len);
			return true;
		}
	}
	return false;
}

static bool ConvertToUnicode(const cc_string* str, TDes& destBuf) {
	return ConvertToUnicode(str->buffer, (size_t)str->length, destBuf);
}

class CWindow : public CBase
{
public:
	static CWindow* NewL();
	void HandleWsEvent(const TWsEvent& aEvent);
	void AllocFrameBuffer(int width, int height);
	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
	void FreeFrameBuffer();
	void ProcessEvents(float delta);
	void RequestClose();
	void InitEvents();
	cc_result OpenBrowserL(const cc_string* url);
	~CWindow();
	
	TWsEvent iWsEvent;
	TRequestStatus iWsEventStatus;
	RWindow* iWindow;

private:
	CWindow();
	void ConstructL();
	void CreateWindowL();

	RWsSession iWsSession;
	RWindowGroup iWindowGroup;
	CWsScreenDevice* iWsScreenDevice;
	CWindowGc* iWindowGc;
	CFbsBitmap* iBitmap;
	CApaWindowGroupName* iWindowGroupName;

	TBool iEventsInitialized;
};

//

CWindow* CWindow::NewL() {
	CWindow* self = new (ELeave) CWindow();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
}

void CWindow::CreateWindowL() {
	TPixelsTwipsAndRotation pixnrot;
	iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);

	iWindow->SetExtent(TPoint(0, 0), pixnrot.iPixelSize);
	iWindow->SetRequiredDisplayMode(iWsScreenDevice->DisplayMode());
#ifdef CC_BUILD_SYMBIAN_3
	iWindow->EnableAdvancedPointers();
#endif
	iWindow->Activate();
	iWindow->SetVisible(ETrue);
	iWindow->SetNonFading(ETrue);
	iWindow->SetShadowDisabled(ETrue);
	iWindow->EnableRedrawStore(EFalse);
	iWindow->EnableVisibilityChangeEvents();
	iWindow->SetNonTransparent();
	iWindow->SetBackgroundColor();
	iWindow->SetOrdinalPosition(0);
	// Enable drag events
	iWindow->PointerFilter(EPointerFilterDrag, 0);

	WindowInfo.Focused = true;
	WindowInfo.Exists = true;
	WindowInfo.Handle.ptr = (void*) iWindow;
#ifdef CC_BUILD_TOUCH
	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_VIRTUAL;
#endif
	
	TInt w = pixnrot.iPixelSize.iWidth,
		h = pixnrot.iPixelSize.iHeight;
	
	DisplayInfo.Width = w;
	DisplayInfo.Height = h;
	
	WindowInfo.Width = w;
	WindowInfo.Height = h;

	WindowInfo.UIScaleX = DEFAULT_UI_SCALE_X;
	WindowInfo.UIScaleY = DEFAULT_UI_SCALE_Y;
	if (w <= 360) {
		DisplayInfo.ScaleX = 0.5f;
		DisplayInfo.ScaleY = 0.5f;
	} else {
		DisplayInfo.ScaleX = 1;
		DisplayInfo.ScaleY = 1;
	}
}

CWindow::CWindow() {
	
}

CWindow::~CWindow() {
	if (iWindowGc) {
		delete iWindowGc;
		iWindowGc = NULL;
	}
	if (iWindow) {
		iWindow->SetOrdinalPosition(KOrdinalPositionSwitchToOwningWindow);
		iWindow->Close();
		delete iWindow;
		iWindow = NULL;
	}
	if (iWsScreenDevice) {
		delete iWsScreenDevice;
		iWsScreenDevice = NULL;
	}
}

void CWindow::ConstructL() {
	delete CActiveScheduler::Current();
	CActiveScheduler* actScheduler = new (ELeave) CActiveScheduler();
	CActiveScheduler::Install(actScheduler);
	
	CCoeEnv* env = CCoeEnv::Static();
	if (!env) {
		User::Panic(_L("CoeEnv::Static not initialized"), 0);
	}
	
	iWsSession = env->WsSession();
	iWsScreenDevice = new (ELeave) CWsScreenDevice(iWsSession);
	User::LeaveIfError(iWsScreenDevice->Construct());

	iWindowGroup = RWindowGroup(iWsSession);
	User::LeaveIfError(iWindowGroup.Construct(reinterpret_cast<TUint32>(this) - 1));
	iWindowGroup.SetOrdinalPosition(0);
	iWindowGroup.EnableScreenChangeEvents();
#ifdef CC_BUILD_TOUCH
	iWindowGroup.EnableReceiptOfFocus(EFalse);
#else
	iWindowGroup.EnableReceiptOfFocus(ETrue);
#endif

	iWindowGroupName = CApaWindowGroupName::NewL(iWsSession, iWindowGroup.Identifier());
	iWindowGroupName->SetAppUid(TUid::Uid(0xE212A5C2));
	iWindowGroupName->SetCaptionL(_L("ClassiCube"));
	iWindowGroupName->SetHidden(EFalse);
	iWindowGroupName->SetSystem(EFalse);
	iWindowGroupName->SetRespondsToShutdownEvent(ETrue);
	iWindowGroupName->SetWindowGroupName(iWindowGroup);

	iWindow = new (ELeave) RWindow(iWsSession);

	TInt err = iWindow->Construct(iWindowGroup, reinterpret_cast<TUint32>(this));
	User::LeaveIfError(err);

	TRAP(err, CreateWindowL());
	if (err) {
		User::Panic(_L("Window creation failed"), err);
	}

	RWindowGroup rootWin = CCoeEnv::Static()->RootWin();
	CApaWindowGroupName* rootWindGroupName = 0;
	TRAP_IGNORE(rootWindGroupName = CApaWindowGroupName::NewL(iWsSession, rootWin.Identifier()));
	if (rootWindGroupName) {
		rootWindGroupName->SetHidden(ETrue);
		rootWindGroupName->SetWindowGroupName(rootWin);
	}
	
	TDisplayMode displayMode = iWindow->DisplayMode();
	TInt bufferSize = 0;

	switch (displayMode) {
	case EColor4K:
		bufferSize = 12;
		break;
	case EColor64K:
		bufferSize = 16;
		break;
	case EColor16M:
		bufferSize = 24;
		break;
	case EColor16MU:
	case EColor16MA:
		bufferSize = 32;
		break;
	default:
		break;
	}
	
	DisplayInfo.Depth = bufferSize;
	
	iWsSession.EventReadyCancel();
}

static int ConvertKey(TInt aScanCode) {
	// TODO array?
	switch (aScanCode) {
	case EStdKeyBackspace:
		return CCKEY_BACKSPACE;
	case EStdKeyTab:
		return CCKEY_TAB;
	case EStdKeyEnter:
		return CCKEY_ENTER;
	case EStdKeyEscape:
		return CCKEY_ESCAPE;
	case EStdKeySpace:
		return CCKEY_SPACE;
	case EStdKeyPrintScreen:
		return CCKEY_PRINTSCREEN;
	case EStdKeyPause:
		return CCKEY_PAUSE;
	case EStdKeyHome:
		return CCKEY_HOME;
	case EStdKeyEnd:
		return CCKEY_END;
	case EStdKeyPageUp:
		return CCKEY_PAGEUP;
	case EStdKeyPageDown:
		return CCKEY_PAGEDOWN;
	case EStdKeyInsert:
		return CCKEY_INSERT;
	case EStdKeyDelete:
		return CCKEY_DELETE;
	case EStdKeyLeftArrow:
		return CCKEY_LEFT;
	case EStdKeyRightArrow:
		return CCKEY_RIGHT;
	case EStdKeyUpArrow:
		return CCKEY_UP;
	case EStdKeyDownArrow:
		return CCKEY_DOWN;
	case EStdKeyLeftShift:
		return CCKEY_LSHIFT;
	case EStdKeyRightShift:
		return CCKEY_RSHIFT;
	case EStdKeyLeftAlt:
		return CCKEY_LALT;
	case EStdKeyRightAlt:
		return CCKEY_RALT;
	case EStdKeyLeftCtrl:
		return CCKEY_LCTRL;
	case EStdKeyRightCtrl:
		return CCKEY_RCTRL;
	case EStdKeyLeftFunc:
		return CCKEY_LWIN;
	case EStdKeyRightFunc:
		return CCKEY_RWIN;
	case EStdKeyNumLock:
		return CCKEY_NUMLOCK;
	case EStdKeyScrollLock:
		return CCKEY_SCROLLLOCK;

	case 0x30:
		return CCKEY_0;
	case 0x31:
		return CCKEY_1;
	case 0x32:
		return CCKEY_2;
	case 0x33:
		return CCKEY_3;
	case 0x34:
		return CCKEY_4;
	case 0x35:
		return CCKEY_5;
	case 0x36:
		return CCKEY_6;
	case 0x37:
		return CCKEY_7;
	case 0x38:
		return CCKEY_8;
	case 0x39:
		return CCKEY_9;

	case EStdKeyComma:
		return CCKEY_COMMA;
	case EStdKeyFullStop:
		return CCKEY_PERIOD;
	case EStdKeyForwardSlash:
		return CCKEY_SLASH;
	case EStdKeyBackSlash:
		return CCKEY_BACKSLASH;
	case EStdKeySemiColon:
		return CCKEY_SEMICOLON;
	case EStdKeySingleQuote:
		return CCKEY_QUOTE;
	case EStdKeyHash:
		return '#';
	case EStdKeySquareBracketLeft:
		return CCKEY_LBRACKET;
	case EStdKeySquareBracketRight:
		return CCKEY_RBRACKET;
	case EStdKeyMinus:
		return CCKEY_MINUS;
	case EStdKeyEquals:
		return CCKEY_EQUALS;

	case EStdKeyNkpForwardSlash:
		return CCKEY_KP_DIVIDE;
	case EStdKeyNkpAsterisk:
		return CCKEY_KP_MULTIPLY;
	case EStdKeyNkpMinus:
		return CCKEY_KP_MINUS;
	case EStdKeyNkpPlus:
		return CCKEY_KP_PLUS;
	case EStdKeyNkpEnter:
		return CCKEY_KP_ENTER;
	case EStdKeyNkp1:
		return CCKEY_KP1;
	case EStdKeyNkp2:
		return CCKEY_KP2;
	case EStdKeyNkp3:
		return CCKEY_KP3;
	case EStdKeyNkp4:
		return CCKEY_KP4;
	case EStdKeyNkp5:
		return CCKEY_KP5;
	case EStdKeyNkp6:
		return CCKEY_KP6;
	case EStdKeyNkp7:
		return CCKEY_KP7;
	case EStdKeyNkp8:
		return CCKEY_KP8;
	case EStdKeyNkp9:
		return CCKEY_KP9;
	case EStdKeyNkp0:
		return CCKEY_KP0;
	case EStdKeyNkpFullStop:
		return CCKEY_KP_DECIMAL;

	case EStdKeyIncVolume:
		return CCKEY_VOLUME_UP;
	case EStdKeyDecVolume:
		return CCKEY_VOLUME_DOWN;
		
// softkeys
	case EStdKeyDevice0: // left soft
		return CCKEY_F1;
	case EStdKeyDevice1: // right soft
		return CCKEY_ESCAPE;
	case EStdKeyDevice3: // d-pad center
		return CCKEY_ENTER;
	}
	
	return aScanCode;
}

void CWindow::HandleWsEvent(const TWsEvent& aWsEvent) {
	TInt eventType = aWsEvent.Type();
	switch (eventType) {
	case EEventKeyDown: {
		Input_Set(ConvertKey(aWsEvent.Key()->iScanCode), true);
		break;
	}
	case EEventKeyUp: {
		Input_Set(ConvertKey(aWsEvent.Key()->iScanCode), false);
		break;
	}
	case EEventScreenDeviceChanged:
	//case 27: /* EEventDisplayChanged */
	{
		TPixelsTwipsAndRotation pixnrot;
		iWsScreenDevice->GetScreenModeSizeAndRotation(iWsScreenDevice->CurrentScreenMode(), pixnrot);
		if (pixnrot.iPixelSize != iWindow->Size()) {
			iWindow->SetExtent(TPoint(0, 0), pixnrot.iPixelSize);
			
			TInt w = pixnrot.iPixelSize.iWidth,
				h = pixnrot.iPixelSize.iHeight;
			
			DisplayInfo.Width = w;
			DisplayInfo.Height = h;
			
			WindowInfo.Width = w;
			WindowInfo.Height = h;
			
			Event_RaiseVoid(&WindowEvents.Resized);
		}
		Event_RaiseVoid(&WindowEvents.RedrawNeeded);
		break;
	}
	case EEventFocusLost: {
#if 0 // TODO
		if (!WindowInfo.Focused) break;
		WindowInfo.Focused = false;
		
		Event_RaiseVoid(&WindowEvents.FocusChanged);
#endif
		break;
	}
	case EEventFocusGained: {
		if (!WindowInfo.Focused) {
			WindowInfo.Focused = true;
			
			Event_RaiseVoid(&WindowEvents.FocusChanged);
		}
		Event_RaiseVoid(&WindowEvents.RedrawNeeded);
		break;
	}
	// shutdown request from task manager
	case KAknShutOrHideApp: {
		WindowInfo.Exists = false;
		RequestClose();
		break;
	}
	// shutdown request from system (out of memory)
	case EEventUser: {
		TApaSystemEvent apaSystemEvent = *(TApaSystemEvent*) aWsEvent.EventData();
		if (apaSystemEvent == EApaSystemEventShutdown) {
			WindowInfo.Exists = false;
			RequestClose();
		}
		break;
	}
	case EEventWindowVisibilityChanged: {
		if (aWsEvent.Handle() == reinterpret_cast<TUint32>(this)) {
			WindowInfo.Inactive = (aWsEvent.VisibilityChanged()->iFlags & TWsVisibilityChangedEvent::EFullyVisible) == 0;
			Event_RaiseVoid(&WindowEvents.InactiveChanged);
		}
		break;
	}
#ifdef CC_BUILD_TOUCH
	case EEventPointer: {
#ifdef CC_BUILD_SYMBIAN_3
		TAdvancedPointerEvent* pointer = aWsEvent.Pointer();
		long num = pointer->IsAdvancedPointerEvent() ? pointer->PointerNumber() : 0;
#else
		TPointerEvent* pointer = aWsEvent.Pointer();
		long num = 0;
#endif
		TPoint pos = pointer->iPosition;
		switch (pointer->iType) {
		case TPointerEvent::EButton1Down:
			Input_AddTouch(num, pos.iX, pos.iY);
			break;
		case TPointerEvent::EDrag:
			Input_AddTouch(num, pos.iX, pos.iY);
			break;
		case TPointerEvent::EButton1Up:
			Input_RemoveTouch(num, pos.iX, pos.iY);
			break;
		default:
			break;
		}
		break;
	}
#endif
	}
}

void CWindow::AllocFrameBuffer(int width, int height) {
	FreeFrameBuffer();
	if (!iWindowGc) {
		iWindowGc = new (ELeave) CWindowGc(iWsScreenDevice);
		iWindowGc->Construct();
	}
	iBitmap = new CFbsBitmap();
	iBitmap->Create(TSize(width, height), EColor16MA);
}

void CWindow::FreeFrameBuffer() {
	if (iWindowGc != NULL) {
		delete iWindowGc;
		iWindowGc = NULL;
	}
	if (iBitmap != NULL) {
		delete iBitmap;
		iBitmap = NULL;
	}
}

void CWindow::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	if (!iWindowGc) return;
	iWindow->Invalidate(/*TRect(r.x, r.y, r.width, r.height)*/);
	iWindow->BeginRedraw();
	iWindowGc->Activate(*iWindow);

	if (iBitmap) {
//		iBitmap->BeginDataAccess();
		iBitmap->LockHeap();
		TUint8* data = (TUint8*) iBitmap->DataAddress();
		const TUint8* src = (TUint8*) bmp->scan0;
		for (TInt row = bmp->height - 1; row >= 0; --row) {
			memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
			src += bmp->width * BITMAPCOLOR_SIZE;
			data += iBitmap->DataStride();
		}
//		iBitmap->EndDataAccess();
		iBitmap->UnlockHeap();
		
		iWindowGc->BitBlt(TPoint(0, 0), iBitmap/*, TRect(r.x, r.y, r.width, r.height)*/);
	}
	iWindowGc->Deactivate();
	iWindow->EndRedraw();
	iWsSession.Flush();
}

void CWindow::ProcessEvents(float delta) {
	RThread thread;
	TInt error = KErrNone;
	while (thread.RequestCount()) {
		CActiveScheduler::RunIfReady(error, CActive::EPriorityIdle);
		User::WaitForAnyRequest();
	}
	
	while (iWsEventStatus != KRequestPending) {
		iWsSession.GetEvent(window->iWsEvent);
		HandleWsEvent(window->iWsEvent);
		iWsEventStatus = KRequestPending;
		iWsSession.EventReady(&iWsEventStatus);
	}
}

void CWindow::RequestClose() {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void CWindow::InitEvents() {
	iWindow->Invalidate();
	iWindow->BeginRedraw();
	iWindow->EndRedraw();
	iWsSession.Flush();
	if (iEventsInitialized)
		return;
	iEventsInitialized = ETrue;
	iWsEventStatus = KRequestPending;
	iWsSession.EventReady(&iWsEventStatus);
}

cc_result CWindow::OpenBrowserL(const cc_string* url) {
#if defined CC_BUILD_SYMBIAN_3 || defined CC_BUILD_SYMBIAN_S60V5
	TUid browserUid = {0x10008D39};
#else
	TUid browserUid = {0x1020724D};
#endif
	TApaTaskList tasklist(window->iWsSession);
	TApaTask task = tasklist.FindApp(browserUid);
	
	if (task.Exists()) {
		task.BringToForeground();
		task.SendMessage(TUid::Uid(0), TPtrC8((TUint8 *)url->buffer, (TInt)url->length));
	} else {
		RApaLsSession ls;
		if (!ls.Handle()) {
			User::LeaveIfError(ls.Connect());
		}
		
		TThreadId tid;
		TBuf<FILENAME_SIZE> buf;
		ConvertToUnicode(url, buf);
		ls.StartDocument(buf, browserUid, tid);
		ls.Close();
	}
	return 0;
}

void Window_PreInit(void) {
	//NormDevice.defaultBinds = symbian_binds; TODO only use on devices with limited hardware
	
	CCoeEnv* env = new (ELeave) CCoeEnv();
	TRAPD(err, env->ConstructL(ETrue, 0));
	
	if (err != KErrNone) {
		User::Panic(_L("Failed to create CoeEnv"), 0);
	}
}

void Window_Init(void) {
	TRAPD(err, window = CWindow::NewL());
	if (err) {
		User::Panic(_L("Failed to initialize CWindow"), err);
	}
	
#ifdef CC_BUILD_TOUCH
	//TBool touch = AknLayoutUtils::PenEnabled();
	
	bool touch = true;
	Input_SetTouchMode(touch);
	Gui_SetTouchUI(touch);
#endif
}

void Window_Free(void) {
	if (window) {
		delete window;
		window = NULL;
	}
	
	CCoeEnv::Static()->DestroyEnvironment();
	delete CCoeEnv::Static();
}

void Window_Create2D(int width, int height) {
	launcherMode = true;
	window->InitEvents();
}
void Window_Create3D(int width, int height) {
	launcherMode = false;
	window->InitEvents();
}

void Window_Destroy(void) {
}

void Window_SetTitle(const cc_string* title) { }

void Clipboard_GetText(cc_string* value) { }

void Clipboard_SetText(const cc_string* value) { }

int Window_GetWindowState(void) { return WINDOW_STATE_FULLSCREEN; }

cc_result Window_EnterFullscreen(void) { return 0; }

cc_result Window_ExitFullscreen(void)  { return 0; }

int Window_IsObscured(void) {
	return WindowInfo.Inactive;
}

void Window_Show(void) { }

void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	window->RequestClose();
}

void Window_ProcessEvents(float delta) {
	window->ProcessEvents(delta);
}

void Window_EnableRawMouse(void)  { Input.RawMode = true; }

void Window_UpdateRawMouse(void)  {  }

void Window_DisableRawMouse(void) { Input.RawMode = false; }

void Gamepads_Init(void) {
	
}

void Gamepads_Process(float delta) {
	
}

void ShowDialogCore(const char* title, const char* msg) {
	// TODO
	static const cc_string t2 = String_Init((char*) title, String_Length(title), String_Length(title));
	Logger_Log(&t2);
	static const cc_string msg2 = String_Init((char*) msg, String_Length(msg), String_Length(msg));
	Logger_Log(&msg2);
	
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	VirtualKeyboard_Open(args, launcherMode);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Close(void) {
	VirtualKeyboard_Close();
}

void Window_LockLandscapeOrientation(cc_bool lock) {
	// TODO
}

static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
static void Cursor_DoSetVisible(cc_bool visible) { }

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap");
	bmp->width  = width;
	bmp->height = height;
	window->AllocFrameBuffer(width, height);
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	window->DrawFramebuffer(r, bmp);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	window->FreeFrameBuffer();
	Mem_Free(bmp->scan0);
}

#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL1
void GLContext_Create(void) {
	static EGLint attribs[] = {
		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_BUFFER_SIZE,       DisplayInfo.Depth,
		EGL_DEPTH_SIZE,        16,
		EGL_NONE
	};

	ctx_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(ctx_display, NULL, NULL);
	EGLint numConfigs;
	
	eglChooseConfig(ctx_display, attribs, &ctx_config, 1, &numConfigs);
	ctx_context = eglCreateContext(ctx_display, ctx_config, EGL_NO_CONTEXT, NULL);
	if (!ctx_context) Process_Abort2(eglGetError(), "Failed to create EGL context");
	GLContext_InitSurface();
}
#endif

cc_result Process_StartOpen(const cc_string* args) {
	TInt err = 0;
	TRAP(err, err = window->OpenBrowserL(args));
	return (cc_result) err;
}

#endif
