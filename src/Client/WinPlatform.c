#include "Platform.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Drawer2D.h"
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <Windows.h>
#include <stdlib.h>

HDC hdc;
HANDLE heap;
bool stopwatch_highResolution;
LARGE_INTEGER stopwatch_freq;

UInt8* Platform_NewLine = "\r\n";
UInt8 Platform_DirectorySeparator = '\\';
ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
ReturnCode ReturnCode_FileNotFound = ERROR_FILE_NOT_FOUND;

void Platform_Init(void) {
	heap = GetProcessHeap(); /* TODO: HeapCreate instead? probably not */
	hdc = CreateCompatibleDC(NULL);
	if (hdc == NULL) ErrorHandler_Fail("Failed to get screen DC");

	stopwatch_highResolution = QueryPerformanceFrequency(&stopwatch_freq);

	UInt32 deviceNum = 0;
	/* Get available video adapters and enumerate all monitors */
	DISPLAY_DEVICEA device = { 0 };
	device.cb = sizeof(DISPLAY_DEVICEA);

	while (EnumDisplayDevicesA(NULL, deviceNum, &device, 0)) {
		deviceNum++;
		if ((device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) continue;
		bool devPrimary = false;
		DisplayResolution resolution = DisplayResolution_Make(0, 0, 0, 0.0f);
		DEVMODEA mode = { 0 };
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

void Platform_Exit(ReturnCode code) {
	ExitProcess(code);
}

void* Platform_MemAlloc(UInt32 numBytes) {
	return malloc(numBytes);
	//return HeapAlloc(heap, 0, numBytes);
}

void* Platform_MemRealloc(void* mem, UInt32 numBytes) {
	return realloc(mem, numBytes);
	//return HeapReAlloc(heap, 0, mem, numBytes);
}

void Platform_MemFree(void** mem) {
	if (mem == NULL || *mem == NULL) return;
	free(*mem);
	//HeapFree(heap, 0, *mem);
	*mem = NULL;
}

void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes) {
	memset(dst, value, numBytes);
}

void Platform_MemCpy(void* dst, void* src, UInt32 numBytes) {
	memcpy(dst, src, numBytes);
}


void Platform_Log(STRING_PURE String* message) {
	/* TODO: log to console */
	OutputDebugStringA(message->buffer);
}

void Platform_LogConst(const UInt8* message) {
	/* TODO: log to console */
	OutputDebugStringA(message);
}

void Platform_FromSysTime(DateTime* time, SYSTEMTIME* sysTime) {
	time->Year   = sysTime->wYear;
	time->Month  = (UInt8)sysTime->wMonth;
	time->Day    = (UInt8)sysTime->wDay;
	time->Hour   = (UInt8)sysTime->wHour;
	time->Minute = (UInt8)sysTime->wMinute;
	time->Second = (UInt8)sysTime->wSecond;
	time->Milli  = sysTime->wMilliseconds;
	return time;
}

void Platform_CurrentUTCTime(DateTime* time) {
	SYSTEMTIME utcTime;
	GetSystemTime(&utcTime);
	Platform_FromSysTime(time, &utcTime);
}

void Platform_CurrentLocalTime(DateTime* time) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	Platform_FromSysTime(time, &localTime);
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

ReturnCode Platform_EnumFiles(STRING_PURE String* path, void* obj, Platform_EnumFilesCallback callback) {
	/* Need to do directory\* to search for files in directory */
	UInt8 searchPatternBuffer[FILENAME_SIZE + 10];
	String searchPattern = String_InitAndClearArray(searchPatternBuffer);
	String_AppendString(&searchPattern, path);
	String_AppendConst(&searchPattern, "\\*");

	WIN32_FIND_DATAA data;
	HANDLE find = FindFirstFileA(searchPattern.buffer, &data);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		String path = String_FromRawArray(data.cFileName);
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			/* folder1/folder2/entry.zip --> entry.zip */
			Int32 lastDir = String_LastIndexOf(&path, Platform_DirectorySeparator);
			String filename = path;
			if (lastDir >= 0) {
				filename = String_UNSAFE_SubstringAt(&filename, lastDir + 1);
			}
			callback(&filename, obj);
		}
	} while (FindNextFileA(find, &data));

	/* get return code from FindNextFile before closing find handle */
	ReturnCode code = GetLastError();
	FindClose(find);
	return code == ERROR_NO_MORE_FILES ? 0 : code;
}

