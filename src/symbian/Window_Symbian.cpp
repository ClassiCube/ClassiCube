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
#include <aknmessagequerydialog.h>
#include <baclipb.h>
#include <s32ucmp.h>
#include <classicube.rsg>
#include <e32property.h>
#include <hal.h>
extern "C" {
#include <stdapis/string.h>
#include <gles/egl.h>
#include "_WindowBase.h"
#include "Errors.h"
#include "Logger.h"
#include "String_.h"
#include "Gui.h"
#include "Graphics.h"
#include "Game.h"
#include "VirtualKeyboard.h"
#include "Platform.h"
#include "Options.h"
#include "Server.h"
#include "LScreens.h"
#include "Http.h"
#include "main_impl.h"
}

class CCContainer;

static cc_bool gameRunning = false;

const TUid KUidClassiCube = {0xE212A5C2};

static CCContainer* container;

// 12 keys
const BindMapping symbian_binds_12[BIND_COUNT] = {
	{ '2', 0 }, { '8', 0 }, { '4', 0 }, { '6', 0 }, /* BIND_FORWARD - BIND_RIGHT */
	{ CCKEY_ENTER, 0 },  { '3', 0 },                /* BIND_JUMP, BIND_RESPAWN */
	{ 0, 0 },            { 0, 0 },                  /* BIND_SET_SPAWN, BIND_CHAT */
	{ '1', 0 },          { 0, 0 },                  /* BIND_INVENTORY, BIND_FOG */
	{ 0, 0 },            { 0, 0 },                  /* BIND_SEND_CHAT, BIND_TABLIST */
	{ 0, 0 }, { '7', 0 },{ '9', 0 },                /* BIND_SPEED, BIND_NOCLIP, BIND_FLY */ 
	{ 0, 0 },            { '0', 0 },                /* BIND_FLY_UP, BIND_FLY_DOWN */
	{ 0, 0 },            { 0, 0 },                  /* BIND_EXT_INPUT, BIND_HIDE_FPS */
	{ 0, 0 },            { 0, 0 },                  /* BIND_SCREENSHOT, BIND_FULLSCREEN */
	{ 0, 0 },            { 0, 0 },                  /* BIND_THIRD_PERSON, BIND_HIDE_GUI */ 
	{ 0, 0 }, { 0, 0 },  { 0, 0 },                  /* BIND_AXIS_LINES, BIND_ZOOM_SCROLL, BIND_HALF_SPEED */
	{ '5', 0 }, { 0, 0 },{ CCKEY_F1, 0},            /* BIND_DELETE_BLOCK, BIND_PICK_BLOCK, BIND_PLACE_BLOCK */
	{ 0, 0 },            { 0, 0 },                  /* BIND_AUTOROTATE, BIND_HOTBAR_SWITCH */

	{ 0, 0 },            { CCKEY_BACKSPACE, 0 },    /* BIND_SMOOTH_CAMERA, BIND_DROP_BLOCK */
	{ 0, 0 },            { 0, 0 },                  /* BIND_IDOVERLAY, BIND_BREAK_LIQUIDS */
	{ CCKEY_UP, 0 }, { CCKEY_DOWN, 0 }, { CCKEY_RIGHT, 0 }, { CCKEY_LEFT, 0 }, /* BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_RIGHT, BIND_LOOK_LEFT */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },                   /* BIND_HOTBAR_1, BIND_HOTBAR_2, BIND_HOTBAR_3 */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },                   /* BIND_HOTBAR_4, BIND_HOTBAR_5, BIND_HOTBAR_6 */
	{ 0, 0 }, { 0, 0 }, { 0, 0 },                   /* BIND_HOTBAR_7, BIND_HOTBAR_8, BIND_HOTBAR_9 */
	{ CCKEY_LWIN, 0 }, { CCKEY_BACKSLASH, 0 }       /* BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT */
};

