#include "Platform.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "AsyncDownloader.h"
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <Windows.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <WinInet.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "Wininet.lib")
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

	SetTextColor(hdc, 0x00FFFFFF);
	SetBkColor(hdc, 0x00000000);
	SetBkMode(hdc, OPAQUE);

	stopwatch_highResolution = QueryPerformanceFrequency(&stopwatch_freq);

	WSADATA wsaData;
	ReturnCode wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	ErrorHandler_CheckOrFail(wsaResult, "WSAStartup failed");

	UInt32 deviceNum = 0;
	/* Get available video adapters and enumerate all monitors */
	DISPLAY_DEVICEA device = { 0 };
	device.cb = sizeof(DISPLAY_DEVICEA);

	while (EnumDisplayDevicesA(NULL, deviceNum, &device, 0)) {
		deviceNum++;
		if ((device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) continue;
		bool devPrimary = false;
		DisplayResolution resolution = { 0 };
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
	WSACleanup();
}

void Platform_Exit(ReturnCode code) {
	ExitProcess(code);
}

STRING_PURE String Platform_GetCommandLineArgs(void) {
	String args = String_FromReadonly(GetCommandLineA());

	Int32 argsStart;
	if (args.buffer[0] == '"') {
		/* Handle path argument in full "path" form, which can include spaces */
		argsStart = String_IndexOf(&args, '"', 1) + 1;
	} else {
		argsStart = String_IndexOf(&args, ' ', 0) + 1;
	}

	if (argsStart == 0) argsStart = args.length;
	args = String_UNSAFE_SubstringAt(&args, argsStart);

	/* get rid of duplicate leading spaces before first arg */
	while (args.length > 0 && args.buffer[0] == ' ') {
		args = String_UNSAFE_SubstringAt(&args, 1);
	}
	return args;
}

void* Platform_MemAlloc(UInt32 numElems, UInt32 elemsSize) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	return malloc(numBytes);
	//return HeapAlloc(heap, 0, numBytes);
}

void* Platform_MemRealloc(void* mem, UInt32 numElems, UInt32 elemsSize) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
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
	OutputDebugStringA("\n");
}

void Platform_LogConst(const UInt8* message) {
	/* TODO: log to console */
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}

