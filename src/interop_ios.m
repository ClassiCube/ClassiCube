#include "Window.h"
#include "Platform.h"
#include "String.h"
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

	Platform_EncodeString(raw, value);
	str = [NSString stringWithUTF8String:raw];
	[UIPasteboard generalPasteboard].string = str;
	[str release];
}

void Window_SetTitle(const cc_string* title) {
	/* TODO: Implement this somehow */
}

void Window_SetSize(int width, int height) { }

void Window_EnableRawMouse(void)  { }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { }
