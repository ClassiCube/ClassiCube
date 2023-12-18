#include "Core.h"
#if defined CC_BUILD_ANDROID && !defined CC_BUILD_SDL
#include "_WindowBase.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "Graphics.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/keycodes.h>

static ANativeWindow* win_handle;
static cc_bool winCreated;
static jmethodID JAVA_openKeyboard, JAVA_setKeyboardText, JAVA_closeKeyboard;
static jmethodID JAVA_getWindowState, JAVA_enterFullscreen, JAVA_exitFullscreen;
static jmethodID JAVA_showAlert, JAVA_setRequestedOrientation;
static jmethodID JAVA_openFileDialog, JAVA_saveFileDialog;
static jmethodID JAVA_processedSurfaceDestroyed, JAVA_processEvents;
static jmethodID JAVA_getDpiX, JAVA_getDpiY, JAVA_setupForGame;

static void RefreshWindowBounds(void) {
	WindowInfo.Width  = ANativeWindow_getWidth(win_handle);
	WindowInfo.Height = ANativeWindow_getHeight(win_handle);
	Platform_Log2("SCREEN BOUNDS: %i,%i", &WindowInfo.Width, &WindowInfo.Height);
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

	case AKEYCODE_BUTTON_A: return CCPAD_A;
	case AKEYCODE_BUTTON_B: return CCPAD_B;
	case AKEYCODE_BUTTON_X: return CCPAD_X;
	case AKEYCODE_BUTTON_Y: return CCPAD_Y;

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
	if (key) Input_SetPressed(key);

	if (Input_IsPadButton(key)) Input.Sources |= INPUT_SOURCE_GAMEPAD;
}

static void JNICALL java_processKeyUp(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - UP %i,%i", &code, &key);
	if (key) Input_SetReleased(key);
}

static void JNICALL java_processKeyChar(JNIEnv* env, jobject o, jint code) {
	int key = MapNativeKey(code);
	Platform_Log2("KEY - PRESS %i,%i", &code, &key);
	Event_RaiseInt(&InputEvents.Press, code);
}