void Platform_Log4(const UInt8* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	UInt8 msgBuffer[String_BufferSize(512)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(&msg);
}

void Platform_FromSysTime(DateTime* time, SYSTEMTIME* sysTime) {
	time->Year   = sysTime->wYear;
	time->Month  = (UInt8)sysTime->wMonth;
	time->Day    = (UInt8)sysTime->wDay;
	time->Hour   = (UInt8)sysTime->wHour;
	time->Minute = (UInt8)sysTime->wMinute;
	time->Second = (UInt8)sysTime->wSecond;
	time->Milli  = sysTime->wMilliseconds;
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
	return CloseHandle((HANDLE)file) ? 0 : GetLastError();
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

ReturnCode Platform_FilePosition(void* file, UInt32* position) {
	*position = SetFilePointer(file, 0, NULL, 1); /* SEEK_CUR */
	return *position == INVALID_SET_FILE_POINTER ? GetLastError() : 0;
}

ReturnCode Platform_FileLength(void* file, UInt32* length) {
	*length = GetFileSize(file, NULL);
	return *length == INVALID_FILE_SIZE ? GetLastError() : 0;
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

void Platform_ThreadJoin(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Platform_ThreadFreeHandle(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Freeing thread handle");
	}
}

CRITICAL_SECTION mutexList[3];
Int32 mutexIndex;
void* Platform_MutexCreate(void) {
	if (mutexIndex == Array_Elems(mutexList)) { 
		ErrorHandler_Fail("Cannot allocate another mutex");
		return NULL;
	} else {
		CRITICAL_SECTION* ptr = &mutexList[mutexIndex];
		InitializeCriticalSection(ptr); mutexIndex++;
		return ptr;
	}
}

void Platform_MutexFree(void* handle) {
	DeleteCriticalSection((CRITICAL_SECTION*)handle);
}

void Platform_MutexLock(void* handle) {
	EnterCriticalSection((CRITICAL_SECTION*)handle);
}

void Platform_MutexUnlock(void* handle) {
	LeaveCriticalSection((CRITICAL_SECTION*)handle);
}

void* Platform_EventCreate(void) {
	void* handle = CreateEventA(NULL, false, false, NULL);
	if (handle == NULL) {
		ErrorHandler_FailWithCode(GetLastError(), "Creating event");
	}
	return handle;
}

void Platform_EventFree(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Freeing event");
	}
}

void Platform_EventSet(void* handle) {
	SetEvent((HANDLE)handle);
}

void Platform_EventWait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
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

	font.lfHeight    = -Math_CeilDiv(size * GetDeviceCaps(hdc, LOGPIXELSY), 72);
	font.lfUnderline = style == FONT_STYLE_UNDERLINE;
	font.lfWeight    = style == FONT_STYLE_BOLD ? FW_BOLD : FW_NORMAL;
	font.lfQuality   = ANTIALIASED_QUALITY; /* TODO: CLEARTYPE_QUALITY looks slightly better */

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
void* hbmp_bits;

void Platform_AssociateBitmap(void) {
	if (bmpAssociated) return;
	bmpAssociated = true;

	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bmp->Width;
	bmi.bmiHeader.biHeight = bmp->Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = Bitmap_DataSize(bmp->Width, bmp->Height);

	hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &hbmp_bits, NULL, 0x0);
	if (hbmp == NULL) ErrorHandler_Fail("Creating bitmap handle failed");
	if (!SelectObject(hdc, hbmp)) ErrorHandler_Fail("Selecting bitmap handle");

	String font = String_FromConst("Arial");
	FontDesc desc; Platform_MakeFont(&desc, &font, 12, FONT_STYLE_NORMAL);
	String text = String_FromConst("TEST");
}

void Platform_SetBitmap(Bitmap* bmpNew) {
	/* Defer creating bitmap until necessary */
	bmp = bmpNew;
}

void Platform_DrawBufferedText(void) {
	/* draw text and stuff */
	//Platform_MemCpy(bmp->Scan0, hbmp_bits, Bitmap_DataSize(bmp->Width, bmp->Height));
	//BITMAP bmp = { 0 };
	//./ReturnCode result = GetObjectA(hbmp, sizeof(BITMAP), &bmp);
}

void Platform_ReleaseBitmap(void) {
	if (bmpAssociated) {
		Platform_DrawBufferedText();
		if (!DeleteObject(hbmp)) ErrorHandler_Fail("Deleting bitmap handle failed");
		hbmp = NULL;
	}

	bmpAssociated = false;
	bmp = NULL;
}

/* TODO: not ssociate font with device so much */
Size2D Platform_MeasureText(DrawTextArgs* args) {
	String* text = &args->Text;
	HGDIOBJ oldFont = SelectObject(hdc, args->Font.Handle);
	SIZE area; GetTextExtentPointA(hdc, text->buffer, text->length, &area);

	SelectObject(hdc, oldFont);
	return Size2D_Make(area.cx, area.cy);
}

/* TODO: not allocate so much */
/* TODO: check return codes and stuff */
/* TODO: make text prettier.. somehow? */
/* TODO: Do we need to / 255 instead of >> 8 ? */
/* TODO: GetTextExtentPoint32 instead? */
/* TODO: Code page 437 - so probably need to use W variants of functions */
void Platform_DrawText(DrawTextArgs* args, Int32 x, Int32 y, PackedCol col) {
	String* text = &args->Text;
	HGDIOBJ oldFont = (HFONT)SelectObject(hdc, (HFONT)args->Font.Handle);
	SIZE area; GetTextExtentPointA(hdc, text->buffer, text->length, &area);

	void* bits = NULL;
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth       = bmp->Width;
	bmi.bmiHeader.biHeight      = -bmp->Height;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HBITMAP dib = CreateDIBSection(hdc, &bmi, 0, &bits, NULL, 0);
	HGDIOBJ oldBmp = SelectObject(hdc, dib);
	TextOutA(hdc, 0, 0, text->buffer, text->length);

	Int32 xx, yy;
	for (yy = 0; yy < area.cy; yy++) {
		UInt8* src = (UInt8*)bits + yy * (bmp->Width * 4);
		UInt8* dst = (UInt8*)Bitmap_GetRow(bmp, y + yy); dst += x * BITMAP_SIZEOF_PIXEL;

		for (xx = 0; xx < area.cx; xx++) {
			UInt8 intensity = *src, invIntensity = UInt8_MaxValue - intensity;
			dst[0] = ((col.B * intensity) >> 8) + ((dst[0] * invIntensity) >> 8);
			dst[1] = ((col.G * intensity) >> 8) + ((dst[1] * invIntensity) >> 8);
			dst[2] = ((col.R * intensity) >> 8) + ((dst[2] * invIntensity) >> 8);
			//dst[3] = ((col.A * intensity) >> 8) + ((dst[3] * invIntensity) >> 8);
			dst[3] = intensity                  + ((dst[3] * invIntensity) >> 8);
			src += BITMAP_SIZEOF_PIXEL; dst += BITMAP_SIZEOF_PIXEL;
		}
	}

	SelectObject(hdc, oldBmp);
	SelectObject(hdc, oldFont);
	DeleteObject(dib);

	//RECT r = { 0 };
	//Platform_AssociateBitmap();

	//HGDIOBJ oldFont = SelectObject(hdc, args->Font.Handle);
	//DrawTextA(hdc, args->Text.buffer, args->Text.length,
	//	&r, DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
	//SelectObject(hdc, oldFont);
}


void Platform_SocketCreate(void** socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == INVALID_SOCKET) {
		ErrorHandler_FailWithCode(WSAGetLastError(), "Failed to create socket");
	}
}

