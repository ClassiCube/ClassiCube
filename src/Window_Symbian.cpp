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
#include <aknapp.h>
#include <coecntrl.h>
#include <akndef.h>
#include <akndoc.h>
#include <aknappui.h>
#include <avkon.hrh>
#include <AknUtils.h>
#include <eikstart.h>
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
#include "Platform.h"
#include "Options.h"
#include "Server.h"
#include "LScreens.h"
#include "Http.h"
}

// Event management
enum CCEventType {
	CC_NONE,
	CC_KEY_DOWN, CC_KEY_UP,
	CC_WIN_RESIZED, CC_WIN_FOCUS, CC_WIN_REDRAW, CC_WIN_QUIT,
	CC_TOUCH_ADD, CC_TOUCH_REMOVE
};
struct CCEvent {
	int type;
	int i1, i2, i3;
};

#define EVENTS_DEFAULT_MAX 30
static void* events_mutex;
static int events_count, events_capacity;
static CCEvent* events_list, events_default[EVENTS_DEFAULT_MAX];

static void Events_Init(void) {
	events_mutex    = Mutex_Create("Symbian events");
	events_capacity = EVENTS_DEFAULT_MAX;
	events_list     = events_default;
}

static void Events_Push(const CCEvent* event) {
	if (!events_mutex) return;
	Mutex_Lock(events_mutex);
	{
		if (events_count >= events_capacity) {
			Utils_Resize((void**)&events_list, &events_capacity,
						sizeof(CCEvent), EVENTS_DEFAULT_MAX, 20);
		}
		events_list[events_count++] = *event;
	}
	Mutex_Unlock(events_mutex);
}

static cc_bool Events_Pull(CCEvent* event) {
	cc_bool found = false;
	
	Mutex_Lock(events_mutex);
	{
		if (events_count) {
			*event = events_list[0];
			for (int i = 1; i < events_count; i++) {
				events_list[i - 1] = events_list[i];
			}
			events_count--;
			found = true;
		}
	}
	Mutex_Unlock(events_mutex);
	return found;
}

static cc_bool launcherMode;
static cc_bool gameInitialized;

static void ConvertToUnicode(TDes& dst, const char* src, size_t length);
static void ConvertToUnicode(TDes& dst, const cc_string* src);
static int ConvertKey(TInt aScanCode);

const TUid KUidClassiCube = {0xE212A5C2};

class CClassiCubeContainer;
static CClassiCubeContainer* container;

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

class CClassiCubeApp: public CAknApplication
{
private:
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
};
LOCAL_C CApaApplication* NewApplication();

GLDEF_C TInt E32Main();

//

class CClassiCubeContainer: public CCoeControl, MCoeControlObserver
{
public:
	void ConstructL(const TRect& aRect, CAknAppUi* aAppUi);
	virtual ~CClassiCubeContainer();
	
	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
	static TInt DrawCallBack(TAny* aInstance);

private:
	void SizeChanged();
	void HandleResourceChange(TInt aType);
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void Draw(const TRect& aRect) const;
	virtual void HandleControlEventL(CCoeControl*, TCoeEvent);
	virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);

public:
	CAknAppUi*  iAppUi;
	CFbsBitmap* iBitmap;
	CPeriodic*  iPeriodic;
};

//

class CClassiCubeDocument: public CAknDocument
{
public:
	static CClassiCubeDocument* NewL(CEikApplication& aApp);
	virtual ~CClassiCubeDocument();

protected:
	void ConstructL();

public:
	CClassiCubeDocument(CEikApplication& aApp);

private:
	CEikAppUi* CreateAppUiL();
};

//

class CClassiCubeAppUi: public CAknAppUi
{
public:
	void ConstructL();
	virtual ~CClassiCubeAppUi();

private:
	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
	void HandleCommandL(TInt aCommand);
	virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

private:
	CClassiCubeContainer* iAppContainer;
};

// CClassiCubeApp implementation

