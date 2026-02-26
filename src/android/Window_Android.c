#include "../_WindowBase.h"
#include "../String_.h"
#include "../Funcs.h"
#include "../Bitmap.h"
#include "../Errors.h"
#include "../Graphics.h"
#include "../Gui.h"
#include "interop_android.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/keycodes.h>
#include <unistd.h>

static ANativeWindow* win_handle;
static cc_bool winCreated;

static jmethodID JAVA_openKeyboard, JAVA_setKeyboardText, JAVA_closeKeyboard;
static jmethodID JAVA_getWindowState, JAVA_enterFullscreen, JAVA_exitFullscreen;
static jmethodID JAVA_showAlert, JAVA_setRequestedOri;
static jmethodID JAVA_openFileDialog, JAVA_saveFileDialog;
static jmethodID JAVA_processedSurfaceDestroyed, JAVA_processEvents;
static jmethodID JAVA_getDpiX, JAVA_getDpiY, JAVA_setupForGame;
static jmethodID JAVA_getClipboardText, JAVA_setClipboardText;

static int32_t (*_ANativeWindow_lock)(ANativeWindow* window, ANativeWindow_Buffer* oBuffer, ARect* ioDirtyBounds);
static int32_t (*_ANativeWindow_unlockAndPost)(ANativeWindow* window);

static ANativeWindow* (*_ANativeWindow_fromSurface)(JNIEnv* env, jobject surface);
static void           (*_ANativeWindow_release)(ANativeWindow* window);

static void SetWindowBounds(int width, int height) {
	Window_Main.Width  = width;
	Window_Main.Height = height;
	Platform_Log2("SCREEN BOUNDS: %i,%i", &Window_Main.Width, &Window_Main.Height);
	Event_RaiseVoid(&WindowEvents.Resized);
}

