#include "Platform.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Drawer2D.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HDC hdc;
HBITMAP hbmp;
HANDLE heap;

UInt8* Platform_NewLine = "\r\n";
UInt8 Platform_DirectorySeparator = '\\';
ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;

void Platform_Init(void) {
	heap = GetProcessHeap(); /* TODO: HeapCreate instead? probably not */
	hdc = CreateCompatibleDC(NULL);
	if (hdc == NULL) ErrorHandler_Fail("Failed to get screen DC");

	UInt32 deviceNum = 0;
	/* Get available video adapters and enumerate all monitors */
	DISPLAY_DEVICEA device;
	Platform_MemSet(&device, 0, sizeof(DISPLAY_DEVICEA));
	device.cb = sizeof(DISPLAY_DEVICEA);

	while (EnumDisplayDevicesA(NULL, deviceNum, &device, 0)) {
		deviceNum++;
		if ((device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) continue;
		bool devPrimary = false;
		DisplayResolution resolution = DisplayResolution_Make(0, 0, 0, 0.0f);
		DEVMODEA mode;
		Platform_MemSet(&mode, 0, sizeof(DEVMODEA));
		mode.dmSize = sizeof(DEVMODEA);

		/* The second function should only be executed when the first one fails (e.g. when the monitor is disabled) */
		if (EnumDisplaySettingsA(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode) ||
			EnumDisplaySettingsA(device.DeviceName, ENUM_REGISTRY_SETTINGS, &mode)) {
			if (mode.dmBitsPerPel > 0) {
				resolution = DisplayResolution_Make(mode.dmPelsWidth, mode.dmPelsHeight,
					mode.dmBitsPerPel, (Real32)mode.dmDisplayFrequency);
				devPrimary = (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
			}
		}

		/* This device has no valid resolution, ignore it */
		if (resolution.Width == 0 && resolution.Height == 0) continue;
		if (!devPrimary) continue;
		DisplayDevice_Default = DisplayDevice_Make(&resolution);
	}
}

void Platform_Free(void) {
	HeapDestroy(heap);
	DeleteDC(hdc);
}

void* Platform_MemAlloc(UInt32 numBytes) {
	return HeapAlloc(heap, 0, numBytes);
}

void* Platform_MemRealloc(void* mem, UInt32 numBytes) {
	return HeapReAlloc(heap, 0, mem, numBytes);
}

void Platform_MemFree(void* mem) {
	HeapFree(heap, 0, mem);
}

void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes) {
	memset(dst, value, numBytes);
}

void Platform_MemCpy(void* dst, void* src, UInt32 numBytes) {
	memcpy(dst, src, numBytes);
}


void Platform_Log(STRING_PURE String* message) {
	/* TODO: log to console */
}

/* Not worth making this an actual function, just use an inline macro. */
#define Platform_ReturnDateTime(sysTime)\
DateTime time;\
time.Year = sysTime.wYear;\
time.Month = (UInt8)sysTime.wMonth;\
time.Day = (UInt8)sysTime.wDay;\
time.Hour = (UInt8)sysTime.wHour;\
time.Minute = (UInt8)sysTime.wMinute;\
time.Second = (UInt8)sysTime.wSecond;\
time.Milliseconds = sysTime.wMilliseconds;\
return time;\

DateTime Platform_CurrentUTCTime(void) {
	SYSTEMTIME utcTime;
	GetSystemTime(&utcTime);
	Platform_ReturnDateTime(utcTime);
}

DateTime Platform_CurrentLocalTime(void) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	Platform_ReturnDateTime(localTime);
}