TUid CClassiCubeApp::AppDllUid() const
{
	return KUidClassiCube;
}

CApaDocument* CClassiCubeApp::CreateDocumentL()
{
	return CClassiCubeDocument::NewL(*this);
}

CApaApplication* NewApplication()
{
	return new CClassiCubeApp;
}

TInt E32Main()
{
	return EikStart::RunApplication(NewApplication);
}

// CClassiCubeAppUi implementation

void CClassiCubeAppUi::ConstructL()
{
	BaseConstructL();
	iAppContainer = new (ELeave) CClassiCubeContainer;
	iAppContainer->SetMopParent(this);
	iAppContainer->ConstructL(ClientRect(), this);
	AddToStackL(iAppContainer);
}

CClassiCubeAppUi::~CClassiCubeAppUi()
{
	if (iAppContainer) {
		RemoveFromStack(iAppContainer);
		delete iAppContainer;
	}
}

void CClassiCubeAppUi::DynInitMenuPaneL(TInt /*aResourceId*/, CEikMenuPane* /*aMenuPane*/)
{
}

TKeyResponse CClassiCubeAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
	if (!events_mutex) return EKeyWasNotConsumed;
	
	CCEvent event = { 0 };
	event.i1 = aKeyEvent.iScanCode;
	switch (aType) {
	case EEventKeyDown:
		event.type = CC_KEY_DOWN;
		Events_Push(&event);
		return EKeyWasConsumed;
	case EEventKeyUp:
		event.type = CC_KEY_UP;
		Events_Push(&event);
		return EKeyWasConsumed;
	default:
		return EKeyWasNotConsumed;
	}
}

void CClassiCubeAppUi::HandleCommandL(TInt aCommand)
{	
	switch (aCommand) {
	case EAknSoftkeyBack:
	case EEikCmdExit:
	{
		if (events_mutex) {
			CCEvent event = { 0 };
			event.type = CC_WIN_QUIT;
			Events_Push(&event);
		}
		Exit();
		break;
	}
	default:
		break;
	}
}

// CClassiCubeContainer implementation

extern "C" void Launcher_Run(void);
extern "C" struct LScreen* Launcher_Active;
extern "C" cc_bool Launcher_ShouldExit;
extern "C" void Launcher_Free(void);
extern "C" void Game_DoFrame(void);

TInt CClassiCubeContainer::DrawCallBack(TAny* aInstance)
{
	if (launcherMode) {
		Window_ProcessEvents(10 / 1000.0f);
		Gamepad_Tick(10 / 1000.0f);
		
		if (Window_Main.Exists && !Launcher_ShouldExit) {
			Launcher_Active->Tick(Launcher_Active);
			LBackend_Tick();
			Thread_Sleep(10);
			return 0;
		}

		Launcher_Free();
		Launcher_ShouldExit = false;
		launcherMode = false;
		
		Drawer2D_Component.Free();
		Http_Component.Free();
	}
	if (!Window_Main.Exists) {
		Window_RequestClose();
		return 0;
	}
	// init game
	if (!gameInitialized) {
		// trigger relayout to clear launcher contents
		((CClassiCubeContainer*) aInstance)->SetExtentToWholeScreen();
		Game_Run(WindowInfo.Width, WindowInfo.Height, NULL);
		gameInitialized = true;
	}
	Game_DoFrame();
	return 0;
}

