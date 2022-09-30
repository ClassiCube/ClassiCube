#include "Core.h"
#if defined CC_BUILD_HAIKU
extern "C" {
#include "_WindowBase.h"
#include "Graphics.h"
#include "String.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include "Utils.h"
}

#include <AppKit.h>
#include <InterfaceKit.h>
#include <OpenGLKit.h>

static BApplication* app_handle;
static BWindow* win_handle;
static BView* view_handle;
static BGLView* view_3D;
#include <pthread.h>

// Event management
enum CCEventType {
	CC_NONE,
	CC_MOUSE_SCROLL, CC_MOUSE_DOWN, CC_MOUSE_UP, CC_MOUSE_MOVE,
	CC_KEY_DOWN, CC_KEY_UP,
	CC_WIN_RESIZED, CC_WIN_FOCUS, CC_WIN_REDRAW, CC_WIN_QUIT
};
union CCEventValue { float f32; int i32; void* ptr; };
struct CCEvent {
	int type;
	CCEventValue v1, v2;
};

#define EVENTS_DEFAULT_MAX 20
static void* events_mutex;
static int events_count, events_capacity;
static CCEvent* events_list, events_default[EVENTS_DEFAULT_MAX];

static void Events_Init(void) {
	events_mutex    = Mutex_Create();
	events_capacity = EVENTS_DEFAULT_MAX;
	events_list     = events_default;
}

