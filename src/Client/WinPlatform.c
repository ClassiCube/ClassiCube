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
	DISPLAY_DEVICEW device = { 0 };
	device.cb = sizeof(DISPLAY_DEVICEW);

	while (EnumDisplayDevicesW(NULL, deviceNum, &device, 0)) {
		deviceNum++;
		if ((device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) continue;
		bool devPrimary = false;
		DisplayResolution resolution = { 0 };
		DEVMODEW mode = { 0 };
		mode.dmSize = sizeof(DEVMODEW);

		/* The second function should only be executed when the first one fails (e.g. when the monitor is disabled) */
		if (EnumDisplaySettingsW(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode) ||
			EnumDisplaySettingsW(device.DeviceName, ENUM_REGISTRY_SETTINGS, &mode)) {
			if (mode.dmBitsPerPel > 0) {
				resolution = DisplayResolution_Make(mode.dmPelsWidth, mode.dmPelsHeight,
					mode.dmBitsPerPel, mode.dmDisplayFrequency);
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
	DeleteDC(hdc);
	WSACleanup();
	HeapDestroy(heap);
}

void Platform_Exit(ReturnCode code) {
	ExitProcess(code);
}

static void Platform_UnicodeExpand(WCHAR* dst, STRING_PURE String* src) {
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");

	Int32 i;
	for (i = 0; i < src->length; i++) {
		dst[i] = Convert_CP437ToUnicode(src->buffer[i]);
	}
	dst[i] = NULL;
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
	WCHAR pathUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(pathUnicode, path);

	UInt32 attribs = GetFileAttributesW(pathUnicode);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Platform_DirectoryExists(STRING_PURE String* path) {
	WCHAR pathUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(pathUnicode, path);

	UInt32 attribs = GetFileAttributesW(pathUnicode);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Platform_DirectoryCreate(STRING_PURE String* path) {
	WCHAR pathUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(pathUnicode, path);

	BOOL success = CreateDirectoryW(pathUnicode, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_EnumFiles(STRING_PURE String* path, void* obj, Platform_EnumFilesCallback callback) {
	/* Need to do directory\* to search for files in directory */
	UInt8 searchPatternBuffer[FILENAME_SIZE + 10];
	String searchPattern = String_InitAndClearArray(searchPatternBuffer);
	String_Format1(&searchPattern, "%s\\*", path);

	WCHAR searchUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(searchUnicode, &searchPattern);

	WIN32_FIND_DATAW data;
	HANDLE find = FindFirstFileW(searchUnicode, &data);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		String path = String_Init((UInt8*)data.cFileName, 0, MAX_PATH);
		Int32 i;

		/* unicode to code page 437*/
		for (i = 0; i < MAX_PATH && data.cFileName[i] != NULL; i++) {
			path.buffer[i] = Convert_UnicodeToCP437(data.cFileName[i]);
		}
		path.length = i;

		/* folder1/folder2/entry.zip --> entry.zip */
		Int32 lastDir = String_LastIndexOf(&path, Platform_DirectorySeparator);
		String filename = path;
		if (lastDir >= 0) {
			filename = String_UNSAFE_SubstringAt(&filename, lastDir + 1);
		}
		callback(&filename, obj);
	}  while (FindNextFileW(find, &data));

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
	WCHAR pathUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(pathUnicode, path);

	HANDLE handle = CreateFileW(pathUnicode, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;
	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path) {
	WCHAR pathUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(pathUnicode, path);

	HANDLE handle = CreateFileW(pathUnicode, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;
	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileAppend(void** file, STRING_PURE String* path) {
	WCHAR pathUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(pathUnicode, path);

	HANDLE handle = CreateFileW(pathUnicode, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
	void* handle = CreateEventW(NULL, false, false, NULL);
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

void Platform_FontMake(FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style) {
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

void Platform_FontFree(FontDesc* desc) {
	if (!DeleteObject(desc->Handle)) ErrorHandler_Fail("Deleting font handle failed");
	desc->Handle = NULL;
}

/* TODO: not associate font with device so much */
Size2D Platform_TextMeasure(DrawTextArgs* args) {
	WCHAR strUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(strUnicode, &args->Text);

	HGDIOBJ oldFont = SelectObject(hdc, args->Font.Handle);
	SIZE area; GetTextExtentPointW(hdc, strUnicode, args->Text.length, &area);

	SelectObject(hdc, oldFont);
	return Size2D_Make(area.cx, area.cy);
}

HBITMAP platform_dib;
HBITMAP platform_oldBmp;
Bitmap* platform_bmp;
void* platform_bits;

void Platform_SetBitmap(Bitmap* bmp) {
	platform_bmp = bmp;
	platform_bits = NULL;

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bmp->Width;
	bmi.bmiHeader.biHeight = -bmp->Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	platform_dib = CreateDIBSection(hdc, &bmi, 0, &platform_bits, NULL, 0);
	if (platform_dib == NULL) ErrorHandler_Fail("Failed to allocate DIB for text");
	platform_oldBmp = SelectObject(hdc, platform_dib);
}

/* TODO: check return codes and stuff */
/* TODO: make text prettier.. somehow? */
/* TODO: Do we need to / 255 instead of >> 8 ? */
Size2D Platform_TextDraw(DrawTextArgs* args, Int32 x, Int32 y, PackedCol col) {
	WCHAR strUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(strUnicode, &args->Text);

	HGDIOBJ oldFont = (HFONT)SelectObject(hdc, (HFONT)args->Font.Handle);
	SIZE area; GetTextExtentPointW(hdc, strUnicode, args->Text.length, &area);
	TextOutW(hdc, 0, 0, strUnicode, args->Text.length);

	Int32 xx, yy;
	Bitmap* bmp = platform_bmp;
	for (yy = 0; yy < area.cy; yy++) {
		UInt8* src = (UInt8*)platform_bits + (yy * bmp->Stride);
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

	SelectObject(hdc, oldFont);
	//DrawTextA(hdc, args->Text.buffer, args->Text.length,
	//	&r, DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
	return Size2D_Make(area.cx, area.cy);
}

void Platform_ReleaseBitmap(void) {
	/* TODO: Check return values */
	SelectObject(hdc, platform_oldBmp);
	DeleteObject(platform_dib);

	platform_oldBmp = NULL;
	platform_dib = NULL;
	platform_bmp = NULL;
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
	UInt8 headersBuffer[String_BufferSize(STRING_SIZE * 2)];
	String headers = String_MakeNull();
	
	/* https://stackoverflow.com/questions/25308488/c-wininet-custom-http-headers */
	if (request->Etag[0] != NULL || request->LastModified.Year > 0) {
		headers = String_InitAndClearArray(headersBuffer);
		if (request->LastModified.Year > 0) {
			String_AppendConst(&headers, "If-Modified-Since: ");
			DateTime_HttpDate(&request->LastModified, &headers);
			String_AppendConst(&headers, "\r\n");
		}

		if (request->Etag[0] != NULL) {
			String etag = String_FromRawArray(request->Etag);
			String_AppendConst(&headers, "If-None-Match: ");
			String_AppendString(&headers, &etag);
			String_AppendConst(&headers, "\r\n");
		}
		String_AppendConst(&headers, "\r\n\r\n");
	}

	*handle = InternetOpenUrlA(hInternet, url.buffer, headers.buffer, headers.length,
		INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, NULL);
	return *handle == NULL ? GetLastError() : 0;
}

/* TODO: Test last modified and etag even work */
#define Http_Query(flags, result) HttpQueryInfoA(handle, flags, result, &bufferLen, NULL)
ReturnCode Platform_HttpGetRequestHeaders(AsyncRequest* request, void* handle, UInt32* size) {
	DWORD bufferLen;

	UInt32 status;
	bufferLen = sizeof(DWORD);
	if (!Http_Query(HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status)) return GetLastError();
	request->StatusCode = status;

	bufferLen = sizeof(DWORD);
	if (!Http_Query(HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, size)) return GetLastError();

	SYSTEMTIME lastModified;
	bufferLen = sizeof(SYSTEMTIME);
	if (Http_Query(HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, &lastModified)) {
		Platform_FromSysTime(&request->LastModified, &lastModified);
	}

	String etag = String_InitAndClearArray(request->Etag);
	bufferLen = etag.capacity;
	Http_Query(HTTP_QUERY_ETAG, etag.buffer);

	return 0;
}

ReturnCode Platform_HttpGetRequestData(AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress) {
	if (size == 0) return 1;
	*data = Platform_MemAlloc(size, 1);
	if (*data == NULL) ErrorHandler_Fail("Failed to allocate memory for http get data");

	*progress = 0;
	UInt8* buffer = *data;
	UInt32 left = size, read, totalRead = 0;

	while (left > 0) {
		//UInt32 toRead = min(4096, left);
		//UInt32 avail = 0;
		//InternetQueryDataAvailable(handle, &avail, 0, NULL);
		bool success = InternetReadFile(handle, buffer, left, &read);
		if (!success) { Platform_MemFree(data); return GetLastError(); }

		if (read == 0) break;
		buffer += read; totalRead += read; left -= read;
		*progress = (Int32)(100.0f * totalRead / size);
	}

	*progress = 100;
	return 0;
}

ReturnCode Platform_HttpFreeRequest(void* handle) {
	return InternetCloseHandle(handle) ? 0 : GetLastError();
}

ReturnCode Platform_HttpFree(void) {
	return InternetCloseHandle(hInternet) ? 0 : GetLastError();
}