void CClassiCubeContainer::ConstructL(const TRect& aRect, CAknAppUi* aAppUi)
{
	iAppUi = aAppUi;
	
	// create window

	CreateWindowL();
	SetExtentToWholeScreen();
	
	// enable multi touch and drag events
	Window().EnableAdvancedPointers();
	EnableDragEvents();
	
	ActivateL();
	
	container = this;
	
	// init classicube (from main.c)
	
	static char ipBuffer[STRING_SIZE];
	CrashHandler_Install();
	Logger_Hook();
	Window_PreInit();
	Platform_Init();
	Options_Load();
	Window_Init();
	Gamepads_Init();
	Platform_LogConst("Starting " GAME_APP_NAME " ..");
	String_InitArray(Server.Address, ipBuffer);

	TSize size = Size();
	WindowInfo.Focused = true;
	WindowInfo.Exists = true;
	WindowInfo.Handle.ptr = (void*) &Window();
#ifdef CC_BUILD_TOUCH
	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_VIRTUAL;
#endif

	DisplayInfo.Width = size.iWidth;
	DisplayInfo.Height = size.iHeight;

	WindowInfo.Width = size.iWidth;
	WindowInfo.Height = size.iHeight;

	WindowInfo.UIScaleX = DEFAULT_UI_SCALE_X;
	WindowInfo.UIScaleY = DEFAULT_UI_SCALE_Y;
	if (size.iWidth <= 360) {
		DisplayInfo.ScaleX = 0.5f;
		DisplayInfo.ScaleY = 0.5f;
	} else {
		DisplayInfo.ScaleX = 1;
		DisplayInfo.ScaleY = 1;
	}
	
	TDisplayMode displayMode = Window().DisplayMode();
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
	
	// run launcher
	
	Launcher_Run();
	
	iPeriodic = CPeriodic::NewL( CActive::EPriorityIdle );
	iPeriodic->Start(100, 100, TCallBack( CClassiCubeContainer::DrawCallBack, this));
}

CClassiCubeContainer::~CClassiCubeContainer()
{
	delete iPeriodic;
}

void CClassiCubeContainer::SizeChanged()
{
	TSize size = Size();
	if (iBitmap) {
		delete iBitmap;
		iBitmap = NULL;
	}
	iBitmap = new CFbsBitmap();
	TInt err = iBitmap->Create(size, EColor16MA);
	if (err) {
		User::Panic(_L("Bitmap"), err);
	}
	
	DisplayInfo.Width = size.iWidth;
	DisplayInfo.Height = size.iHeight;

	WindowInfo.Width = size.iWidth;
	WindowInfo.Height = size.iHeight;

	CCEvent event = { 0 };
	event.type = CC_WIN_RESIZED;
	event.i1 = size.iWidth;
	event.i2 = size.iHeight;
	Events_Push(&event);
	DrawNow();
}

void CClassiCubeContainer::HandleResourceChange(TInt aType)
{
	switch (aType) {
	case KEikDynamicLayoutVariantSwitch:
		SetExtentToWholeScreen();
		break;
	}
}

TInt CClassiCubeContainer::CountComponentControls() const
{
	return 0;
}

CCoeControl* CClassiCubeContainer::ComponentControl(TInt /*aIndex*/) const
{
	return NULL;
}

void CClassiCubeContainer::Draw(const TRect& aRect) const
{
	if (iBitmap && launcherMode) {
		SystemGc().BitBlt(TPoint(0, 0), iBitmap);
	}
	// do nothing in gl mode
}

void CClassiCubeContainer::DrawFramebuffer(Rect2D r, struct Bitmap* bmp)
{
	if (iBitmap) {
		iBitmap->LockHeap();
		TUint8* data = (TUint8*) iBitmap->DataAddress();
		if (!data) {
			User::Panic(_L("DataAddress"), 0);
		}
		const TUint8* src = (TUint8*) bmp->scan0;
		for (TInt row = bmp->height - 1; row >= 0; --row) {
			memcpy(data, src, bmp->width * BITMAPCOLOR_SIZE);
			src += bmp->width * BITMAPCOLOR_SIZE;
			data += iBitmap->DataStride();
		}
		iBitmap->UnlockHeap();
	}
	DrawDeferred();
}

void CClassiCubeContainer::HandleControlEventL(CCoeControl*, TCoeEvent) {
	
}

void CClassiCubeContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
#ifdef CC_BUILD_TOUCH
	CCEvent event = { 0 };
#ifdef CC_BUILD_SYMBIAN_3
	const TAdvancedPointerEvent* advpointer = aPointerEvent.AdvancedPointerEvent();
	event.i1 = advpointer != NULL ? advpointer->PointerNumber() : 0;