// QWERTY
const BindMapping symbian_binds_qwerty[BIND_COUNT] = {
	{ 'W', 0 }, { 'S', 0 }, { 'A', 0 }, { 'D', 0 }, /* BIND_FORWARD - BIND_RIGHT */
	{ CCKEY_SPACE, 0 },  { 'I', 0 },                /* BIND_JUMP, BIND_RESPAWN */
	{ 0, 0 },            { 'K', 0 },                /* BIND_SET_SPAWN, BIND_CHAT */
	{ CCKEY_F1, 0 },     { 'L', 0 },                /* BIND_INVENTORY, BIND_FOG */
	{ CCKEY_ENTER, 0 },  { CCKEY_TAB, 0 },          /* BIND_SEND_CHAT, BIND_TABLIST */
	{ CCKEY_LSHIFT, 0 }, { 'X', 0}, { 'Z', 0 },     /* BIND_SPEED, BIND_NOCLIP, BIND_FLY */ 
	{ 'C', 0 },          { 'D', 0 },                /* BIND_FLY_UP, BIND_FLY_DOWN */
	{ CCKEY_LALT, 0 },   { 0, 0 },                  /* BIND_EXT_INPUT, BIND_HIDE_FPS */
	{ 0, 0 },            { 0, 0 },                  /* BIND_SCREENSHOT, BIND_FULLSCREEN */
	{ 0, 0 },            { 0, 0 },                  /* BIND_THIRD_PERSON, BIND_HIDE_GUI */ 
	{ 0, 0 }, { 0, 0 },  { CCKEY_LCTRL, 0 },        /* BIND_AXIS_LINES, BIND_ZOOM_SCROLL, BIND_HALF_SPEED */
	{ 'E', 0 }, { 0, 0 },{ 'Q', 0},                 /* BIND_DELETE_BLOCK, BIND_PICK_BLOCK, BIND_PLACE_BLOCK */
	{ 0, 0 },            { 0, 0 },                  /* BIND_AUTOROTATE, BIND_HOTBAR_SWITCH */

	{ 0, 0 },            { CCKEY_BACKSPACE, 0 },    /* BIND_SMOOTH_CAMERA, BIND_DROP_BLOCK */
	{ 0, 0 },            { 0, 0 },                  /* BIND_IDOVERLAY, BIND_BREAK_LIQUIDS */
	{ CCKEY_UP, 0 }, { CCKEY_DOWN, 0 }, { CCKEY_RIGHT, 0 }, { CCKEY_LEFT, 0 }, /* BIND_LOOK_UP, BIND_LOOK_DOWN, BIND_LOOK_RIGHT, BIND_LOOK_LEFT */
	{ '1', 0 }, { '2', 0 }, { '3', 0 },             /* BIND_HOTBAR_1, BIND_HOTBAR_2, BIND_HOTBAR_3 */
	{ '4', 0 }, { '5', 0 }, { '6', 0 },             /* BIND_HOTBAR_4, BIND_HOTBAR_5, BIND_HOTBAR_6 */
	{ '7', 0 }, { '8', 0 }, { '9', 0 },             /* BIND_HOTBAR_7, BIND_HOTBAR_8, BIND_HOTBAR_9 */
	{ 'O', 0 }, { 'P', 0 }                          /* BIND_HOTBAR_LEFT, BIND_HOTBAR_RIGHT */
};

// Event management
enum CCEventType {
	CC_NONE,
	CC_KEY_DOWN, CC_KEY_UP, CC_KEY_INPUT,
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

class CCApp: public CAknApplication {
private:
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
};
LOCAL_C CApaApplication* NewApplication();

GLDEF_C TInt E32Main();

class CCContainer: public CCoeControl, MCoeControlObserver {
public:
	void ConstructL(const TRect& aRect, CAknAppUi* aAppUi);
	virtual ~CCContainer();
	