static void Events_Push(const CCEvent* event) {
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

// BApplication implementation
class CC_BApp : public BApplication
{
public:
	CC_BApp();
	void DispatchMessage(BMessage* msg, BHandler* handler);
};

CC_BApp::CC_BApp() : BApplication("application/x-ClassiCube") {
}

void CC_BApp::DispatchMessage(BMessage* msg, BHandler* handler) {
	CCEvent event = { 0 };
	int what = msg->what;
	
	switch (msg->what)
	{
	case B_QUIT_REQUESTED:
		event.type = CC_WIN_QUIT;
		break;
	default:
		//Platform_Log1("APP DISPATCH: %i", &what);
		break;
	}
	if (event.type) Events_Push(&event);
	BApplication::DispatchMessage(msg, handler);
}

// BWindow implementation
class CC_BWindow : public BWindow
{
	public:
		CC_BWindow(BRect frame);
		void DispatchMessage(BMessage* msg, BHandler* handler);
};

CC_BWindow::CC_BWindow(BRect frame) : BWindow(frame, "", B_TITLED_WINDOW, 0) {
}

static int last_buttons;
static void UpdateMouseButton(int btn, int pressed) {
	CCEvent event;
	event.type   = pressed ? CC_MOUSE_DOWN : CC_MOUSE_UP;
	event.v1.i32 = btn;
	Events_Push(&event);
}

static void UpdateMouseButtons(int buttons) {
	// BeOS API is really odd in that it only provides you with a bitmask
	//  of 'current mouse buttons pressed'
	int changed  = buttons ^ last_buttons;
	
	// TODO move logic to UpdateMouseButton instead?
	if (changed & B_PRIMARY_MOUSE_BUTTON)   
		UpdateMouseButton(KEY_LMOUSE, buttons & B_PRIMARY_MOUSE_BUTTON);
	if (changed & B_SECONDARY_MOUSE_BUTTON)
		UpdateMouseButton(KEY_RMOUSE, buttons & B_SECONDARY_MOUSE_BUTTON);
	if (changed & B_TERTIARY_MOUSE_BUTTON) 
		UpdateMouseButton(KEY_MMOUSE, buttons & B_TERTIARY_MOUSE_BUTTON);
	last_buttons = buttons;
}

void CC_BWindow::DispatchMessage(BMessage* msg, BHandler* handler) {
	CCEvent event = { 0 };
	BPoint where;
	float delta;
	int value, width, height;
	bool active;
	
	switch (msg->what)
	{
	case B_MOUSE_UP:
	case B_MOUSE_DOWN:
		if (msg->FindInt32("buttons", &value) == B_OK) {
			UpdateMouseButtons(value);
		}
		break;
	case B_MOUSE_MOVED:
		if (msg->FindPoint("where", &where) == B_OK) {
			event.type   = CC_MOUSE_MOVE;
			event.v1.i32 = where.x;
			event.v2.i32 = where.y;
		}
		break;
	case B_MOUSE_WHEEL_CHANGED:
		if (msg->FindFloat("be:wheel_delay_y", &delta) == B_OK) {
			event.type   = CC_MOUSE_SCROLL;
			event.v1.f32 = delta;
		}
		break;
		
	case B_WINDOW_ACTIVATED:
		if (msg->FindBool("active", &active) == B_OK) {
			event.type   = CC_WIN_FOCUS;
			event.v1.i32 = active;
		}
		break;
	case B_WINDOW_RESIZED:
		if (msg->FindInt32("width",  &width)  == B_OK &&
			msg->FindInt32("height", &height) == B_OK) {
			event.type   = CC_WIN_RESIZED;
			event.v1.i32 = width;
			event.v2.i32 = height;
		}
		break;
	case B_QUIT_REQUESTED:
		Platform_LogConst("WIN_QUIT");
		break;
	case _UPDATE_:
		event.type = CC_WIN_REDRAW;
		break;
	default:
		Platform_LogConst("UNHANDLED MESSAGE:");
		msg->PrintToStream();
		break;
	}
	if (event.type) Events_Push(&event);
	BWindow::DispatchMessage(msg, handler);
}


static void AppThread(void) {
		app_handle = new CC_BApp();
		// runs forever
		app_handle->Run();
		delete app_handle;
}

static void RunApp(void) {
	void* thread = Thread_Create(AppThread);
	Thread_Start2(thread, AppThread);
	Thread_Detach(thread);
	
	// wait for BApplication to be started in other thread
	do {
		Thread_Sleep(10);
	} while (!app_handle || app_handle->IsLaunching());
	
	Platform_LogConst("App initialised");
	pthread_t tid = pthread_self();
	Platform_Log1("MAIN THREAD: %x", (cc_uintptr*)&tid);
}

void Window_Init(void) {
	Events_Init();
	RunApp();
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect frame = screen.Frame();
	
	DisplayInfo.Width  = (int)frame.Width();
	DisplayInfo.Height = (int)frame.Height();
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

static void DoCreateWindow(int width, int height) {
	int x = Display_CentreX(width), y = Display_CentreY(height);
	BRect frame(x, y, x + width, y + height);
	win_handle = new CC_BWindow(frame);
	
	WindowInfo.Exists = true;
	WindowInfo.Handle = win_handle;
	
	frame = win_handle->Bounds();
	WindowInfo.Width  = (int)frame.Width();
	WindowInfo.Height = (int)frame.Height();
}

void Window_Create2D(int width, int height) {
	DoCreateWindow(width, height);
	view_handle = new BView(win_handle->Bounds(), "CC_LAUNCHER",
						B_FOLLOW_LEFT | B_FOLLOW_TOP, 0);
	win_handle->AddChild(view_handle);
}

void Window_Create3D(int width, int height) {
	DoCreateWindow(width, height);
	view_3D = new BGLView(win_handle->Bounds(), "CC_GAME",
						B_FOLLOW_LEFT | B_FOLLOW_TOP, 
						BGL_RGB | BGL_ALPHA | BGL_DOUBLE | BGL_DEPTH, 0);
	view_handle = view_3D;
	win_handle->AddChild(view_handle);
}

void Window_SetTitle(const cc_string* title) {
	char raw[NATIVE_STR_LEN];
	Platform_EncodeUtf8(raw, title);
	
	win_handle->Lock(); // TODO even need to lock/unlock?
	win_handle->SetTitle(raw);
	win_handle->Unlock();
}

void Clipboard_GetText(cc_string* value) {
}

void Clipboard_SetText(const cc_string* value) {
}

int Window_GetWindowState(void) {
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	return ERR_NOT_SUPPORTED;
}
cc_result Window_ExitFullscreen(void) { return ERR_NOT_SUPPORTED; }

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
	win_handle->Lock(); // TODO even need to lock/unlock ?
	win_handle->Show();
	win_handle->Unlock();
}

void Window_SetSize(int width, int height) {
	win_handle->Lock(); // TODO even need to lock/unlock ?
	win_handle->ResizeTo(width, height);
	win_handle->Unlock();
}

void Window_Close(void) {
	BMessage* msg = new BMessage(B_QUIT_REQUESTED);
	app_handle->PostMessage(msg);
}

void Window_ProcessEvents(void) {
	CCEvent event;
	int btn, key;
	
	while (Events_Pull(&event))
	{
		//Platform_Log1("GO: %i", &event.type);
		switch (event.type)
		{
		case CC_MOUSE_SCROLL:
			Mouse_ScrollWheel(event.v1.f32);
			break;
		case CC_MOUSE_DOWN:
			Input_SetPressed(event.v1.i32);
			break;
		case CC_MOUSE_UP:
			Input_SetReleased(event.v1.i32);
			break; 
		case CC_MOUSE_MOVE:
			//Platform_Log2("POS: %i,%i", &event.v1.i32, &event.v2.i32);
			Pointer_SetPosition(0, event.v1.i32, event.v2.i32);
			break;
		case CC_KEY_DOWN:
			break; 
		case CC_KEY_UP:
			break;
		case CC_WIN_RESIZED:
			WindowInfo.Width  = event.v1.i32;
			WindowInfo.Height = event.v2.i32;
			Platform_Log2("WINIE: %i,%i", &WindowInfo.Width, &WindowInfo.Height);
			Event_RaiseVoid(&WindowEvents.Resized);
			break;
		case CC_WIN_FOCUS:
			WindowInfo.Focused = event.v1.i32;
			Event_RaiseVoid(&WindowEvents.FocusChanged);
			break;
		case CC_WIN_REDRAW:
			Event_RaiseVoid(&WindowEvents.RedrawNeeded);
			break;
		case CC_WIN_QUIT:
			WindowInfo.Exists = false;
			Event_RaiseVoid(&WindowEvents.Closing);
			break;
		}
	}
}

static void Cursor_GetRawPos(int* x, int* y) {
	BPoint where;
	uint32 buttons;
	
	win_handle->Lock();
	view_handle->GetMouse(&where, &buttons, false);
	win_handle->Unlock();
	
	// TODO: Should checkQueue should be true
	*x = (int)where.x;
	*y = (int)where.y;
}
void Cursor_SetPosition(int x, int y) {
	// TODO no API for this?
}

static void Cursor_DoSetVisible(cc_bool visible) {
	if (visible) {
		app_handle->ShowCursor();
	} else {
		app_handle->HideCursor();
	}
}

static void ShowDialogCore(const char* title, const char* msg) {
	BAlert* alert = new BAlert(title, msg, "OK");
	// doesn't show title by default
	alert->SetLook(B_TITLED_WINDOW_LOOK);
	alert->Go();
}

cc_result Window_OpenFileDialog(const char* const* filters, OpenFileDialogCallback callback) {
	return ERR_NOT_SUPPORTED;
}

static BBitmap* win_framebuffer;
static struct Bitmap win_fb;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
	BRect bounds(0, 0, bmp->width, bmp->height);
	win_framebuffer = new BBitmap(bounds, B_RGB32);
	
	bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "framebuffer pixels");
	win_fb = *bmp;
}