#else
	event.i1 = 0;
#endif
	TPoint pos = aPointerEvent.iPosition;
	event.i2 = pos.iX;
	event.i3 = pos.iY;
	switch (aPointerEvent.iType) {
	case TPointerEvent::EButton1Down:
		event.type = CC_TOUCH_ADD;
		break;
	case TPointerEvent::EDrag:
		event.type = CC_TOUCH_ADD;
		break;
	case TPointerEvent::EButton1Up:
		event.type = CC_TOUCH_REMOVE;
		break;
	default:
		break;
	}
	if (event.type) Events_Push(&event);
#endif
	CCoeControl::HandlePointerEventL(aPointerEvent);
}

// CClassiCubeDocument implementation

CClassiCubeDocument::CClassiCubeDocument(CEikApplication& aApp) :
	CAknDocument(aApp)
{
}

CClassiCubeDocument::~CClassiCubeDocument()
{
}

void CClassiCubeDocument::ConstructL()
{
}

CClassiCubeDocument* CClassiCubeDocument::NewL(CEikApplication& aApp)
{
	CClassiCubeDocument* self = new (ELeave) CClassiCubeDocument(aApp);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();

	return self;
}

CEikAppUi* CClassiCubeDocument::CreateAppUiL()
{
	return new (ELeave) CClassiCubeAppUi;
}

//

cc_result OpenBrowserL(const cc_string* url)
{
#if defined CC_BUILD_SYMBIAN_3 || defined CC_BUILD_SYMBIAN_S60V5
	TUid browserUid = {0x10008D39};
#else
	TUid browserUid = {0x1020724D};
#endif
	TApaTaskList tasklist(CEikonEnv::Static()->WsSession());
	TApaTask task = tasklist.FindApp(browserUid);

	if (task.Exists()) {
		task.BringToForeground();
		task.SendMessage(TUid::Uid(0), TPtrC8((TUint8 *) url->buffer, (TInt) url->length));
	}
	else {
		RApaLsSession ls;
		if (!ls.Handle()) {
			User::LeaveIfError(ls.Connect());
		}

		TThreadId tid;
		TBuf<FILENAME_SIZE> buf;
		ConvertToUnicode(buf, url);
		ls.StartDocument(buf, browserUid, tid);
		ls.Close();
	}
	return 0;
}

void Window_PreInit(void)
{
	//NormDevice.defaultBinds = symbian_binds; TODO only use on devices with limited hardware
}

void Window_Init(void)
{
	Events_Init();
#ifdef CC_BUILD_TOUCH
	//TBool touch = AknLayoutUtils::PenEnabled();

	bool touch = true;
	Input_SetTouchMode(touch);
	Gui_SetTouchUI(touch);
#endif
}

void Window_Free(void)
{
}

void Window_Create2D(int width, int height)
{
	launcherMode = true;
}
void Window_Create3D(int width, int height)
{
	launcherMode = false;
}

void Window_Destroy(void)
{
}

void Window_SetTitle(const cc_string* title)
{
}

void Clipboard_GetText(cc_string* value)
{
}

void Clipboard_SetText(const cc_string* value)
{
}

int Window_GetWindowState(void)
{
	return WINDOW_STATE_FULLSCREEN;
}

cc_result Window_EnterFullscreen(void)
{
	return 0;
}

cc_result Window_ExitFullscreen(void)
{
	return 0;
}

int Window_IsObscured(void)
{
	return WindowInfo.Inactive;
}

void Window_Show(void)
{
}

void Window_SetSize(int width, int height)
{
}

void Window_RequestClose(void)
{
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
	container->iAppUi->Exit();
}