	void DrawFramebuffer(Rect2D r, struct Bitmap* bmp);
	static TInt LoopCallBack(TAny* aInstance);
	void RestartTimerL(TInt aInterval);

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

class CCDocument: public CAknDocument {
public:
	static CCDocument* NewL(CEikApplication& aApp);
	virtual ~CCDocument();

protected:
	void ConstructL();

public:
	CCDocument(CEikApplication& aApp);

private:
	CEikAppUi* CreateAppUiL();
};

class CCAppUi: public CAknAppUi {
public:
	void ConstructL();
	virtual ~CCAppUi();

private:
	void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
	void HandleCommandL(TInt aCommand);
	virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
	virtual void HandleForegroundEventL(TBool aForeground);

private:
	CCContainer* iAppContainer;
};

// CCApp implementation

TUid CCApp::AppDllUid() const {
	return KUidClassiCube;
}

CApaDocument* CCApp::CreateDocumentL() {
	return CCDocument::NewL(*this);
}

CApaApplication* NewApplication() {
	return new CCApp;
}

TInt E32Main() {
	return EikStart::RunApplication(NewApplication);
}

// CCAppUi implementation

void CCAppUi::ConstructL() {
	BaseConstructL(CAknAppUi::EAknEnableSkin);
	iAppContainer = new (ELeave) CCContainer;
	iAppContainer->SetMopParent(this);
	iAppContainer->ConstructL(ClientRect(), this);
	AddToStackL(iAppContainer);
}

CCAppUi::~CCAppUi() {
	if (iAppContainer) {
		RemoveFromStack(iAppContainer);
		delete iAppContainer;
	}
}

void CCAppUi::DynInitMenuPaneL(TInt, CEikMenuPane*) { }

static CC_INLINE int MapScanCode(TInt aScanCode, TInt aModifiers) {
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
		if (aModifiers & EModifierRotateBy90) {
			return CCKEY_UP;
		}
		if (aModifiers & EModifierRotateBy180) {
			return CCKEY_RIGHT;
		}
		if (aModifiers & EModifierRotateBy270) {
			return CCKEY_DOWN;
		}
		return CCKEY_LEFT;
	case EStdKeyRightArrow:
		if (aModifiers & EModifierRotateBy90) {
			return CCKEY_DOWN;
		}
		if (aModifiers & EModifierRotateBy180) {
			return CCKEY_LEFT;
		}
		if (aModifiers & EModifierRotateBy270) {
			return CCKEY_UP;
		}
		return CCKEY_RIGHT;
	case EStdKeyUpArrow:
		if (aModifiers & EModifierRotateBy90) {
			return CCKEY_RIGHT;
		}
		if (aModifiers & EModifierRotateBy180) {
			return CCKEY_DOWN;
		}
		if (aModifiers & EModifierRotateBy270) {
			return CCKEY_LEFT;
		}
		return CCKEY_UP;
	case EStdKeyDownArrow:
		if (aModifiers & EModifierRotateBy90) {
			return CCKEY_LEFT;
		}
		if (aModifiers & EModifierRotateBy180) {
			return CCKEY_UP;
		}
		if (aModifiers & EModifierRotateBy270) {
			return CCKEY_RIGHT;
		}
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

	return aScanCode < INPUT_COUNT ? aScanCode : INPUT_NONE;
}

TKeyResponse CCAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType) {
	if (!events_mutex) return EKeyWasNotConsumed;
	
	CCEvent event = { 0 };
	event.i1 = MapScanCode(aKeyEvent.iScanCode, aKeyEvent.iModifiers);
	switch (aType) {
	case EEventKey: {
		int code = aKeyEvent.iCode;
		if (code != 0 && (code < ENonCharacterKeyBase || code > ENonCharacterKeyBase + ENonCharacterKeyCount)) {
			event.i1 = code;
			event.type = CC_KEY_INPUT;
			Events_Push(&event);
			return EKeyWasConsumed;
		}
		break;
	}
	case EEventKeyDown: {
		if (event.i1 != INPUT_NONE) {
			event.type = CC_KEY_DOWN;
			Events_Push(&event);
		}
		return EKeyWasConsumed;
	}
	case EEventKeyUp: {
		if (event.i1 != INPUT_NONE) {
			event.type = CC_KEY_UP;
			Events_Push(&event);
		}
		return EKeyWasConsumed;
	}
	}
	return EKeyWasNotConsumed;
}

void CCAppUi::HandleForegroundEventL(TBool aForeground) {
	WindowInfo.Inactive = !aForeground;
	Event_RaiseVoid(&WindowEvents.InactiveChanged);
	
}

void CCAppUi::HandleCommandL(TInt aCommand) {	
	switch (aCommand) {
	case EAknSoftkeyBack:
	case EEikCmdExit: {
		WindowInfo.Exists = false;
		Window_RequestClose();
		Exit();
		break;
	}
	default:
		break;
	}
}

extern cc_bool crashed;

// CCContainer implementation
TInt CCContainer::LoopCallBack(TAny*) {
	if (crashed) {
		return EFalse;
	}
	cc_bool run = false;
	for (;;) {
		if (!WindowInfo.Exists) {
			Window_RequestClose();
			container->iAppUi->Exit();
			return EFalse;
		}
		
		if (run) {
			run = false;
			gameRunning = true;
			ProcessProgramArgs(0, 0);
			Game_Setup();
			container->RestartTimerL(100);
		}
		
		if (!gameRunning) {
			if (Launcher_Tick()) break;
			Launcher_Finish();
			run = true;
			continue;
		}
		
		if (!Game_Running) {
			gameRunning = false;
			Game_Free();
//			Launcher_Setup();
//			container->RestartTimerL(10000);
			WindowInfo.Exists = false;
			continue;
		}
		
		Game_RenderFrame();
		break;
	}
	return ETrue;
}

void CCContainer::RestartTimerL(TInt aInterval) {
	if (iPeriodic) {
		iPeriodic->Cancel();
	} else {
		iPeriodic = CPeriodic::NewL(CActive::EPriorityIdle);
	}
	iPeriodic->Start(aInterval, aInterval, TCallBack(CCContainer::LoopCallBack, this));
}

void CCContainer::ConstructL(const TRect& aRect, CAknAppUi* aAppUi) {
	iAppUi = aAppUi;
	
	// create window
	CreateWindowL();
	SetExtentToWholeScreen();
	
	// enable multi-touch and drag events
#ifdef CC_BUILD_SYMBIAN_3
	Window().EnableAdvancedPointers();
#endif
	EnableDragEvents();
	
	ActivateL();
	container = this;
	
	SetupProgram(0, 0);

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

	Launcher_Setup();
	// reduced tickrate for launcher
	RestartTimerL(10000);
}

CCContainer::~CCContainer() {
	delete iPeriodic;
}

void CCContainer::SizeChanged() {
	TSize size = Size();
	if (iBitmap) {
		delete iBitmap;
		iBitmap = NULL;
	}
	iBitmap = new CFbsBitmap();
	TInt err = iBitmap->Create(size, EColor16MA);
	if (err) {
		Process_Abort("Failed to create bitmap");
		return;
	}
	
	DisplayInfo.Width  = size.iWidth;
	DisplayInfo.Height = size.iHeight;

	if (Window_Main.Is3D) {
		if (size.iWidth <= 360) {
			DisplayInfo.ScaleX = 0.5f;
			DisplayInfo.ScaleY = 0.5f;
		} else {
			DisplayInfo.ScaleX = 1;
			DisplayInfo.ScaleY = 1;
		}
	}

	WindowInfo.Width  = size.iWidth;
	WindowInfo.Height = size.iHeight;
	Event_RaiseVoid(&WindowEvents.Resized);
	DrawNow();
}

void CCContainer::HandleResourceChange(TInt aType) {
	switch (aType) {
	case KEikDynamicLayoutVariantSwitch:
		SetExtentToWholeScreen();
		break;
	}
}

TInt CCContainer::CountComponentControls() const {
	return 0;
}

CCoeControl* CCContainer::ComponentControl(TInt) const {
	return NULL;
}

void CCContainer::Draw(const TRect& aRect) const {
#if CC_GFX_BACKEND_IS_GL()
	if (Window_Main.Is3D) return;
#endif
	if (iBitmap) {
		SystemGc().BitBlt(TPoint(0, 0), iBitmap);
	}
}

void CCContainer::DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	if (iBitmap) {
		iBitmap->LockHeap();
		TUint8* data = (TUint8*) iBitmap->DataAddress();
		if (!data) {
			Process_Abort("Bitmap data address is null");
			return;
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

void CCContainer::HandleControlEventL(CCoeControl*, TCoeEvent) {
}

void CCContainer::HandlePointerEventL(const TPointerEvent& aPointerEvent) {
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

// CCDocument implementation

CCDocument::CCDocument(CEikApplication& aApp) :
	CAknDocument(aApp) {
}

CCDocument::~CCDocument() {
}

void CCDocument::ConstructL() {
}

CCDocument* CCDocument::NewL(CEikApplication& aApp) {
	CCDocument* self = new (ELeave) CCDocument(aApp);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();

	return self;
}

CEikAppUi* CCDocument::CreateAppUiL() {
	return new (ELeave) CCAppUi;
}

static void ConvertToUnicode(TDes& dst, const char* src, size_t length) {
	if (!src) return;

	wchar_t* uni = reinterpret_cast<wchar_t*>(const_cast <TUint16*> (dst.Ptr()));
	mbstowcs(uni, src, length);
	
	dst.SetLength(length);
}

static CC_INLINE void ConvertToUnicode(TDes& dst, const cc_string* src) {
	if (!src->buffer) return;
	size_t length = (size_t)src->length;

	cc_unichar* uni = reinterpret_cast<cc_unichar*>(const_cast <TUint16*> (dst.Ptr()));
	for (int i = 0; i < length; i++) {
		*uni++ = Convert_CP437ToUnicode(src->buffer[i]);
	}
	*uni = '\0';
	dst.SetLength(length);
}

static cc_result OpenBrowserL(const cc_string* url) {
	TUid browserUid = {0x10008D39};
	TApaTaskList tasklist(CEikonEnv::Static()->WsSession());
	TApaTask task = tasklist.FindApp(browserUid);

	if (task.Exists()) {
		task.BringToForeground();
		task.SendMessage(TUid::Uid(0), TPtrC8((TUint8*) url->buffer, (TInt) url->length));
	} else {
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

static void ShowDialogL(const char* title, const char* msg) {
	TBuf<512> msgBuf;
	ConvertToUnicode(msgBuf, msg, String_Length(msg));
	
	CAknMessageQueryDialog* dialog = CAknMessageQueryDialog::NewL(msgBuf);
	dialog->PrepareLC(R_QUERY_DIALOG);
	
	TBuf<100> titleBuf;
	ConvertToUnicode(titleBuf, title, String_Length(title));
	dialog->SetHeaderTextL(titleBuf);
	
	dialog->RunLD();
}

static void GetClipboardL(cc_string* value) {
	TUid uid = {268450333}; // KClipboardUidTypePlainText
	
	CClipboard* cb = CClipboard::NewForReadingLC(CCoeEnv::Static()->FsSession());
	TStreamId stid = cb->StreamDictionary().At(uid);
	if (stid != KNullStreamId) {
		RStoreReadStream stream;
		stream.OpenLC(cb->Store(), stid);
		const TInt32 size = stream.ReadInt32L();
		HBufC* buf = HBufC::NewLC(size);
		buf->Des().SetLength(size);

		TUnicodeExpander e;
		TMemoryUnicodeSink sink(&buf->Des()[0]);
		e.ExpandL(sink, stream, size);

		String_AppendUtf16(value, buf->Ptr(), buf->Length() * 2);

		stream.Close();
		CleanupStack::Pop();
		CleanupStack::PopAndDestroy(buf);
	}
	CleanupStack::PopAndDestroy(cb);
}

static void SetClipboardL(const cc_string* value) {
	TUid uid = {268450333}; // KClipboardUidTypePlainText
	
	HBufC* buf = HBufC::NewLC(value->length + 1);
	TPtr16 des = buf->Des();
	ConvertToUnicode(des, value);
	
	CClipboard* cb = CClipboard::NewForWritingLC(CCoeEnv::Static()->FsSession());

	RStoreWriteStream stream;
	TStreamId stid = stream.CreateLC(cb->Store());
	stream.WriteInt32L(buf->Length());

	TUnicodeCompressor c;
	TMemoryUnicodeSource source(buf->Ptr());
	TInt bytes(0);
	TInt words(0);
	c.CompressL(stream, source, KMaxTInt, buf->Length(), &bytes, &words);

	stream.WriteInt8L(0);

	stream.CommitL();
	cb->StreamDictionary().AssignL(uid, stid);
	cb->CommitL();

	stream.Close();
	CleanupStack::PopAndDestroy();
	CleanupStack::PopAndDestroy(cb);
	
	CleanupStack::PopAndDestroy(buf);
}

// Window implementation

void Window_PreInit(void) {
	TInt keyboardType = -1;
	TUid categoryUid = { 0x101F876E }; //  KCRUidAvkon
	RProperty::Get(categoryUid, 0x0000000B /* KAknKeyBoardLayout */, keyboardType);
	
	switch (keyboardType) {
	case 0: // ENoKeyboard
		break;
	case 1: // EKeyboardWith12Key,
	case 5: // EHalfQWERTY
		NormDevice.defaultBinds = symbian_binds_12;
		break;
	case 2: // EQWERTY4x12Layout
	case 3: // EQWERTY4x10Layout
	case 4: // EQWERTY3x11Layout
	case 6: // ECustomQWERTY
		NormDevice.defaultBinds = symbian_binds_qwerty;
		break;
	default: // unknown or platform is older than s60v3.2
		if (HAL::Get(HAL::EKeyboard, keyboardType) == KErrNone) {
			if (!(keyboardType & EKeyboard_Full)) {
				NormDevice.defaultBinds = symbian_binds_12;
			} else {
				NormDevice.defaultBinds = symbian_binds_qwerty;
			}
		}
		break;
	}
}

void Window_Init(void) {
	Events_Init();
#ifdef CC_BUILD_TOUCH
	bool touch = AknLayoutUtils::PenEnabled();

	Input_SetTouchMode(touch);
	Gui_SetTouchUI(touch);
#endif
}

void Window_Free(void) {
}

void Window_Create2D(int width, int height) {
	Window_Main.Is3D = false;
	container->SetExtentToWholeScreen();
}

void Window_Create3D(int width, int height) {
	Window_Main.Is3D = true;
	container->SetExtentToWholeScreen();
}

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) { }

void Clipboard_GetText(cc_string* value) {
	TRAP_IGNORE(GetClipboardL(value));
}

void Clipboard_SetText(const cc_string* value) {
	TRAP_IGNORE(SetClipboardL(value));
}

int Window_GetWindowState(void) {
	return WINDOW_STATE_FULLSCREEN;
}

cc_result Window_EnterFullscreen(void) {
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	return 0;
}

int Window_IsObscured(void) {
	return WindowInfo.Inactive;
}

void Window_Show(void) {
}

void Window_SetSize(int width, int height) {
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(float delta) {
	CCEvent event;
	while (Events_Pull(&event)) {
		switch (event.type) {
		case CC_KEY_DOWN:
			Input_SetPressed(event.i1);
			break; 
		case CC_KEY_UP:
			Input_SetReleased(event.i1);
			break;
		case CC_KEY_INPUT:
			Event_RaiseInt(&InputEvents.Press, event.i1);
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

void Window_EnableRawMouse(void) {
	Input.RawMode = true;
}

void Window_UpdateRawMouse(void) { }

void Window_DisableRawMouse(void) {
	Input.RawMode = false;
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }

void ShowDialogCore(const char* title, const char* msg) {
	TRAP_IGNORE(ShowDialogL(title, msg));
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
#ifdef CC_BUILD_TOUCH
	VirtualKeyboard_Open(args);
#endif
}

void OnscreenKeyboard_SetText(const cc_string* text) {
#ifdef CC_BUILD_TOUCH
	VirtualKeyboard_SetText(text);
#endif
}

void OnscreenKeyboard_Close(void) {
#ifdef CC_BUILD_TOUCH
	VirtualKeyboard_Close();
#endif
}

void Window_LockLandscapeOrientation(cc_bool lock) {
	container->iAppUi->SetOrientationL(lock ? CAknAppUiBase::EAppUiOrientationLandscape : CAknAppUiBase::EAppUiOrientationAutomatic);
	container->SetExtentToWholeScreen();
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
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	container->DrawFramebuffer(r, bmp);
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
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
	TRAP(err, err = OpenBrowserL(args));
	return (cc_result) err;
}