void Window_DrawFramebuffer(Rect2D r) {
	void* dst_pixels = win_framebuffer->Bits();
	int32 dst_stride = win_framebuffer->BytesPerRow();
	
	// TODO redo Bitmap so it supports strides
	for (int y = r.Y; y < r.Y + r.Height; y++)
	{
		BitmapCol* src = Bitmap_GetRow(&win_fb, y) + r.X;
		char*  dst     = (char*)dst_pixels + dst_stride * y + r.X * 4;
		Mem_Copy(dst, src, r.Width * 4);
	}
	
	BRect rect(r.X, r.Y, r.X + r.Width, r.Y + r.Height);
	win_handle->Lock();
	view_handle->DrawBitmap(win_framebuffer, rect, rect);
	win_handle->Unlock();
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	delete win_framebuffer;
	Mem_Free(bmp->scan0);
	bmp->scan0 = NULL;
}

void Window_OpenKeyboard(struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) {  }

void Window_EnableRawMouse(void) {
	RegrabMouse();
	//SDL_SetRelativeMouseMode(true);
	Input_RawMode = true;
}
void Window_UpdateRawMouse(void) { CentreMousePosition(); }

void Window_DisableRawMouse(void) {
	RegrabMouse();
	//SDL_SetRelativeMouseMode(false);
	Input_RawMode = false;
}


/*########################################################################################################################*
*-----------------------------------------------------OpenGL context------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_GL && !defined CC_BUILD_EGL
static cc_bool win_vsync;

void GLContext_Create(void) {
	view_3D->LockGL();
}

void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return true; }
void GLContext_Free(void) {
	view_3D->UnlockGL();
}

void* GLContext_GetAddress(const char* function) {
	return view_3D->GetGLProcAddress(function);
}

cc_bool GLContext_SwapBuffers(void) {
	view_3D->SwapBuffers(win_vsync);
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	win_vsync = vsync;
}
void GLContext_GetApiInfo(cc_string* info) { }
#endif
#endif