void Window_ProcessEvents(float delta) {
	CCEvent event;
	int key;
	
	while (Events_Pull(&event))
	{
		switch (event.type)
		{
		case CC_KEY_DOWN:
			key = ConvertKey(event.i1);
			if (key) Input_SetPressed(key);
			break; 
		case CC_KEY_UP:
			key = ConvertKey(event.i1);
			if (key) Input_SetReleased(key);
			break;
		case CC_WIN_RESIZED:
			Window_Main.Width  = event.i1;
			Window_Main.Height = event.i2;
			Event_RaiseVoid(&WindowEvents.Resized);
			break;
		case CC_WIN_FOCUS:
			Window_Main.Focused = event.i1;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			break;
		case CC_WIN_REDRAW:
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
			break;
		case CC_WIN_QUIT:
			Window_Main.Exists = false;
			Window_RequestClose();
			break;
#ifdef CC_BUILD_TOUCH
		case CC_TOUCH_ADD:
			Input_AddTouch(static_cast<long>(event.i1), event.i2, event.i3);
			break;
		case CC_TOUCH_REMOVE:
			Input_RemoveTouch(static_cast<long>(event.i1), event.i2, event.i3);
			break;
#endif
		}
	}
}

void Window_EnableRawMouse(void)
{
	Input.RawMode = true;
}

void Window_UpdateRawMouse(void)
{
}

void Window_DisableRawMouse(void)
{
	Input.RawMode = false;
}

void Gamepads_Init(void)
{

}

void Gamepads_Process(float delta)
{

}

static void ShowDialogL(const char* title, const char* msg) {
	CAknInformationNote* note = new CAknInformationNote();
	TBuf<512> titleBuf;
	ConvertToUnicode(titleBuf, title, String_Length(title));
	note->SetTitleL(titleBuf);
	
	TBuf<512> msgBuf;
	ConvertToUnicode(msgBuf, msg, String_Length(msg));
	note->ExecuteLD(msgBuf);
}

void ShowDialogCore(const char* title, const char* msg)
{
	TRAP_IGNORE(ShowDialogL(title, msg));
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args)
{
	VirtualKeyboard_Open(args, launcherMode);
}

void OnscreenKeyboard_SetText(const cc_string* text)
{
	VirtualKeyboard_SetText(text);
}

void OnscreenKeyboard_Close(void)
{
	VirtualKeyboard_Close();
}

void Window_LockLandscapeOrientation(cc_bool lock)
{
	container->iAppUi->SetOrientationL(lock ? CAknAppUiBase::EAppUiOrientationLandscape : CAknAppUiBase::EAppUiOrientationAutomatic);
}

static void Cursor_GetRawPos(int* x, int* y)
{
	*x = 0;
	*y = 0;
}
void Cursor_SetPosition(int x, int y)
{
}
static void Cursor_DoSetVisible(cc_bool visible)
{
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args)
{
	return ERR_NOT_SUPPORTED;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args)
{
	return ERR_NOT_SUPPORTED;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height)
{
	bmp->scan0 = (BitmapCol*) Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "bitmap");
	bmp->width = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp)
{
	container->DrawFramebuffer(r, bmp);
}

void Window_FreeFramebuffer(struct Bitmap* bmp)
{
	Mem_Free(bmp->scan0);
}

#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL1
void GLContext_Create(void) {
	static EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BUFFER_SIZE, DisplayInfo.Depth,
		EGL_DEPTH_SIZE, 16,
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

cc_result Process_StartOpen(const cc_string* args)
{
	TInt err = 0;
	TRAP(err, err = OpenBrowserL(args));
	return (cc_result) err;
}

static void ConvertToUnicode(TDes& dst, const char* src, size_t length) {
	if (!src) return;

	cc_unichar* uni = reinterpret_cast<cc_unichar*>(const_cast <TUint16*> (dst.Ptr()));
	for (int i = 0; i < length; i++) {
		*uni++ = Convert_CP437ToUnicode(src[i]);
	}
	*uni = '\0';
	dst.SetLength(length);
}

static void ConvertToUnicode(TDes& dst, const cc_string* src) {
	ConvertToUnicode(dst, src->buffer, (size_t)src->length);
}

static int ConvertKey(TInt aScanCode)
{
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

#endif
