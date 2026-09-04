#include "Core.h"
#if CC_WIN_BACKEND == CC_WIN_BACKEND_WAYLAND

#define CC_BUILD_EGL
#include "_WindowBase.h"
#include "String_.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Options.h"
#include "Errors.h"
#include "Utils.h"
#include "Platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>


/*########################################################################################################################*
*-----------------------------------------------------Wayland socket------------------------------------------------------*
*#########################################################################################################################*/
static int wl_fd = -1;

static void wl_socket_get_target(struct sockaddr_un* addr) {
	char* xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (!xdg_runtime_dir) Process_Abort("no XDG runtime directory");

	cc_string str = String_FromArray(addr->sun_path);
	String_AppendConst(&str, xdg_runtime_dir);
	String_Append(&str, '/');

	char* wayland_display = getenv("WAYLAND_DISPLAY");
	wayland_display = wayland_display ? wayland_display : "wayland-0";
	String_AppendConst(&str, wayland_display);
}

static void wl_socket_connect(void) {
	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	wl_socket_get_target(&addr);

	wl_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (wl_fd == -1) Process_Abort2(errno, "connect wayland socket");

	int res = Socket_Connect(wl_fd, &addr, sizeof(addr));
	if (res) Process_Abort2(res, "connect wayland socket");
}

static void wl_socket_close(void) {
	if (wl_fd == -1) return;
	Socket_Close(wl_fd);
}


/*########################################################################################################################*
*--------------------------------------------------Public implementation--------------------------------------------------*
*#########################################################################################################################*/
void Window_PreInit(void) { 
	DisplayInfo.CursorVisible = true;
}

void Window_Init(void) {
	Input.Sources = INPUT_SOURCE_NORMAL;
	wl_socket_connect();

	/* TODO: Use Xinerama and XRandR for querying these */
	DisplayInfo.Width  = 640;
	DisplayInfo.Height = 480;
	DisplayInfo.Depth  = 32;
	DisplayInfo.ScaleX = 1;
	DisplayInfo.ScaleY = 1;
}

void Window_Free(void) {
	wl_socket_close(); // TODO
}


void Window_Create2D(int width, int height) { 
	// TODO 
	Window_Main.Exists     = true;
	Window_Main.Handle.val = 1;
}

void Window_Create3D(int width, int height) { 
	// TODO
	Window_Main.Exists     = true;
	Window_Main.Handle.val = 1;
}

void Window_Destroy(void) {
	// TODO
}

void Window_SetTitle(const cc_string* title) {
	// TODO
}

void Clipboard_GetText(cc_string* value) {
	// TODO
}

void Clipboard_SetText(const cc_string* value) {
	// TODO
}

int Window_GetWindowState(void) {
	// TODO
	return WINDOW_STATE_NORMAL;
}

cc_result Window_EnterFullscreen(void) {
	// TODO
	return 0;
}

cc_result Window_ExitFullscreen(void) {
	// TODO
	return 0;
}

int Window_IsObscured(void) { return 0; }

void Window_Show(void) {
	// TODO
}

void Window_SetSize(int width, int height) {
	// TODO
}

void Window_RequestClose(void) {
	Event_RaiseVoid(&WindowEvents.Closing);
}


void Window_ProcessEvents(float delta) {
	// TODO
}


void Gamepads_PreInit(void) { }

void Gamepads_Init(void) { }

void Gamepads_Process(float delta) { }


static void Cursor_GetRawPos(int* x, int* y) {
	// TODO
	*x = 0; *y = 0;
}

void Cursor_SetPosition(int x, int y) {
	// TODO
}

static void Cursor_DoSetVisible(cc_bool visible) {
	// TODO
}


/*########################################################################################################################*
*-----------------------------------------------------X11 message box-----------------------------------------------------*
*#########################################################################################################################*/
static void ShowDialogCore(const char* title, const char* msg) {
	// TODO
	Platform_LogConst(title);
	Platform_LogConst(msg);
}

static cc_result OpenSaveFileDialog(const char* args, FileDialogCallback callback, const char* defaultExt) {
	cc_string path; char pathBuffer[1024];
	char result[4096] = { 0 };
	int len;
	/* TODO this doesn't detect when Zenity doesn't exist */
	FILE* fp = popen(args, "r");
	if (!fp) return 0;

	/* result from zenity is normally just one string */
	while (fgets(result, sizeof(result), fp)) { }
	pclose(fp);

	len = String_Length(result);
	if (!len) return 0;

	String_InitArray(path, pathBuffer);
	String_AppendUtf8(&path, result, len);

	/* Add default file extension if necessary */
	if (defaultExt) {
		cc_string file = path;
		Utils_UNSAFE_GetFilename(&file);
		if (String_IndexOf(&file, '.') == -1) String_AppendConst(&path, defaultExt);
	}
	callback(&path);
	return 0;
}

cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) {
	const char* const* filters = args->filters;
	cc_string path; char pathBuffer[1024];
	int i;

	String_InitArray_NT(path, pathBuffer);
	String_Format1(&path, "zenity --file-selection --file-filter='%c (", args->description);

	for (i = 0; filters[i]; i++)
	{
		if (i) String_Append(&path, ',');
		String_Format1(&path, "*%c", filters[i]);
	}
	String_AppendConst(&path, ") |");

	for (i = 0; filters[i]; i++)
	{
		String_Format1(&path, " *%c", filters[i]);
	}
	String_AppendConst(&path, "'");

	path.buffer[path.length] = '\0';
	return OpenSaveFileDialog(path.buffer, args->Callback, NULL);
}

cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) {
	const char* const* titles   = args->titles;
	const char* const* fileExts = args->filters;
	cc_string path; char pathBuffer[1024];
	int i;

	String_InitArray_NT(path, pathBuffer);
	String_AppendConst(&path, "zenity --file-selection");
	for (i = 0; fileExts[i]; i++)
	{
		String_Format3(&path, " --file-filter='%c (*%c) | *%c'", titles[i], fileExts[i], fileExts[i]);
	}
	String_AppendConst(&path, " --save --confirm-overwrite");

	/* TODO: Utf8 encode filename */
	if (args->defaultName.length) {
		String_Format1(&path, " --filename='%s'", &args->defaultName);
	}

	path.buffer[path.length] = '\0';
	return OpenSaveFileDialog(path.buffer, args->Callback, fileExts[0]);
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
	bmp->scan0  = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "window pixels");
	bmp->width  = width;
	bmp->height = height;
	// TODO
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
	// TODO
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
	// TODO
	Mem_Free(bmp->scan0);
}


void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { }


void Window_EnableRawMouse(void) {
	DefaultEnableRawMouse();
	// TODO
}

void Window_UpdateRawMouse(void) {
	DefaultUpdateRawMouse();
	// TODO
}

void Window_DisableRawMouse(void) {
	DefaultDisableRawMouse();
	// TODO
}
#endif
