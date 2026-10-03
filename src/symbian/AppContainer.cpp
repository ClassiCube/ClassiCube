#include "Window_Symbian.h"

extern "C" {
#include "main_impl.h"
}

CCContainer* container;

TInt CCContainer::LoopCallBack(TAny* aPtr) {
	if (crashed) {
		return EFalse;
	}
	CCContainer* container = (CCContainer*) aPtr;
	bool run = false;
	for (;;) {
		if (!WindowInfo.Exists) {
			Window_RequestClose();
			container->iAppUi->Exit();
			return EFalse;
		}
		
		if (run) {
			run = false;
			container->gameRunning = true;
			ProcessProgramArgs(0, 0);
			Game_Setup();
			container->RestartTimerL(100);
		}
		
		if (!container->gameRunning) {
			if (Launcher_Tick()) break;
			Launcher_Finish();
			run = true;
			continue;
		}
		
		if (!Game_Running) {
			container->gameRunning = false;
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

void CCContainer::ConstructL(const TRect& aRect, CCAppUi* aAppUi) {
	iAppUi = aAppUi;
	
	CAknWsEventMonitor* monitor = iAppUi->EventMonitor();
	monitor->AddObserverL(this);
#ifdef __WINSCW__
	monitor->Enable(ETrue);
	useAknWsEventMonitor = true;
#else
	static const cc_string avkon = String_FromConst("avkon.dll");
	void* lib = DynamicLib_Load2(&avkon);
	if (lib != NULL) {
		void *func = DynamicLib_Get2(lib, "4345"); // _ZN18CAknWsEventMonitor6EnableEi
		if (func != NULL) {
			((void (*)(CAknWsEventMonitor*, TBool)) func)(monitor, ETrue);
			useAknWsEventMonitor = true;
		}
		// TODO dlclose ?
	}
#endif
	
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
	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_VIRTUAL;

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
	const TInt KEikDynamicLayoutVariantSwitch = 0x101F8121;
	switch (aType) {
	case KEikDynamicLayoutVariantSwitch:
		SetExtentToWholeScreen();
		// keyboard type may have changed, reset default binds
		Window_PreInit();
		InputBind_Load(&NormDevice);
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
	if (event.type) PushEvent(&event);
#endif
	CCoeControl::HandlePointerEventL(aPointerEvent);
}

const cc_uint8 key_map[] = {
	EStdKeyBackspace, CCKEY_BACKSPACE,
	EStdKeyTab, CCKEY_TAB,
	EStdKeyEnter, CCKEY_ENTER,
	EStdKeyEscape, CCKEY_ESCAPE,
	EStdKeySpace, CCKEY_SPACE,
	EStdKeyPrintScreen, CCKEY_PRINTSCREEN,
	EStdKeyPause, CCKEY_PAUSE,
	EStdKeyHome, CCKEY_HOME,
	EStdKeyEnd, CCKEY_END,
	EStdKeyPageUp, CCKEY_PAGEUP,
	EStdKeyPageDown, CCKEY_PAGEDOWN,
	EStdKeyInsert, CCKEY_INSERT,
	EStdKeyDelete, CCKEY_DELETE,
	EStdKeyLeftShift, CCKEY_LSHIFT,
	EStdKeyRightShift, CCKEY_RSHIFT,
	EStdKeyLeftAlt, CCKEY_LALT,
	EStdKeyRightAlt, CCKEY_RALT,
	EStdKeyLeftCtrl, CCKEY_LCTRL,
	EStdKeyRightCtrl, CCKEY_RCTRL,
	EStdKeyLeftFunc, CCKEY_LWIN,
	EStdKeyRightFunc, CCKEY_RWIN,
	EStdKeyNumLock, CCKEY_NUMLOCK,
	EStdKeyScrollLock, CCKEY_SCROLLLOCK,

	0x30, CCKEY_0,
	0x31, CCKEY_1,
	0x32, CCKEY_2,
	0x33, CCKEY_3,
	0x34, CCKEY_4,
	0x35, CCKEY_5,
	0x36, CCKEY_6,
	0x37, CCKEY_7,
	0x38, CCKEY_8,
	0x39, CCKEY_9,

	EStdKeyComma, CCKEY_COMMA,
	EStdKeyFullStop, CCKEY_PERIOD,
	EStdKeyForwardSlash, CCKEY_SLASH,
	EStdKeyBackSlash, CCKEY_BACKSLASH,
	EStdKeySemiColon, CCKEY_SEMICOLON,
	EStdKeySingleQuote, CCKEY_QUOTE,
	EStdKeyHash, '#',
	EStdKeySquareBracketLeft, CCKEY_LBRACKET,
	EStdKeySquareBracketRight, CCKEY_RBRACKET,
	EStdKeyMinus, CCKEY_MINUS,
	EStdKeyEquals, CCKEY_EQUALS,

	EStdKeyNkpForwardSlash, CCKEY_KP_DIVIDE,
	EStdKeyNkpAsterisk, CCKEY_KP_MULTIPLY,
	EStdKeyNkpMinus, CCKEY_KP_MINUS,
	EStdKeyNkpPlus, CCKEY_KP_PLUS,
	EStdKeyNkpEnter, CCKEY_KP_ENTER,
	EStdKeyNkp1, CCKEY_KP1,
	EStdKeyNkp2, CCKEY_KP2,
	EStdKeyNkp3, CCKEY_KP3,
	EStdKeyNkp4, CCKEY_KP4,
	EStdKeyNkp5, CCKEY_KP5,
	EStdKeyNkp6, CCKEY_KP6,
	EStdKeyNkp7, CCKEY_KP7,
	EStdKeyNkp8, CCKEY_KP8,
	EStdKeyNkp9, CCKEY_KP9,
	EStdKeyNkp0, CCKEY_KP0,
	EStdKeyNkpFullStop, CCKEY_KP_DECIMAL,

	EStdKeyIncVolume, CCKEY_VOLUME_UP,
	EStdKeyDecVolume, CCKEY_VOLUME_DOWN,

	EStdKeyDevice0, CCKEY_F1,
	EStdKeyDevice1, CCKEY_ESCAPE,
	EStdKeyDevice3, CCKEY_ENTER,
};

static int MapScanCode(TInt aScanCode, TInt aModifiers) {
	if (aScanCode == EStdKeyLeftArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_UP;
		if (aModifiers & EModifierRotateBy180) return CCKEY_RIGHT;
		if (aModifiers & EModifierRotateBy270) return CCKEY_DOWN;
		return CCKEY_LEFT;
	}
	if (aScanCode == EStdKeyRightArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_DOWN;
		if (aModifiers & EModifierRotateBy180) return CCKEY_LEFT;
		if (aModifiers & EModifierRotateBy270) return CCKEY_UP;
		return CCKEY_RIGHT;
	}
	if (aScanCode == EStdKeyUpArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_RIGHT;
		if (aModifiers & EModifierRotateBy180) return CCKEY_DOWN;
		if (aModifiers & EModifierRotateBy270) return CCKEY_LEFT;
		return CCKEY_UP;
	}
	if (aScanCode == EStdKeyDownArrow) {
		if (aModifiers & EModifierRotateBy90) return CCKEY_LEFT;
		if (aModifiers & EModifierRotateBy180) return CCKEY_UP;
		if (aModifiers & EModifierRotateBy270) return CCKEY_RIGHT;
		return CCKEY_DOWN;
	}
	
	for (size_t i = 0; i < sizeof(key_map); i += 2) {
		if (key_map[i] == aScanCode) {
			return key_map[i + 1];
		}
	}

	return aScanCode < INPUT_COUNT ? aScanCode : INPUT_NONE;
}

void CCContainer::HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination) {
	TInt type = aEvent.Type();
	
	if (type == EEventKey || type == EEventKeyDown || type == EEventKeyUp) {
		DoHandleKeyEventL(aEvent.Key(), type);
	}
}

TKeyResponse CCContainer::DoHandleKeyEventL(const TKeyEvent* aKeyEvent, TInt aType) {
	if (!events_mutex || WindowInfo.Inactive || iAppUi->IsDisplayingDialog()) return EKeyWasNotConsumed;
	CCEvent event = { 0 };
	switch (aType) {
	case EEventKey: {
		event.i1 = MapScanCode(aKeyEvent->iScanCode, aKeyEvent->iModifiers);
		int code = aKeyEvent->iCode;
		if (code != 0 && (code < ENonCharacterKeyBase || code > ENonCharacterKeyBase + ENonCharacterKeyCount)) {
			event.i1 = code;
			event.type = CC_KEY_INPUT;
			PushEvent(&event);
			return EKeyWasConsumed;
		}
		break;
	}
	case EEventKeyDown: {
		event.i1 = MapScanCode(aKeyEvent->iScanCode, aKeyEvent->iModifiers);
		if (event.i1 != INPUT_NONE) {
			event.type = CC_KEY_DOWN;
			PushEvent(&event);
		}
		return EKeyWasConsumed;
	}
	case EEventKeyUp: {
		event.i1 = MapScanCode(aKeyEvent->iScanCode, aKeyEvent->iModifiers);
		if (event.i1 != INPUT_NONE) {
			event.type = CC_KEY_UP;
			PushEvent(&event);
		}
		return EKeyWasConsumed;
	}
	default:
		return EKeyWasNotConsumed;
	}
	return EKeyWasNotConsumed;
}

void CCContainer::InitEvents() {
	events_mutex    = Mutex_Create("Symbian events");
	events_capacity = EVENTS_DEFAULT_MAX;
	events_list     = events_default;
}

void CCContainer::PushEvent(const CCEvent* event) {
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

cc_bool CCContainer::PullEvent(CCEvent* event) {
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
