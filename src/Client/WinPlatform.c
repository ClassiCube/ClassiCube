#include "Platform.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "ExtMath.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


HANDLE heap;
void Platform_Init(void) {
	heap = GetProcessHeap(); /* TODO: HeapCreate instead? probably not */	
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
}

void* Platform_MemAlloc(UInt32 numBytes) {
	return HeapAlloc(heap, 0, numBytes);
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


void Platform_Log(STRING_TRANSIENT String* message) {
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


bool Platform_FileExists(STRING_TRANSIENT String* path) {
	UInt32 attribs = GetFileAttributesA(path->buffer);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Platform_DirectoryExists(STRING_TRANSIENT String* path) {
	UInt32 attribs = GetFileAttributesA(path->buffer);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Platform_DirectoryCreate(STRING_TRANSIENT String* path) {
	BOOL success = CreateDirectoryA(path->buffer, NULL);
	return success ? 0 : GetLastError();
}


ReturnCode Platform_FileOpen(void** file, STRING_TRANSIENT String* path, bool readOnly) {
	UINT32 access = GENERIC_READ;
	if (!readOnly) access |= GENERIC_WRITE;
	HANDLE handle = CreateFileA(path->buffer, access, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileCreate(void** file, STRING_TRANSIENT String* path) {
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


void Platform_MakeFont(FontDesc* desc) {
	LOGFONTA font = { 0 };
	font.lfHeight    = -Math_Ceil(desc->Size * GetDeviceCaps(hDC, LOGPIXELSY) / 72.0f);
	font.lfUnderline = desc->Style == FONT_STYLE_UNDERLINE;
	font.lfWeight    = desc->Style == FONT_STYLE_BOLD ? FW_BOLD : FW_NORMAL;
	font.lfQuality   = ANTIALIASED_QUALITY;

	desc->Handle = CreateFontIndirectA(&font);
	if (desc->Handle == NULL) {
		ErrorHandler_Fail("Creating font handle failed");
	}
}

void Platform_FreeFont(FontDesc* desc) {
	if (!DeleteObject(desc->Handle)) {
		ErrorHandler_Fail("Deleting font handle failed");
	}
	desc->Handle = NULL;
}