// https://developer.android.com/ndk/reference/group/input
static int MapNativeKey(int code) {
	if (code >= AKEYCODE_0  && code <= AKEYCODE_9)   return (code - AKEYCODE_0)  + '0';
	if (code >= AKEYCODE_A  && code <= AKEYCODE_Z)   return (code - AKEYCODE_A)  + 'A';
	if (code >= AKEYCODE_F1 && code <= AKEYCODE_F12) return (code - AKEYCODE_F1) + CCKEY_F1;
	if (code >= AKEYCODE_NUMPAD_0 && code <= AKEYCODE_NUMPAD_9) return (code - AKEYCODE_NUMPAD_0) + CCKEY_KP0;

	switch (code) {
		/* TODO: AKEYCODE_STAR */
		/* TODO: AKEYCODE_POUND */
	case AKEYCODE_BACK:   return CCKEY_ESCAPE;
	case AKEYCODE_COMMA:  return CCKEY_COMMA;
	case AKEYCODE_PERIOD: return CCKEY_PERIOD;
	case AKEYCODE_ALT_LEFT:    return CCKEY_LALT;
	case AKEYCODE_ALT_RIGHT:   return CCKEY_RALT;
	case AKEYCODE_SHIFT_LEFT:  return CCKEY_LSHIFT;
	case AKEYCODE_SHIFT_RIGHT: return CCKEY_RSHIFT;
	case AKEYCODE_TAB:    return CCKEY_TAB;
	case AKEYCODE_SPACE:  return CCKEY_SPACE;
	case AKEYCODE_ENTER:  return CCKEY_ENTER;
	case AKEYCODE_DEL:    return CCKEY_BACKSPACE;
	case AKEYCODE_GRAVE:  return CCKEY_TILDE;
	case AKEYCODE_MINUS:  return CCKEY_MINUS;
	case AKEYCODE_EQUALS: return CCKEY_EQUALS;
	case AKEYCODE_LEFT_BRACKET:  return CCKEY_LBRACKET;
	case AKEYCODE_RIGHT_BRACKET: return CCKEY_RBRACKET;
	case AKEYCODE_BACKSLASH:  return CCKEY_BACKSLASH;
	case AKEYCODE_SEMICOLON:  return CCKEY_SEMICOLON;
	case AKEYCODE_APOSTROPHE: return CCKEY_QUOTE;
	case AKEYCODE_SLASH:      return CCKEY_SLASH;
		/* TODO: AKEYCODE_AT */
		/* TODO: AKEYCODE_PLUS */
		/* TODO: AKEYCODE_MENU */
	case AKEYCODE_PAGE_UP:     return CCKEY_PAGEUP;
	case AKEYCODE_PAGE_DOWN:   return CCKEY_PAGEDOWN;
	case AKEYCODE_ESCAPE:      return CCKEY_ESCAPE;
	case AKEYCODE_FORWARD_DEL: return CCKEY_DELETE;
	case AKEYCODE_CTRL_LEFT:   return CCKEY_LCTRL;
	case AKEYCODE_CTRL_RIGHT:  return CCKEY_RCTRL;
	case AKEYCODE_CAPS_LOCK:   return CCKEY_CAPSLOCK;
	case AKEYCODE_SCROLL_LOCK: return CCKEY_SCROLLLOCK;
	case AKEYCODE_META_LEFT:   return CCKEY_LWIN;
	case AKEYCODE_META_RIGHT:  return CCKEY_RWIN;
	case AKEYCODE_SYSRQ:    return CCKEY_PRINTSCREEN;
	case AKEYCODE_BREAK:    return CCKEY_PAUSE;
	case AKEYCODE_INSERT:   return CCKEY_INSERT;
	case AKEYCODE_NUM_LOCK: return CCKEY_NUMLOCK;
	case AKEYCODE_NUMPAD_DIVIDE:   return CCKEY_KP_DIVIDE;
	case AKEYCODE_NUMPAD_MULTIPLY: return CCKEY_KP_MULTIPLY;
	case AKEYCODE_NUMPAD_SUBTRACT: return CCKEY_KP_MINUS;
	case AKEYCODE_NUMPAD_ADD:      return CCKEY_KP_PLUS;
	case AKEYCODE_NUMPAD_DOT:      return CCKEY_KP_DECIMAL;
	case AKEYCODE_NUMPAD_ENTER:    return CCKEY_KP_ENTER;

	case AKEYCODE_DPAD_UP:    return CCPAD_UP;
	case AKEYCODE_DPAD_DOWN:  return CCPAD_DOWN;
	case AKEYCODE_DPAD_LEFT:  return CCPAD_LEFT;
	case AKEYCODE_DPAD_RIGHT: return CCPAD_RIGHT;

	case AKEYCODE_BUTTON_A: return CCPAD_1;
	case AKEYCODE_BUTTON_B: return CCPAD_2;
	case AKEYCODE_BUTTON_X: return CCPAD_3;
	case AKEYCODE_BUTTON_Y: return CCPAD_4;

	case AKEYCODE_BUTTON_L1: return CCPAD_L;
	case AKEYCODE_BUTTON_R1: return CCPAD_R;
	case AKEYCODE_BUTTON_L2: return CCPAD_ZL;
	case AKEYCODE_BUTTON_R2: return CCPAD_ZR;

	case AKEYCODE_BUTTON_START:  return CCPAD_START;
	case AKEYCODE_BUTTON_SELECT: return CCPAD_SELECT;
	case AKEYCODE_BUTTON_THUMBL: return CCPAD_LSTICK;
	case AKEYCODE_BUTTON_THUMBR: return CCPAD_RSTICK;
	}
	return INPUT_NONE;
}

static void JNICALL java_processKeyDown(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - DOWN %i,%i", &code, &key);

	if (Input_IsPadButton(key)) {
		int port = Gamepad_Connect(0xAD01D, PadBind_Defaults);
		Gamepad_SetButton(port, key, true);
	} else {
		if (key) Input_SetPressed(key);
	}
}

static void JNICALL java_processKeyUp(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - UP %i,%i", &code, &key);

	if (Input_IsPadButton(key)) {
		int port = Gamepad_Connect(0xAD01D, PadBind_Defaults);
		Gamepad_SetButton(port, key, false);
	} else {
		if (key) Input_SetReleased(key);
	}
}

static void JNICALL java_processKeyChar(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - PRESS %i,%i", &code, &key);
	Event_RaiseInt(&InputEvents.Press, code);
}