static void JNICALL java_processKeyText(JNIEnv* env, jobject o, jstring str) {
	char buffer[NATIVE_STR_LEN];
	cc_string text = JavaGetString(env, str, buffer);
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

static void JNICALL java_processSurfaceCreated(JNIEnv* env, jobject o, jobject surface) {
	Platform_LogConst("WIN - CREATED");
	win_handle        = ANativeWindow_fromSurface(env, surface);
	winCreated        = true;
	WindowInfo.Handle = win_handle;
	RefreshWindowBounds();
	/* TODO: Restore context */
	Event_RaiseVoid(&WindowEvents.Created);
}

static void JNICALL java_processSurfaceDestroyed(JNIEnv* env, jobject o) {
	Platform_LogConst("WIN - DESTROYED");
	if (win_handle) ANativeWindow_release(win_handle);

	win_handle        = NULL;
	WindowInfo.Handle = NULL;
	/* eglSwapBuffers might return EGL_BAD_SURFACE, EGL_BAD_ALLOC, or some other error */
	/* Instead the context is lost here in a consistent manner */
	if (Gfx.Created) Gfx_LoseContext("surface lost");
	JavaICall_Void(env, JAVA_processedSurfaceDestroyed, NULL);
}

static void JNICALL java_processSurfaceResized(JNIEnv* env, jobject o, jobject surface) {
	Platform_LogConst("WIN - RESIZED");
	RefreshWindowBounds();
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

	if (WindowInfo.Exists) Window_Close();
	/* TODO: signal to java code we're done */
	/* JavaICall_Void(env, JAVA_processedDestroyed", NULL); */
}

static void JNICALL java_onGotFocus(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - GOT FOCUS");
	WindowInfo.Focused = true;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
}

static void JNICALL java_onLostFocus(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - LOST FOCUS");
	WindowInfo.Focused = false;
	Event_RaiseVoid(&WindowEvents.FocusChanged);
	/* TODO: Disable rendering? */
}

static void JNICALL java_onLowMemory(JNIEnv* env, jobject o) {
	Platform_LogConst("APP - LOW MEM");
	/* TODO: Low memory */
}

static void JNICALL java_processOFDResult(JNIEnv* env, jobject o, jstring str);

static const JNINativeMethod methods[] = {
	{ "processKeyDown",   "(I)V", java_processKeyDown },
	{ "processKeyUp",     "(I)V", java_processKeyUp },
	{ "processKeyChar",   "(I)V", java_processKeyChar },
	{ "processKeyText",   "(Ljava/lang/String;)V", java_processKeyText },

	{ "processPointerDown", "(IIII)V", java_processPointerDown },
	{ "processPointerUp",   "(IIII)V", java_processPointerUp },
	{ "processPointerMove", "(IIII)V", java_processPointerMove },

	{ "processSurfaceCreated",      "(Landroid/view/Surface;)V",  java_processSurfaceCreated },
	{ "processSurfaceDestroyed",    "()V",                        java_processSurfaceDestroyed },
	{ "processSurfaceResized",      "(Landroid/view/Surface;)V",  java_processSurfaceResized },
	{ "processSurfaceRedrawNeeded", "()V",                        java_processSurfaceRedrawNeeded },

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
static void CacheMethodRefs(JNIEnv* env) {
	JAVA_openKeyboard    = JavaGetIMethod(env, "openKeyboard",    "(Ljava/lang/String;I)V");
	JAVA_setKeyboardText = JavaGetIMethod(env, "setKeyboardText", "(Ljava/lang/String;)V");
	JAVA_closeKeyboard   = JavaGetIMethod(env, "closeKeyboard",   "()V");

	JAVA_getWindowState  = JavaGetIMethod(env, "getWindowState",  "()I");
	JAVA_enterFullscreen = JavaGetIMethod(env, "enterFullscreen", "()V");
	JAVA_exitFullscreen  = JavaGetIMethod(env, "exitFullscreen",  "()V");

	JAVA_getDpiX      = JavaGetIMethod(env, "getDpiX", "()F");
	JAVA_getDpiY      = JavaGetIMethod(env, "getDpiY", "()F");
	JAVA_setupForGame = JavaGetIMethod(env, "setupForGame", "()V");

	JAVA_processedSurfaceDestroyed = JavaGetIMethod(env, "processedSurfaceDestroyed", "()V");
	JAVA_processEvents             = JavaGetIMethod(env, "processEvents",             "()V");

	JAVA_showAlert = JavaGetIMethod(env, "showAlert", "(Ljava/lang/String;Ljava/lang/String;)V");
	JAVA_setRequestedOrientation = JavaGetIMethod(env, "setRequestedOrientation", "(I)V");
	JAVA_openFileDialog = JavaGetIMethod(env, "openFileDialog", "(Ljava/lang/String;)I");
	JAVA_saveFileDialog = JavaGetIMethod(env, "saveFileDialog", "(Ljava/lang/String;Ljava/lang/String;)I");
}

// TODO move to bottom of file?
void Window_Init(void) {
	JNIEnv* env;
	/* TODO: ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN, 0); */
	JavaGetCurrentEnv(env);
	JavaRegisterNatives(env, methods);
	CacheMethodRefs(env);

	WindowInfo.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
	Input_SetTouchMode(true);
	Input.Sources = INPUT_SOURCE_NORMAL;

	DisplayInfo.Depth  = 32;
	DisplayInfo.ScaleX = JavaICall_Float(env, JAVA_getDpiX, NULL);
	DisplayInfo.ScaleY = JavaICall_Float(env, JAVA_getDpiY, NULL);
}

static void RemakeWindowSurface(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	winCreated = false;

	/* Force window to be destroyed and re-created */
	/* (see comments in setupForGame for why this has to be done) */
	JavaICall_Void(env, JAVA_setupForGame, NULL);
	Platform_LogConst("Entering wait for window exist loop..");

	/* Loop until window gets created by main UI thread */
	/* (i.e. until processSurfaceCreated is received) */
	while (!winCreated) {
		Window_ProcessEvents(0.01);
		Thread_Sleep(10);
	}

	Platform_LogConst("OK window created..");
}

static void DoCreateWindow(void) {
	WindowInfo.Exists = true;
	RemakeWindowSurface();
	/* always start as fullscreen */
	Window_EnterFullscreen();
}
void Window_Create2D(int width, int height) { DoCreateWindow(); }
void Window_Create3D(int width, int height) { DoCreateWindow(); }

void Window_SetTitle(const cc_string* title) {
	/* TODO: Implement this somehow */
	/* Maybe https://stackoverflow.com/questions/2198410/how-to-change-title-of-activity-in-android */
}

void Clipboard_GetText(cc_string* value) {
	JavaCall_Void_String("getClipboardText", value);
}
void Clipboard_SetText(const cc_string* value) {
	JavaCall_String_Void("setClipboardText", value);
}

int Window_GetWindowState(void) { 
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	return JavaICall_Int(env, JAVA_getWindowState, NULL);
}

cc_result Window_EnterFullscreen(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaICall_Void(env, JAVA_enterFullscreen, NULL);
	return 0; 
}

cc_result Window_ExitFullscreen(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaICall_Void(env, JAVA_exitFullscreen, NULL);
	return 0; 
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) { } /* Window already visible */
void Window_SetSize(int width, int height) { }

void Window_Close(void) {
	WindowInfo.Exists = false;
	Event_RaiseVoid(&WindowEvents.Closing);
	/* TODO: Do we need to call finish here */
	/* ANativeActivity_finish(app->activity); */
}

void Window_ProcessEvents(double delta) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	/* TODO: Cache the java env */
	JavaICall_Void(env, JAVA_processEvents, NULL);
}

/* No actual mouse cursor */
static void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
static void Cursor_DoSetVisible(cc_bool visible) { }

static void ShowDialogCore(const char* title, const char* msg) {
	JNIEnv* env;
	jvalue args[2];
	JavaGetCurrentEnv(env);

	Platform_LogConst(title);
	Platform_LogConst(msg);
	/* in case surface destroyed message has arrived */
	Window_ProcessEvents(0.0);

	args[0].l = JavaMakeConst(env, title);
	args[1].l = JavaMakeConst(env, msg);
	JavaICall_Void(env, JAVA_showAlert, args);
	(*env)->DeleteLocalRef(env, args[0].l);
	(*env)->DeleteLocalRef(env, args[1].l);
}

static FileDialogCallback ofd_callback;
static int ofd_action;
static void JNICALL java_processOFDResult(JNIEnv* env, jobject o, jstring str) {
    const char* raw;

    char buffer[NATIVE_STR_LEN];
	cc_string path = JavaGetString(env, str, buffer);
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
    JavaGetCurrentEnv(env);

    ofd_callback = open_args->Callback;
    ofd_action   = open_args->uploadAction;

    // TODO use filters
    args[0].l = JavaMakeConst(env, open_args->uploadFolder);
    int OK = JavaICall_Int(env, JAVA_openFileDialog, args);
    (*env)->DeleteLocalRef(env, args[0].l);
    // TODO: Better error handling
    return OK ? 0 : ERR_INVALID_ARGUMENT;
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* save_args) {
    JNIEnv* env;
    jvalue args[2];
    JavaGetCurrentEnv(env);
    if (!save_args->defaultName.length) return SFD_ERR_NEED_DEFAULT_NAME;

    // save the item to a temp file, which is then (usually) later deleted by intent callback
    cc_string tmpDir = String_FromConst("Exported");
    Directory_Create(&tmpDir);

    cc_string path; char pathBuffer[FILENAME_SIZE];
    String_InitArray(path, pathBuffer);
    String_Format3(&path, "%s/%s%c", &tmpDir, &save_args->defaultName, save_args->filters[0]);
    save_args->Callback(&path);
    // TODO kinda ugly, maybe a better way?
    cc_string file = String_UNSAFE_SubstringAt(&path, String_IndexOf(&path, '/') + 1);

    args[0].l = JavaMakeString(env, &path);
    args[1].l = JavaMakeString(env, &file);
    int OK = JavaICall_Int(env, JAVA_saveFileDialog, args);
    (*env)->DeleteLocalRef(env, args[0].l);
    (*env)->DeleteLocalRef(env, args[1].l);
    // TODO: Better error handling
    return OK ? 0 : ERR_INVALID_ARGUMENT;
}

static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
	fb_bmp     = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	ANativeWindow_Buffer buffer;
	cc_uint32* src;
	cc_uint32* dst;
	ARect b;
	int y, res, size;

	/* window not created yet */
	if (!win_handle) return;
	b.left = r.x; b.right  = r.x + r.Width;
	b.top  = r.y; b.bottom = r.y + r.Height;

	/* Platform_Log4("DIRTY: %i,%i - %i,%i", &b.left, &b.top, &b.right, &b.bottom); */
	res  = ANativeWindow_lock(win_handle, &buffer, &b);
	if (res) Logger_Abort2(res, "Locking window pixels");
	/* Platform_Log4("ADJUS: %i,%i - %i,%i", &b.left, &b.top, &b.right, &b.bottom); */

	/* In some rare cases, the returned locked region will be entire area of the surface */
	/* This can cause a crash if the surface has been resized (i.e. device rotated), */
	/* but the framebuffer has not been resized yet. So always constrain bounds. */
	b.left = min(b.left, fb_bmp.width);  b.right  = min(b.right,  fb_bmp.width);
	b.top  = min(b.top,  fb_bmp.height); b.bottom = min(b.bottom, fb_bmp.height);

	src  = (cc_uint32*)fb_bmp.scan0 + b.left;
	dst  = (cc_uint32*)buffer.bits  + b.left;
	size = (b.right - b.left) * 4;

	for (y = b.top; y < b.bottom; y++) {
		Mem_Copy(dst + y * buffer.stride, src + y * fb_bmp.width, size);
	}
	res = ANativeWindow_unlockAndPost(win_handle);
	if (res) Logger_Abort2(res, "Unlocking window pixels");
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	Mem_Free(bmp->scan0);
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* kArgs) {
	JNIEnv* env;
	jvalue args[2];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, kArgs->text);
	args[1].i = kArgs->type;
	JavaICall_Void(env, JAVA_openKeyboard, args);
	(*env)->DeleteLocalRef(env, args[0].l);
}

void Window_SetKeyboardText(const cc_string* text) {
	JNIEnv* env;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, text);
	JavaICall_Void(env, JAVA_setKeyboardText, args);
	(*env)->DeleteLocalRef(env, args[0].l);
}

void Window_CloseKeyboard(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaICall_Void(env, JAVA_closeKeyboard, NULL);
}

void Window_LockLandscapeOrientation(cc_bool lock) {
	JNIEnv* env;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	/* SCREEN_ORIENTATION_SENSOR_LANDSCAPE = 0x00000006 */
	/* SCREEN_ORIENTATION_UNSPECIFIED = 0xffffffff */
	args[0].i = lock ? 0x00000006 : 0xffffffff;
	JavaICall_Void(env, JAVA_setRequestedOrientation, args);
}

void Window_EnableRawMouse(void)  { DefaultEnableRawMouse(); }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { DefaultDisableRawMouse(); }
#endif
