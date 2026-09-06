#include "Core.h"
#if CC_WIN_BACKEND == CC_WIN_BACKEND_WAYLAND

// https://gaultier.github.io/blog/wayland_from_scratch.html
// https://wayland.freedesktop.org/docs/book/Protocol.html#wire-format

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

// TODO async..
static void wl_socket_send(const void* ptr, int size) {
	while (size) {
		cc_uint32 sent = 0;
		cc_result res  = Socket_Write(wl_fd, ptr, size, &sent);

		if (res) Process_Abort2(res, "writing wayland socket");
		ptr += sent; size -= sent;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Wayland message-----------------------------------------------------*
*#########################################################################################################################*/
enum wl_field_type {
	WL_TYPE_INT32,
	WL_TYPE_STRING,
	WL_TYPE_ARRAY,
	WL_TYPE_END = 0x574C4E44,
};

typedef struct wl_string { int32_t size; char* ptr; } wl_string;
typedef struct wl_array  { int32_t size; void* ptr; } wl_array;
typedef int32_t wl_obj_id;

typedef struct wl_field {
	enum wl_field_type type;

	union {
		int32_t   int_val;
		wl_string str_val;
		wl_array  arr_val;
	};
} wl_field;

#define WL_FIELD_END           { WL_TYPE_END }
#define WL_FIELD_INT(value)    { WL_TYPE_INT32,  .int_val = (value) }
#define WL_FIELD_STR(ptr, len) { WL_TYPE_STRING, .str_val.size = len,             .str_val.ptr = ptr }
#define WL_FIELD_CONST(str)    { WL_TYPE_STRING, .str_val.size = sizeof(str) - 1, .str_val.ptr = str }
#define WL_FIELD_ARR(ptr, len) { WL_TYPE_ARRAY,  .arr_val.size = len,             .arr_val.ptr = ptr }


#define WL_MSG_HDR_SIZE    8 // 4 bytes for object ID, 4 bytes for opcode + size
#define WL_MSG_SIZE_SHIFT 16
struct wl_message {
	uint32_t obj_id;
	uint32_t opcode_size;
	char data[0x10000 - WL_MSG_HDR_SIZE]; 
};

static int wl_msg_calc_data_size(wl_field* fields) {
	wl_field* f = fields;
	int size = 0;

	while (f->type != WL_TYPE_END) 
	{
		switch (f->type) {
			case WL_TYPE_INT32:  
				size += 4; break;
			case WL_TYPE_STRING: 
				size += 4 + f->str_val.size; break;
			case WL_TYPE_ARRAY:  
				size += 4 + f->arr_val.size; break;
		}
		f++;
	}
	return size;
}

#define COPY_BYTES(dst, src, size) memcpy(dst, src, size); dst += ((size) + 3) & ~0x03;
static void wl_msg_write_data(wl_field* fields, char* dst) {
	wl_field* f = fields;

	while (f->type != WL_TYPE_END) 
	{
		switch (f->type) {
			case WL_TYPE_INT32:
				COPY_BYTES(dst, &f->int_val, sizeof(int32_t));
				break;
			case WL_TYPE_STRING: 
				COPY_BYTES(dst, &f->str_val.size, sizeof(int32_t));
				COPY_BYTES(dst,  f->str_val.ptr, f->arr_val.size);
				break;
			case WL_TYPE_ARRAY:  
				COPY_BYTES(dst, &f->arr_val.size, sizeof(int32_t));
				COPY_BYTES(dst,  f->arr_val.ptr, f->arr_val.size);
				break;
		}
		f++;
	}
}

static void wl_msg_send(int32_t senderObj, int32_t opcode, wl_field* fields) {
	struct wl_message msg;
	msg.obj_id      = senderObj;
	msg.opcode_size = opcode;

	int size = wl_msg_calc_data_size(fields);
	if (size > sizeof(msg.data)) Process_Abort("wayland message too large");

	size += WL_MSG_HDR_SIZE;
	msg.opcode_size |= size << WL_MSG_SIZE_SHIFT;
	wl_msg_write_data(fields, msg.data);

	wl_socket_send(&msg, size);
}


/*########################################################################################################################*
*------------------------------------------------Wayland event processing-------------------------------------------------*
*#########################################################################################################################*/


/*########################################################################################################################*
*-----------------------------------------------------Wayland objects-----------------------------------------------------*
*#########################################################################################################################*/
#define WL_DISPLAY_OBJ_ID 1 // 1 is reserved for wl_display singleton

static wl_obj_id wl_current_obj_id = WL_DISPLAY_OBJ_ID;
static wl_obj_id wl_allocate_obj_id(void) { return ++wl_current_obj_id; }


// === WAYLAND DISPLAY OBJECT ===
static wl_obj_id wl_display_obj_id = WL_DISPLAY_OBJ_ID;

#define WL_DISPLAY_REQ_SYNC         0
#define WL_DISPLAY_REQ_GET_REGISTRY 1

#define WL_DISPLAY_EVT_ERROR        0
#define WL_DISPLAY_EVT_DELETE_ID    1

static wl_obj_id wl_display_send_sync(void) {
	wl_obj_id callbackID = wl_allocate_obj_id();

	wl_field fields[] = {
		WL_FIELD_INT(callbackID),
		WL_FIELD_END,
	};

	wl_msg_send(wl_display_obj_id, WL_DISPLAY_REQ_SYNC, fields);
	return callbackID;
}

static wl_obj_id wl_display_send_get_registry(void) {
	wl_obj_id registryID = wl_allocate_obj_id();

	wl_field fields[] = {
		WL_FIELD_INT(registryID),
		WL_FIELD_END,
	};
	
	wl_msg_send(wl_display_obj_id, WL_DISPLAY_REQ_GET_REGISTRY, fields);
	return registryID;
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
	Platform_LogConst(title);
	Platform_LogConst(msg);

	cc_string args; char argsBuffer[1024];
	String_InitArray_NT(args, argsBuffer);
	String_Format2(&args, "zenity --info --title=\"%c\" --text=\"%c\"", title, msg);
	args.buffer[args.length] = '\0';

	/* TODO this doesn't detect when Zenity doesn't exist */
	FILE* fp = popen(argsBuffer, "r");
	if (!fp) return;

	/* result from zenity is normally just one string */
	char result[64];
	while (fgets(result, sizeof(result), fp)) { }
	pclose(fp);
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