static void JNICALL java_processKeyText(JNIEnv* env, jobject o, jstring str) {
	char buffer[NATIVE_STR_LEN]; cc_string text;
	String_InitArray(text, buffer);
	
	Java_DecodeString(env, str, &text);
	Platform_Log1("KEY - TEXT %s", &text);
	Event_RaiseString(&InputEvents.TextChanged, &text);
}

static void JNICALL java_processPointerDown(JNIEnv* env, jobject o, jint id, jint x, jint y, jint isMouse) {
	Platform_Log4("POINTER %i (%i) - DOWN %i,%i", &id, &isMouse, &x, &y);
	Input_AddTouch(id, x, y);
}

static void JNICALL java_processPointerUp(JNIEnv* env, jobject o, jint id, jint x, jint y, jint isMouse) {
	Platform_Log4("POINTER %i (%i) - UP   %i,%i", &id, &isMouse, &x, &y);
	Input_RemoveTouch(id, x, y);
}

static void JNICALL java_processPointerMove(JNIEnv* env, jobject o, jint id, jint x, jint y, jint isMouse) {
	Platform_Log4("POINTER %i (%i) - MOVE %i,%i", &id, &isMouse, &x, &y);
	Input_UpdateTouch(id, x, y);
}

static void JNICALL java_processJoystickL(JNIEnv* env, jobject o, jint x, jint y) {
	int port = Gamepad_Connect(0xAD01D, PadBind_Defaults);
	Gamepad_SetAxis(port, PAD_AXIS_LEFT,  x / 4096.0f, y / 4096.0f, 1.0f / 60);
}

static void JNICALL java_processJoystickR(JNIEnv* env, jobject o, jint x, jint y) {
	int port = Gamepad_Connect(0xAD01D, PadBind_Defaults);
	Gamepad_SetAxis(port, PAD_AXIS_RIGHT, x / 4096.0f, y / 4096.0f, 1.0f / 60);
}

static void JNICALL java_processSurfaceCreated(JNIEnv* env, jobject o, jobject surface, jint w, jint h) {
	Platform_LogConst("WIN - CREATED");
	win_handle        = _ANativeWindow_fromSurface(env, surface);
	winCreated        = true;

	Window_Main.Handle.ptr = win_handle;
	SetWindowBounds(w, h);
	/* TODO: Restore context */
	Event_RaiseVoid(&WindowEvents.Created);
}

static void JNICALL java_processSurfaceDestroyed(JNIEnv* env, jobject o) {
	Platform_LogConst("WIN - DESTROYED");
	if (win_handle) _ANativeWindow_release(win_handle);

	win_handle             = NULL;
	Window_Main.Handle.ptr = NULL;

	/* eglSwapBuffers might return EGL_BAD_SURFACE, EGL_BAD_ALLOC, or some other error */
	/* Instead the context is lost here in a consistent manner */
	if (Gfx.Created) Gfx_LoseContext("surface lost");
	Java_ICall_Void(env, JAVA_processedSurfaceDestroyed, NULL);
}

static void JNICALL java_processSurfaceResized(JNIEnv* env, jobject o, jint w, jint h) {
	Platform_LogConst("WIN - RESIZED");
	SetWindowBounds(w, h);
}

static void JNICALL java_processSurfaceRedrawNeeded(JNIEnv* env, jobject o) {
	Platform_LogConst("WIN - REDRAW");
	Event_RaiseVoid(&WindowEvents.RedrawNeeded);
}

static void JNICALL java_onStart(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON START");
}

static void JNICALL java_onStop(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON STOP");
}

static void JNICALL java_onResume(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON RESUME");
	/* TODO: Resume rendering */
}

static void JNICALL java_onPause(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON PAUSE");
	/* TODO: Disable rendering */
}

static void JNICALL java_onDestroy(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - ON DESTROY");

	if (Window_Main.Exists) Window_RequestClose();
	/* TODO: signal to java code we're done */
	/* Java_ICall_Void(env, JAVA_processedDestroyed", NULL); */
}

static void JNICALL java_onGotFocus(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - GOT FOCUS");
	Window_Main.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void JNICALL java_onLostFocus(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - LOST FOCUS");
	Window_Main.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
	/* TODO: Disable rendering? */
}

static void JNICALL java_onLowMemory(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - LOW MEM");
	/* TODO: Low memory */
}

static void JNICALL java_processOFDResult(JNIEnv* env, jobject o, jstring str);

static void RemakeWindowSurface(void) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	winCreated = false;

	/* Force window to be destroyed and re-created */
	/* (see comments in setupForGame for why this has to be done) */
	Java_ICall_Void(env, JAVA_setupForGame, NULL);
	Platform_LogConst("Entering wait for window exist loop..");

	/* Loop until window gets created by main UI thread */
	/* (i.e. until processSurfaceCreated is received) */
	while (!winCreated) {
		Window_ProcessEvents(0.01f);
		Thread_Sleep(10);
	}

	Platform_LogConst("OK window created..");
}

static void DoCreateWindow(void) {
	Window_Main.Exists   = true;
	Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
	Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
	
	Window_Main.SoftKeyboardInstant = true;
	RemakeWindowSurface();
	/* always start as fullscreen */
	Window_EnterFullscreen();
}
void Window_Create2D(int width, int height) { DoCreateWindow(); }
void Window_Create3D(int width, int height) { DoCreateWindow(); }

void Window_Destroy(void) { }

void Window_SetTitle(const cc_string* title) {
	/* TODO: Implement this somehow */
	/* Maybe https://stackoverflow.com/questions/2198410/how-to-change-title-of-activity-in-android */
}

void Clipboard_GetText(cc_string* value) {
	JNIEnv* env;
	jobject obj;
	Java_GetCurrentEnv(env);

	obj = Java_ICall_Obj(env, JAVA_getClipboardText, NULL);
	if (obj) Java_DecodeString(env, obj, value);
	
	Java_DeleteLocalRef(env, obj);
}

void Clipboard_SetText(const cc_string* value) {
	JNIEnv* env;
	jvalue args[1];
	Java_GetCurrentEnv(env);

	args[0].l = Java_AllocString(env, value);
	Java_ICall_Void(env, JAVA_setClipboardText, args);
	Java_DeleteLocalRef(env, args[0].l);
}

int Window_GetWindowState(void) { 
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	return Java_ICall_Int(env, JAVA_getWindowState, NULL);
}

cc_result Window_EnterFullscreen(void) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	Java_ICall_Void(env, JAVA_enterFullscreen, NULL);
	return 0; 
}

cc_result Window_ExitFullscreen(void) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	Java_ICall_Void(env, JAVA_exitFullscreen, NULL);
	return 0; 
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) { } /* Window already visible */
void Window_SetSize(int width, int height) { }

void Window_RequestClose(void) {
	Window_Main.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
	/* TODO: Do we need to call finish here */
	/* ANativeActivity_finish(app->activity); */
}

void Window_ProcessEvents(float delta) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	/* TODO: Cache the java env */
	Java_ICall_Void(env, JAVA_processEvents, NULL);
}

void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }

/* No actual mouse cursor */
static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
static void Cursor_DoSetVisible(cc_bool visible) { }

static void ShowDialogCore(const char* title, const char* msg) {
	JNIEnv* env;
	jvalue args[2];
	Java_GetCurrentEnv(env);

	Platform_LogConst(title);
	Platform_LogConst(msg);
	/* in case surface destroyed message has arrived */
	Window_ProcessEvents(0.0);

	args[0].l = Java_AllocConst(env, title);
	args[1].l = Java_AllocConst(env, msg);
	Java_ICall_Void(env, JAVA_showAlert, args);
	Java_DeleteLocalRef(env, args[0].l);
	Java_DeleteLocalRef(env, args[1].l);
}