bool Platform_FileExists(STRING_PURE String* path) {
	UInt32 attribs = GetFileAttributesA(path->buffer);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Platform_DirectoryExists(STRING_PURE String* path) {
	UInt32 attribs = GetFileAttributesA(path->buffer);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Platform_DirectoryCreate(STRING_PURE String* path) {
	BOOL success = CreateDirectoryA(path->buffer, NULL);
	return success ? 0 : GetLastError();
}


ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path, bool readOnly) {
	UINT32 access = GENERIC_READ;
	if (!readOnly) access |= GENERIC_WRITE;
	HANDLE handle = CreateFileA(path->buffer, access, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path) {
	UINT32 access = GENERIC_READ | GENERIC_WRITE;
	HANDLE handle = CreateFileA(path->buffer, access, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead) {
	BOOL success = ReadFile((HANDLE)file, buffer, count, bytesRead, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten) {
	BOOL success = WriteFile((HANDLE)file, buffer, count, bytesWritten, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_FileClose(void* file) {
	BOOL success = CloseHandle((HANDLE)file);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType) {
	DWORD pos;
	switch (seekType) {
	case STREAM_SEEKFROM_BEGIN:
		pos = SetFilePointer(file, offset, NULL, 0); break;
	case STREAM_SEEKFROM_CURRENT:
		pos = SetFilePointer(file, offset, NULL, 1); break;
	case STREAM_SEEKFROM_END:
		pos = SetFilePointer(file, offset, NULL, 2); break;
	default:
		ErrorHandler_Fail("Invalid SeekType provided when seeking file");
	}
	return pos == INVALID_SET_FILE_POINTER ? GetLastError() : 0;
}

UInt32 Platform_FilePosition(void* file) {
	DWORD pos = SetFilePointer(file, 0, NULL, 1); /* SEEK_CUR */
	if (pos == INVALID_SET_FILE_POINTER) {
		ErrorHandler_FailWithCode(GetLastError(), "Getting file position");
	}
	return pos;
}

UInt32 Platform_FileLength(void* file) {
	DWORD pos = GetFileSize(file, NULL);
	if (pos == INVALID_FILE_SIZE) {
		ErrorHandler_FailWithCode(GetLastError(), "Getting file length");
	}
	return pos;
}


void Platform_ThreadSleep(UInt32 milliseconds) {
	Sleep(milliseconds);
}


void Platform_MakeFont(FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style) {
	desc->Size    = size; 
	desc->Style   = style;
	LOGFONTA font = { 0 };

	font.lfHeight    = -Math_Ceil(size * GetDeviceCaps(hdc, LOGPIXELSY) / 72.0f);
	font.lfUnderline = style == FONT_STYLE_UNDERLINE;
	font.lfWeight    = style == FONT_STYLE_BOLD ? FW_BOLD : FW_NORMAL;
	font.lfQuality   = ANTIALIASED_QUALITY;

	String dstName = String_Init(font.lfFaceName, 0, LF_FACESIZE);
	String_AppendString(&dstName, fontName);
	desc->Handle = CreateFontIndirectA(&font);
	if (desc->Handle == NULL) ErrorHandler_Fail("Creating font handle failed");
}

void Platform_FreeFont(FontDesc* desc) {
	if (!DeleteObject(desc->Handle)) ErrorHandler_Fail("Deleting font handle failed");
	desc->Handle = NULL;
}

/* TODO: Associate Font with device */
Size2D Platform_MeasureText(struct DrawTextArgs_* args) {
	HDC hDC = GetDC(NULL);
	RECT r = { 0 };
	DrawTextA(hDC, args->Text.buffer, args->Text.length,
		&r, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
	return Size2D_Make(r.right, r.bottom);
}

void Platform_DrawText(struct DrawTextArgs_* args, Int32 x, Int32 y) {
	HDC hDC = GetDC(NULL);
	RECT r = { 0 };
	DrawTextA(hDC, args->Text.buffer, args->Text.length,
		&r, DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
}

void Platform_SetBitmap(struct Bitmap_* bmp) {
	hbmp = CreateBitmap(bmp->Width, bmp->Height, 1, 32, bmp->Scan0);
	if (hbmp == NULL) ErrorHandler_Fail("Creating bitmap handle failed");
	/* TODO: Should we be using CreateDIBitmap here? */

	if (!SelectObject(hdc, hbmp)) ErrorHandler_Fail("Selecting bitmap handle");
}

void Platform_ReleaseBitmap(void) {
	if (!DeleteObject(hbmp)) ErrorHandler_Fail("Deleting bitmap handle failed");
	hbmp = NULL;
}