ReturnCode Platform_SocketConnect(void* socket, STRING_PURE String* ip, Int32 port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip->buffer);
	addr.sin_port = htons((UInt16)port);

	ReturnCode result = connect(socket, (SOCKADDR*)(&addr), sizeof(addr));
	return result == SOCKET_ERROR ? WSAGetLastError() : 0;
}

ReturnCode Platform_SocketRead(void* socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 recvCount = recv(socket, buffer, count, 0);
	if (recvCount == SOCKET_ERROR) {
		*modified = 0; return WSAGetLastError();
	} else {
		*modified = recvCount; return 0;
	}
}

ReturnCode Platform_SocketWrite(void* socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 sentCount = send(socket, buffer, count, 0);
	if (sentCount == SOCKET_ERROR) {
		*modified = 0; return WSAGetLastError();
	} else {
		*modified = sentCount; return 0;
	}
}

ReturnCode Platform_SocketClose(void* socket) {
	ReturnCode result = 0;
	ReturnCode result1 = shutdown(socket, SD_BOTH);
	if (result1 == SOCKET_ERROR) result = WSAGetLastError();

	ReturnCode result2 = closesocket(socket);
	if (result2 == SOCKET_ERROR) result = WSAGetLastError();
	return result;
}

ReturnCode Platform_SocketAvailable(void* socket, UInt32* available) {
	return ioctlsocket(socket, FIONREAD, available);
}

ReturnCode Platform_SocketSelectRead(void* socket, Int32 microseconds, bool* success) {
	void* args[2];
	args[0] = (void*)1;
	args[1] = socket;

	TIMEVAL time;
	time.tv_usec = microseconds % (1000 * 1000);
	time.tv_sec  = microseconds / (1000 * 1000);

	Int32 selectCount = select(1, &args, NULL, NULL, &time);
	if (selectCount == SOCKET_ERROR) {
		*success = false; return WSAGetLastError();
	} else {
		*success = args[0] != 0; return 0;
	}
}

HINTERNET hInternet;
void Platform_HttpInit(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(PROGRAM_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL) ErrorHandler_FailWithCode(GetLastError(), "Failed to init WinINet");
}

ReturnCode Platform_HttpMakeRequest(AsyncRequest* request, void** handle) {
	String url = String_FromRawArray(request->URL);
	*handle = InternetOpenUrlA(hInternet, url.buffer, NULL, 0, 
		INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, NULL);
	return *handle == NULL ? GetLastError() : 0;
}

ReturnCode Platform_HttpGetRequestData(AsyncRequest* request, void* handle) {

}

ReturnCode Platform_HttpFreeRequest(void* handle) {
	return InternetCloseHandle(handle) ? 0 : GetLastError();
}

ReturnCode Platform_HttpFree(void) {
	return InternetCloseHandle(hInternet) ? 0 : GetLastError();
}