static FileDialogCallback ofd_callback;
static int ofd_action;
static void JNICALL java_processOFDResult(JNIEnv* env, jobject o, jstring str) {
    const char* raw;
    char buffer[NATIVE_STR_LEN]; cc_string path;
	String_InitArray(path, buffer);
	
	// TODO should be raw
	Java_DecodeString(env, str, &path);
	ofd_callback(&path);

	if (ofd_action == OFD_UPLOAD_DELETE) {
	    // TODO better way of doing this? msybe move to java side?
	    raw = (*env)->GetStringUTFChars(env, str, NULL);
	    unlink(raw);
	    (*env)->ReleaseStringUTFChars(env, str, raw);
	}
    ofd_callback = NULL;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* open_args) {
    JNIEnv* env;
    jvalue args[1];
    Java_GetCurrentEnv(env);

    ofd_callback = open_args->Callback;
    ofd_action   = open_args->uploadAction;

    // TODO use filters
    args[0].l = Java_AllocConst(env, open_args->uploadFolder);
    int OK = Java_ICall_Int(env, JAVA_openFileDialog, args);
    Java_DeleteLocalRef(env, args[0].l);
	
    // TODO: Better error handling
    return OK ? 0 : ERR_INVALID_ARGUMENT;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* save_args) {
    JNIEnv* env;
    jvalue args[2];
    Java_GetCurrentEnv(env);
    if (!save_args->defaultName.length) return SFD_ERR_NEED_DEFAULT_NAME;

    // save the item to a temp file, which is then (usually) later deleted by intent callback
    Directory_Create2(FILEPATH_RAW("Exported"));

    cc_string path; char pathBuffer[FILENAME_SIZE];
    String_InitArray(path, pathBuffer);
    String_Format2(&path, "Exported/%s%c", &save_args->defaultName, save_args->filters[0]);
    save_args->Callback(&path);
    // TODO kinda ugly, maybe a better way?
    cc_string file = String_UNSAFE_SubstringAt(&path, String_IndexOf(&path, '/') + 1);

    args[0].l = Java_AllocString(env, &path);
    args[1].l = Java_AllocString(env, &file);
    int OK = Java_ICall_Int(env, JAVA_saveFileDialog, args);
    Java_DeleteLocalRef(env, args[0].l);
    Java_DeleteLocalRef(env, args[1].l);
	
    // TODO: Better error handling
    return OK ? 0 : ERR_INVALID_ARGUMENT;
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	ANativeWindow_Buffer buffer;
	cc_uint32* src;
	cc_uint32* dst;
	ARect b;
	int y, res, size;

	/* window not created yet */
	if (!win_handle) return;
	b.left = r.x; b.right  = r.x + r.width;
	b.top  = r.y; b.bottom = r.y + r.height;

	/* Platform_Log4("DIRTY: %i,%i - %i,%i", &b.left, &b.top, &b.right, &b.bottom); */
	res  = _ANativeWindow_lock(win_handle, &buffer, &b);
	if (res) Process_Abort2(res, "Locking window pixels");
	/* Platform_Log4("ADJUS: %i,%i - %i,%i", &b.left, &b.top, &b.right, &b.bottom); */

	/* In some rare cases, the returned locked region will be entire area of the surface */
	/* This can cause a crash if the surface has been resized (i.e. device rotated), */
	/* but the framebuffer has not been resized yet. So always constrain bounds. */
	b.left = min(b.left, bmp->width);  b.right  = min(b.right,  bmp->width);
	b.top  = min(b.top,  bmp->height); b.bottom = min(b.bottom, bmp->height);

	src  = (cc_uint32*)bmp->scan0  + b.left;
	dst  = (cc_uint32*)buffer.bits + b.left;
	size = (b.right - b.left) * 4;

	for (y = b.top; y < b.bottom; y++) 
	{
		Mem_Copy(dst + y * buffer.stride, src + y * bmp->width, size);
	}
	res = _ANativeWindow_unlockAndPost(win_handle);
	if (res) Process_Abort2(res, "Unlocking window pixels");
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* kArgs) {
	JNIEnv* env;
	jvalue args[2];
	Java_GetCurrentEnv(env);
	DisplayInfo.ShowingSoftKeyboard = true;

	args[0].l = Java_AllocString(env, kArgs->text);
	args[1].i = kArgs->type;
	Java_ICall_Void(env, JAVA_openKeyboard, args);
	Java_DeleteLocalRef(env, args[0].l);
}

void OnscreenKeyboard_SetText(const cc_string* text) {
	JNIEnv* env;
	jvalue args[1];
	Java_GetCurrentEnv(env);

	args[0].l = Java_AllocString(env, text);
	Java_ICall_Void(env, JAVA_setKeyboardText, args);
	Java_DeleteLocalRef(env, args[0].l);
}

void OnscreenKeyboard_Close(void) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	DisplayInfo.ShowingSoftKeyboard = false;

	Java_ICall_Void(env, JAVA_closeKeyboard, NULL);
}