ReturnCode Platform_FileGetWriteTime(STRING_PURE String* path, DateTime* time) {
	void* file;
	ReturnCode result = Platform_FileOpen(&file, path);
	if (result != 0) return result;

	FILETIME writeTime;
	if (GetFileTime(file, NULL, NULL, &writeTime)) {
		SYSTEMTIME sysTime;
		FileTimeToSystemTime(&writeTime, &sysTime);
		Platform_FromSysTime(time, &sysTime);
	} else {
		Platform_MemSet(time, 0, sizeof(DateTime));
		result = GetLastError();
	}

	Platform_FileClose(file);
	return result;
}


ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path) {
	HANDLE handle = CreateFileA(path->buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path) {
	HANDLE handle = CreateFileA(path->buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileAppend(void** file, STRING_PURE String* path) {
	HANDLE handle = CreateFileA(path->buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	if (handle == INVALID_HANDLE_VALUE) return GetLastError();
	return Platform_FileSeek(*file, 0, STREAM_SEEKFROM_END);
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
		pos = SetFilePointer(file, offset, NULL, FILE_BEGIN); break;
	case STREAM_SEEKFROM_CURRENT:
		pos = SetFilePointer(file, offset, NULL, FILE_CURRENT); break;
	case STREAM_SEEKFROM_END:
		pos = SetFilePointer(file, offset, NULL, FILE_END); break;
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

DWORD WINAPI Platform_ThreadStartCallback(LPVOID lpParam) {
	Platform_ThreadFunc* func = (Platform_ThreadFunc*)lpParam;
	(*func)();
	return 0;
}

void* Platform_ThreadStart(Platform_ThreadFunc* func) {
	void* handle = CreateThread(NULL, 0, Platform_ThreadStartCallback, func, 0, NULL);
	if (handle == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Creating thread");
	}
	return handle;
}

void Platform_ThreadFreeHandle(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Freeing thread handle");
	}
}

void Stopwatch_Start(Stopwatch* timer) {
	if (stopwatch_highResolution) {
		QueryPerformanceCounter(timer);
	} else {
		GetSystemTimeAsFileTime(timer);
	}
}

/* TODO: check this is actually accurate */
Int32 Stopwatch_ElapsedMicroseconds(Stopwatch* timer) {
	Int64 start = *timer, end;
	if (stopwatch_highResolution) {
		QueryPerformanceCounter(&end);
		return (Int32)(((end - start) * 1000 * 1000) / stopwatch_freq.QuadPart);
	} else {
		GetSystemTimeAsFileTime(&end);
		return (Int32)((end - start) / 10);
	}
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

bool bmpAssociated;
HBITMAP hbmp;
Bitmap* bmp;

void Platform_AssociateBitmap(void) {
	if (bmpAssociated) return;
	bmpAssociated = true;
	/* TODO: Should we be using CreateDIBitmap here? */
	hbmp = CreateBitmap(bmp->Width, bmp->Height, 1, 32, bmp->Scan0);

	if (hbmp == NULL) ErrorHandler_Fail("Creating bitmap handle failed");
	if (!SelectObject(hdc, hbmp)) ErrorHandler_Fail("Selecting bitmap handle");
}

void Platform_SetBitmap(Bitmap* bmpNew) {
	/* Defer creating bitmap until necessary */
	bmp = bmpNew;
}

void Platform_ReleaseBitmap(void) {
	if (bmpAssociated) {
		if (!DeleteObject(hbmp)) ErrorHandler_Fail("Deleting bitmap handle failed");
		hbmp = NULL;
	}

	bmpAssociated = false;
	bmp = NULL;
}

/* TODO: Associate Font with device */
/* TODO: Add shadow offset for drawing */
Size2D Platform_MeasureText(DrawTextArgs* args) {
	HDC hDC = GetDC(NULL);
	RECT r = { 0 };

	HGDIOBJ oldFont = SelectObject(hDC, args->Font.Handle);
	DrawTextA(hDC, args->Text.buffer, args->Text.length,
		&r, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
	SelectObject(hDC, oldFont);

	return Size2D_Make(r.right, r.bottom);
}

void Platform_DrawText(DrawTextArgs* args, Int32 x, Int32 y) {
	HDC hDC = GetDC(NULL);
	RECT r = { 0 };

	HGDIOBJ oldFont = SelectObject(hDC, args->Font.Handle);
	DrawTextA(hDC, args->Text.buffer, args->Text.length,
		&r, DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
	SelectObject(hDC, oldFont);
}
