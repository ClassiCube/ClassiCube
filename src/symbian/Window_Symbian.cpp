#include "Window_Symbian.h"
extern "C" {
#include "_WindowBase.h"
}

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
	for (size_t i = 0; i < length; i++) {
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
		bool qwerty = false;
		TRAP_IGNORE(qwerty = CFeatureDiscovery::IsFeatureSupportedL(KFeatureIdQwertyInput));
		NormDevice.defaultBinds = qwerty ? symbian_binds_qwerty : symbian_binds_12;
		break;
	}
}

void Window_Init(void) {
	container->InitEvents();
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
	while (container->PullEvent(&event)) {
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

static TBuf<512> inputBuffer;
static char kb_buffer[512];
static cc_string kb_str = String_FromArray(kb_buffer);

class CCTextQuery : public CAknTextQueryDialog {
public:
	CCTextQuery(TDes& aDataText) :
		CAknTextQueryDialog(aDataText, CAknQueryDialog::ENoTone) {}
	virtual ~CCTextQuery() {}
#if defined CC_BUILD_SYMBIAN_S60V3
public:
	TCoeInputCapabilities InputCapabilities() const {
		return CEikDialog::InputCapabilities();
	}
#endif

protected:
	TBool OkToExitL(TInt aButtonId) {
		bool res = CAknTextQueryDialog::OkToExitL(aButtonId);
		if (res) {
			if (aButtonId == EAknSoftkeyOk) {
				kb_str.length = 0;
				String_AppendUtf16(&kb_str, inputBuffer.Ptr(), inputBuffer.Length() * 2);
				Event_RaiseString(&InputEvents.TextChanged, &kb_str);
			}
		}
		return res;
	}
};

static void ShowQueryDialogL(struct OpenKeyboardArgs* args) {
	CCTextQuery* dlg = new (ELeave) CCTextQuery(inputBuffer);
	dlg->PrepareLC(R_TEXT_QUERY);
	if (args->placeholder && String_Length(args->placeholder) != 0) {
		TBuf<128> buf;
		ConvertToUnicode(buf, args->placeholder, String_Length(args->placeholder));
		dlg->SetPromptL(buf);
	}
	dlg->RunLD();
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) {
	if (CCoeEnv::Static()->AppUi()->IsDisplayingDialog()) {
		return;
	}
	
	inputBuffer.Zero();
	kb_str.length = 0;

	if (args->text) {
		ConvertToUnicode(inputBuffer, args->text);
	}

	TRAP_IGNORE(ShowQueryDialogL(args));
}

void OnscreenKeyboard_SetText(const cc_string* text) {
}

void OnscreenKeyboard_Close(void) {
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