void Window_LockLandscapeOrientation(cc_bool lock) {
	JNIEnv* env;
	jvalue args[1];
	Java_GetCurrentEnv(env);

	/* SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 0x00000006 */
	/* SCREEN_ORIENTATION_UNSPECIFIED = 0xffffffff */
	args[0].i = lock ? 0x00000006 : 0xffffffff;
	Java_ICall_Void(env, JAVA_setRequestedOri, args);
}

void Window_EnableRawMouse(void)  { DefaultEnableRawMouse(); }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { DefaultDisableRawMouse(); }


/*########################################################################################################################*
*------------------------------------------------------libandroid---------------------------------------------------------*
*#########################################################################################################################*/
static const cc_string droidLib = String_FromConst("libandroid.so");

static cc_bool LoadDroidFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_ReqSym(ANativeWindow_lock),
		DynamicLib_ReqSym(ANativeWindow_unlockAndPost),
		DynamicLib_ReqSym(ANativeWindow_fromSurface),
		DynamicLib_ReqSym(ANativeWindow_release),
	};
	void* lib;
	
	return DynamicLib_LoadAll(&droidLib, funcs, Array_Elems(funcs), &lib);
}

static ANativeWindow* fallback_fromSurface(JNIEnv* env, jobject surface) {
	return (void*)1;
}

static void fallback_release(ANativeWindow* window) {
}

static jobject canvas_arr;
static jint*   canvas_ptr;
static ARect   canvas_rect;

static int32_t fallback_lock(ANativeWindow* window, ANativeWindow_Buffer* oBuffer, ARect* ioDirtyBounds) {
	JNIEnv* env;
	jvalue args[4];
	Java_GetCurrentEnv(env);

	args[0].i = ioDirtyBounds->left;
	args[1].i = ioDirtyBounds->top;
	args[2].i = ioDirtyBounds->right;
	args[3].i = ioDirtyBounds->bottom;
	
	jmethodID method = Java_GetIMethod(env, "legacy_lockCanvas", "(IIII)[I");
	canvas_arr = Java_ICall_Obj(env, method, args);
    canvas_ptr = (*env)->GetIntArrayElements(env, canvas_arr, NULL);
	
	oBuffer->stride = ioDirtyBounds->right - ioDirtyBounds->left;
	// Counteract pointer adjustment done in Window_DrawFramebuffer
	oBuffer->bits   = canvas_ptr - (ioDirtyBounds->top * oBuffer->stride) - ioDirtyBounds->left;
	canvas_rect     = *ioDirtyBounds;
	
	if (!canvas_ptr) Process_Abort("Out of memory for canvas");
	return 0;
}

static int32_t fallback_unlockAndPost(ANativeWindow* window) {
	JNIEnv* env;
	jvalue args[5];
	Java_GetCurrentEnv(env);
    (*env)->ReleaseIntArrayElements(env, canvas_arr, canvas_ptr, 0);

	args[0].i = canvas_rect.left;
	args[1].i = canvas_rect.top;
	args[2].i = canvas_rect.right;
	args[3].i = canvas_rect.bottom;
	args[4].l = canvas_arr;
	
	jmethodID method = Java_GetIMethod(env, "legacy_unlockCanvas", "(IIII[I)V");
	Java_ICall_Void(env, method, args);
	
	(*env)->DeleteLocalRef(env, canvas_arr);
	canvas_arr = NULL;
	return 0;
}

static void UseFallbackDroidFuncs(void) {
	_ANativeWindow_fromSurface   = fallback_fromSurface;
	_ANativeWindow_release       = fallback_release;
	_ANativeWindow_lock          = fallback_lock;
	_ANativeWindow_unlockAndPost = fallback_unlockAndPost;
}


