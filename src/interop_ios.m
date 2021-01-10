#include "Window.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include <UIKit/UIPasteboard.h>

void Clipboard_GetText(cc_string* value) {
	const char* raw;
	NSString* str;
        int len;

	str = [UIPasteboard generalPasteboard].string;
	if (!str) return;

	raw = str.UTF8String;
	String_AppendUtf8(value, raw, String_Length(raw));
	[str release];
}

void Clipboard_SetText(const cc_string* value) {
	char raw[NATIVE_STR_LEN];
	NSString* str;
	int len;

	Platform_EncodeUtf8(raw, value);
	str = [NSString stringWithUTF8String:raw];
	[UIPasteboard generalPasteboard].string = str;
	[str release];
}

/*########################################################################################################################*
*---------------------------------------------------------Window----------------------------------------------------------*
*#########################################################################################################################*/
void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
void Cursor_DoSetVisible(cc_bool visible) { }

void Window_SetTitle(const cc_string* title) {
	/* TODO: Implement this somehow */
}

void Window_Init(void) { }
void Window_Create(int width, int height) { }
void Window_SetSize(int width, int height) { }
void Window_Close(void) { }

void Window_Show(void) { }
void Window_ProcessEvents(void) { }
void ShowDialogCore(const char* title, const char* msg) { }

void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) { }
void Window_SetKeyboardText(const cc_string* text) { }
void Window_CloseKeyboard(void) { }

int Window_GetWindowState(void) { return WINDOW_STATE_NORMAL; }
cc_result Window_EnterFullscreen(void) { return ERR_NOT_SUPPORTED; }
cc_result Window_ExitFullscreen(void) { return ERR_NOT_SUPPORTED; }

void Window_AllocFramebuffer(struct Bitmap* bmp) { }
void Window_DrawFramebuffer(Rect2D r) { }
void Window_FreeFramebuffer(struct Bitmap* bmp) { }

void Window_EnableRawMouse(void)  { }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { }


/*########################################################################################################################*
*--------------------------------------------------------GLContext--------------------------------------------------------*
*#########################################################################################################################*/
void GLContext_Create(void) { }
void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return false; }
void GLContext_Free(void) { }

void* GLContext_GetAddress(const char* function) { return NULL; }

cc_bool GLContext_SwapBuffers(void) { return false; }
void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) { }
void GLContext_GetApiInfo(cc_string* info) { }