/*########################################################################################################################*
*----------------------------------------------------Initialisation-------------------------------------------------------*
*#########################################################################################################################*/
static const JNINativeMethod methods[] = {
	{ "processKeyDown",   "(I)V", java_processKeyDown },
	{ "processKeyUp",     "(I)V", java_processKeyUp },
	{ "processKeyChar",   "(I)V", java_processKeyChar },
	{ "processKeyText",   "(Ljava/lang/String;)V", java_processKeyText },

	{ "processPointerDown", "(IIII)V", java_processPointerDown },
	{ "processPointerUp",   "(IIII)V", java_processPointerUp },
	{ "processPointerMove", "(IIII)V", java_processPointerMove },

	{ "processJoystickL", "(II)V", java_processJoystickL },
	{ "processJoystickR", "(II)V", java_processJoystickR },

	{ "processSurfaceCreated",      "(Landroid/view/Surface;II)V", java_processSurfaceCreated },
	{ "processSurfaceDestroyed",    "()V",                         java_processSurfaceDestroyed },
	{ "processSurfaceResized",      "(II)V",                       java_processSurfaceResized },
	{ "processSurfaceRedrawNeeded", "()V",                         java_processSurfaceRedrawNeeded },

	{ "processOnStart",   "()V", java_onStart },
	{ "processOnStop",    "()V", java_onStop },
	{ "processOnResume",  "()V", java_onResume },
	{ "processOnPause",   "()V", java_onPause },
	{ "processOnDestroy", "()V", java_onDestroy },

	{ "processOnGotFocus",  "()V", java_onGotFocus },
	{ "processOnLostFocus", "()V", java_onLostFocus },
	{ "processOnLowMemory", "()V", java_onLowMemory },

	{ "processOFDResult",   "(Ljava/lang/String;)V", java_processOFDResult },
};

static void CacheJavaMethodRefs(JNIEnv* env) {
	JAVA_openKeyboard    = Java_GetIMethod(env, "openKeyboard",    "(Ljava/lang/String;I)V");
	JAVA_setKeyboardText = Java_GetIMethod(env, "setKeyboardText", "(Ljava/lang/String;)V");
	JAVA_closeKeyboard   = Java_GetIMethod(env, "closeKeyboard",   "()V");

	JAVA_getWindowState  = Java_GetIMethod(env, "getWindowState",  "()I");
	JAVA_enterFullscreen = Java_GetIMethod(env, "enterFullscreen", "()V");
	JAVA_exitFullscreen  = Java_GetIMethod(env, "exitFullscreen",  "()V");

	JAVA_getDpiX      = Java_GetIMethod(env, "getDpiX", "()F");
	JAVA_getDpiY      = Java_GetIMethod(env, "getDpiY", "()F");
	JAVA_setupForGame = Java_GetIMethod(env, "setupForGame", "()V");

	JAVA_processedSurfaceDestroyed = Java_GetIMethod(env, "processedSurfaceDestroyed", "()V");
	JAVA_processEvents             = Java_GetIMethod(env, "processEvents",             "()V");

	JAVA_showAlert       = Java_GetIMethod(env, "showAlert", "(Ljava/lang/String;Ljava/lang/String;)V");
	JAVA_setRequestedOri = Java_GetIMethod(env, "setRequestedOrientation", "(I)V");
	JAVA_openFileDialog  = Java_GetIMethod(env, "openFileDialog", "(Ljava/lang/String;)I");
	JAVA_saveFileDialog  = Java_GetIMethod(env, "saveFileDialog", "(Ljava/lang/String;Ljava/lang/String;)I");
	
	JAVA_getClipboardText = Java_GetIMethod(env, "getClipboardText", "()Ljava/lang/String;");
	JAVA_setClipboardText = Java_GetIMethod(env, "setClipboardText", "(Ljava/lang/String;)V");
}

void Window_PreInit(void) {
	JNIEnv* env;
	Java_GetCurrentEnv(env);
	Java_RegisterNatives(env, methods);
	CacheJavaMethodRefs(env);
	
	DisplayInfo.CursorVisible = true;
	if (!LoadDroidFuncs()) UseFallbackDroidFuncs();
}

void Window_Init(void) {
	JNIEnv* env;
	/* TODO: ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN, 0); */
	Java_GetCurrentEnv(env);

	Window_Main.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	Gui_SetTouchUI(true);
	Input.Sources = INPUT_SOURCE_NORMAL;

	DisplayInfo.Depth  = 32;
	DisplayInfo.ScaleX = Java_ICall_Float(env, JAVA_getDpiX, NULL);
	DisplayInfo.ScaleY = Java_ICall_Float(env, JAVA_getDpiY, NULL);
}

void Window_Free(void) { }
