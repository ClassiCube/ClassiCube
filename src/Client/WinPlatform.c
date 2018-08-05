#include "Platform.h"
#if CC_BUILD_WIN
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
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <mmsystem.h>

/* Missing from some old MingW32 headers */
#define HTTP_QUERY_ETAG 54

HDC hdc;
HANDLE heap;
bool stopwatch_highResolution;
LARGE_INTEGER stopwatch_freq;

UChar* Platform_NewLine = "\r\n";
UChar Platform_DirectorySeparator = '\\';
ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
ReturnCode ReturnCode_FileNotFound = ERROR_FILE_NOT_FOUND;
ReturnCode ReturnCode_NotSupported = ERROR_NOT_SUPPORTED;
ReturnCode ReturnCode_InvalidArg   = ERROR_INVALID_PARAMETER;

void Platform_UnicodeExpand(void* dstPtr, STRING_PURE String* src) {
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");
	WCHAR* dst = dstPtr;

	Int32 i;
	for (i = 0; i < src->length; i++) {
		*dst = Convert_CP437ToUnicode(src->buffer[i]); dst++;
	}
	*dst = NULL;
}

static void Platform_InitDisplay(void) {
	HDC hdc = GetDC(NULL);
	struct DisplayDevice device = { 0 };

	device.Bounds.Width = GetSystemMetrics(SM_CXSCREEN);
	device.Bounds.Height = GetSystemMetrics(SM_CYSCREEN);
	device.BitsPerPixel = GetDeviceCaps(hdc, BITSPIXEL);
	DisplayDevice_Default = device;

	ReleaseDC(NULL, hdc);
}

void Platform_Init(void) {
	Platform_InitDisplay();
	heap = GetProcessHeap(); /* TODO: HeapCreate instead? probably not */
	hdc = CreateCompatibleDC(NULL);
	if (!hdc) ErrorHandler_Fail("Failed to get screen DC");

	SetTextColor(hdc, 0x00FFFFFF);
	SetBkColor(hdc, 0x00000000);
	SetBkMode(hdc, OPAQUE);

	stopwatch_highResolution = QueryPerformanceFrequency(&stopwatch_freq);
	WSADATA wsaData;
	ReturnCode wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	ErrorHandler_CheckOrFail(wsaResult, "WSAStartup failed");
}

void Platform_Free(void) {
	DeleteDC(hdc);
	WSACleanup();
	HeapDestroy(heap);
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
	while (args.length && args.buffer[0] == ' ') {
		args = String_UNSAFE_SubstringAt(&args, 1);
	}
	return args;
}

static void Platform_AllocFailed(const UChar* place) {
	UChar logBuffer[String_BufferSize(STRING_SIZE + 20)];
	String log = String_InitAndClearArray(logBuffer);
	String_Format1(&log, "Failed allocating memory for: %c", place);
	ErrorHandler_Fail(log.buffer);
}

void* Platform_MemAlloc(UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, 0, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Platform_MemAllocCleared(UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, HEAP_ZERO_MEMORY, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Platform_MemRealloc(void* mem, UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapReAlloc(heap, 0, mem, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void Platform_MemFree(void** mem) {
	if (mem == NULL || *mem == NULL) return;
	HeapFree(heap, 0, *mem);
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

void Platform_LogConst(const UChar* message) {
	/* TODO: log to console */
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}

void Platform_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	UChar msgBuffer[String_BufferSize(512)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(&msg);
}

void Platform_FromSysTime(DateTime* time, SYSTEMTIME* sysTime) {
	time->Year   = sysTime->wYear;
	time->Month  = sysTime->wMonth;
	time->Day    = sysTime->wDay;
	time->Hour   = sysTime->wHour;
	time->Minute = sysTime->wMinute;
	time->Second = sysTime->wSecond;
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
	WCHAR data[512]; Platform_UnicodeExpand(data, path);
	UInt32 attribs = GetFileAttributesW(data);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Platform_DirectoryExists(STRING_PURE String* path) {
	WCHAR data[512]; Platform_UnicodeExpand(data, path);
	UInt32 attribs = GetFileAttributesW(data);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Platform_DirectoryCreate(STRING_PURE String* path) {
	WCHAR data[512]; Platform_UnicodeExpand(data, path);
	BOOL success = CreateDirectoryW(data, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_EnumFiles(STRING_PURE String* path, void* obj, Platform_EnumFilesCallback callback) {
	UChar fileBuffer[String_BufferSize(MAX_PATH + 10)];
	String file = String_InitAndClearArray(fileBuffer);
	/* Need to append \* to search for files in directory */
	String_Format1(&file, "%s\\*", path);
	WCHAR data[512]; Platform_UnicodeExpand(data, &file);

	WIN32_FIND_DATAW entry;
	HANDLE find = FindFirstFileW(data, &entry);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		String_Clear(&file);
		Int32 i;

		for (i = 0; i < MAX_PATH && entry.cFileName[i] != '\0'; i++) {
			String_Append(&file, Convert_UnicodeToCP437(entry.cFileName[i]));
		}

		Utils_UNSAFE_GetFilename(&file);
		callback(&file, obj);
	}  while (FindNextFileW(find, &entry));

	ReturnCode code = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return code == ERROR_NO_MORE_FILES ? 0 : code;
}

ReturnCode Platform_FileGetModifiedTime(STRING_PURE String* path, DateTime* time) {
	void* file;
	ReturnCode result = Platform_FileOpen(&file, path);
	if (result != 0) return result;

	FILETIME writeTime;
	if (GetFileTime(file, NULL, NULL, &writeTime)) {
		SYSTEMTIME sysTime;
		FileTimeToSystemTime(&writeTime, &sysTime);
		Platform_FromSysTime(time, &sysTime);
	} else {
		result = GetLastError();
	}

	Platform_FileClose(file);
	return result;
}


ReturnCode Platform_FileDo(void** file, STRING_PURE String* path, DWORD access, DWORD createMode) {
	WCHAR data[512]; Platform_UnicodeExpand(data, path);
	*file = CreateFileW(data, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return *file != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path) {
	return Platform_FileDo(file, path, GENERIC_READ, OPEN_EXISTING);
}
ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path) {
	return Platform_FileDo(file, path, GENERIC_WRITE, CREATE_ALWAYS);
}
ReturnCode Platform_FileAppend(void** file, STRING_PURE String* path) {
	ReturnCode code = Platform_FileDo(file, path, GENERIC_WRITE, OPEN_ALWAYS);
	if (code != 0) return code;
	return Platform_FileSeek(*file, 0, STREAM_SEEKFROM_END);
}

ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead) {
	BOOL success = ReadFile((HANDLE)file, buffer, count, bytesRead, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWrote) {
	BOOL success = WriteFile((HANDLE)file, buffer, count, bytesWrote, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode Platform_FileClose(void* file) {
	return CloseHandle((HANDLE)file) ? 0 : GetLastError();
}

ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType) {
	DWORD mode = -1;
	switch (seekType) {
	case STREAM_SEEKFROM_BEGIN:   mode = FILE_BEGIN; break;
	case STREAM_SEEKFROM_CURRENT: mode = FILE_CURRENT; break;
	case STREAM_SEEKFROM_END:     mode = FILE_END; break;
	}

	DWORD pos = SetFilePointer(file, offset, NULL, mode);
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
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, Platform_ThreadStartCallback, func, 0, &threadID);
	if (!handle) {
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

CRITICAL_SECTION mutexList[3]; Int32 mutexIndex;
void* Platform_MutexCreate(void) {
	if (mutexIndex == Array_Elems(mutexList)) ErrorHandler_Fail("Cannot allocate mutex");
	CRITICAL_SECTION* ptr = &mutexList[mutexIndex];
	InitializeCriticalSection(ptr); mutexIndex++;
	return ptr;
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
	if (!handle) {
		ErrorHandler_FailWithCode(GetLastError(), "Creating event");
	}
	return handle;
}

void Platform_EventFree(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Freeing event");
	}
}

void Platform_EventSignal(void* handle) {
	SetEvent((HANDLE)handle);
}

void Platform_EventWait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Stopwatch_Measure(struct Stopwatch* timer) {
	if (stopwatch_highResolution) {
		LARGE_INTEGER value;
		QueryPerformanceCounter(&value);
		timer->Data[0] = value.QuadPart;
	} else {
		FILETIME value;
		GetSystemTimeAsFileTime(&value);
		timer->Data[0] = (Int64)value.dwLowDateTime | ((Int64)value.dwHighDateTime << 32);
	}
}
void Stopwatch_Start(struct Stopwatch* timer) { Stopwatch_Measure(timer); }

/* TODO: check this is actually accurate */
Int32 Stopwatch_ElapsedMicroseconds(struct Stopwatch* timer) {
	Int64 start = timer->Data[0];
	Stopwatch_Measure(timer);
	Int64 end = timer->Data[0];

	if (stopwatch_highResolution) {
		return (Int32)(((end - start) * 1000 * 1000) / stopwatch_freq.QuadPart);
	} else {
		return (Int32)((end - start) / 10);
	}
}

void Platform_FontMake(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style) {
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
	if (!desc->Handle) ErrorHandler_Fail("Creating font handle failed");
}

void Platform_FontFree(struct FontDesc* desc) {
	if (!DeleteObject(desc->Handle)) ErrorHandler_Fail("Deleting font handle failed");
	desc->Handle = NULL;
}

/* TODO: not associate font with device so much */
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args) {
	WCHAR data[512]; Platform_UnicodeExpand(data, &args->Text);
	HGDIOBJ oldFont = SelectObject(hdc, args->Font.Handle);
	SIZE area; GetTextExtentPointW(hdc, data, args->Text.length, &area);

	SelectObject(hdc, oldFont);
	return Size2D_Make(area.cx, area.cy);
}

HBITMAP platform_dib;
HBITMAP platform_oldBmp;
struct Bitmap* platform_bmp;
void* platform_bits;

void Platform_SetBitmap(struct Bitmap* bmp) {
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
	if (!platform_dib) ErrorHandler_Fail("Failed to allocate DIB for text");
	platform_oldBmp = SelectObject(hdc, platform_dib);
}

/* TODO: check return codes and stuff */
/* TODO: make text prettier.. somehow? */
/* TODO: Do we need to / 255 instead of >> 8 ? */
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col) {
	WCHAR strUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_UnicodeExpand(strUnicode, &args->Text);

	HGDIOBJ oldFont = (HFONT)SelectObject(hdc, (HFONT)args->Font.Handle);
	SIZE area; GetTextExtentPointW(hdc, strUnicode, args->Text.length, &area);
	TextOutW(hdc, 0, 0, strUnicode, args->Text.length);

	Int32 xx, yy;
	struct Bitmap* bmp = platform_bmp;
	for (yy = 0; yy < area.cy; yy++) {
		UInt8* src = (UInt8*)platform_bits + (yy * (bmp->Width << 2));
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

HINTERNET hInternet;
void Platform_HttpInit(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(PROGRAM_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) ErrorHandler_FailWithCode(GetLastError(), "Failed to init WinINet");
}

ReturnCode Platform_HttpMakeRequest(struct AsyncRequest* request, void** handle) {
	String url = String_FromRawArray(request->URL);
	UChar headersBuffer[String_BufferSize(STRING_SIZE * 2)];
	String headers = String_MakeNull();
	
	/* https://stackoverflow.com/questions/25308488/c-wininet-custom-http-headers */
	if (request->Etag[0] || request->LastModified.Year) {
		headers = String_InitAndClearArray(headersBuffer);
		if (request->LastModified.Year > 0) {
			String_AppendConst(&headers, "If-Modified-Since: ");
			DateTime_HttpDate(&request->LastModified, &headers);
			String_AppendConst(&headers, "\r\n");
		}

		if (request->Etag[0]) {
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
ReturnCode Platform_HttpGetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size) {
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

ReturnCode Platform_HttpGetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress) {
	if (size == 0) return ERROR_NOT_SUPPORTED;
	*data = Platform_MemAlloc(size, sizeof(UInt8), "http get data");

	*progress = 0;
	UInt8* buffer = *data;
	UInt32 left = size, read, totalRead = 0;

	while (left > 0) {
		UInt32 toRead = left, avail = 0;
		/* only read as much data that is pending */
		if (InternetQueryDataAvailable(handle, &avail, 0, NULL)) {
			toRead = min(toRead, avail);
		}

		bool success = InternetReadFile(handle, buffer, toRead, &read);
		if (!success) { Platform_MemFree(data); return GetLastError(); }

		if (!read) break;
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


struct AudioContext {
	HWAVEOUT Handle;
	WAVEHDR Headers[AUDIO_MAX_CHUNKS];
	struct AudioFormat Format;
	Int32 NumBuffers;
};
struct AudioContext Audio_Contexts[20];

void Platform_AudioInit(Int32* handle, Int32 buffers) {
	Int32 i, j;
	for (i = 0; i < Array_Elems(Audio_Contexts); i++) {
		struct AudioContext* ctx = &Audio_Contexts[i];
		if (ctx->NumBuffers) continue;
		ctx->NumBuffers = buffers;

		*handle = i;
		for (j = 0; j < buffers; j++) { 
			ctx->Headers[j].dwFlags = WHDR_DONE;
		}
		return;
	}
	ErrorHandler_Fail("No free audio contexts");
}

void Platform_AudioFree(Int32 handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	if (!ctx->Handle) return;

	ReturnCode result = waveOutClose(ctx->Handle);
	Platform_MemSet(ctx, 0, sizeof(struct AudioContext));
	ErrorHandler_CheckOrFail(result, "Audio - closing device");
}

struct AudioFormat* Platform_AudioGetFormat(Int32 handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	return &ctx->Format;
}

void Platform_AudioSetFormat(Int32 handle, struct AudioFormat* format) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	struct AudioFormat* cur = &ctx->Format;

	/* only recreate handle if we need to */
	if (AudioFormat_Eq(cur, format)) return;
	if (ctx->Handle) Platform_AudioFree(handle);

	WAVEFORMATEX fmt = { 0 };
	fmt.nChannels       = format->Channels;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.wBitsPerSample  = format->BitsPerSample;
	fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
	fmt.nSamplesPerSec  = format->Frequency;
	fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

	if (waveOutGetNumDevs() == 0u) ErrorHandler_Fail("No audio devices found");
	ReturnCode result = waveOutOpen(&ctx->Handle, WAVE_MAPPER, &fmt, NULL, NULL, CALLBACK_NULL);
	ErrorHandler_CheckOrFail(result, "Audio - opening device");
}

void Platform_AudioPlayData(Int32 handle, Int32 idx, void* data, UInt32 dataSize) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];
	Platform_MemSet(hdr, 0, sizeof(WAVEHDR));

	hdr->lpData         = data;
	hdr->dwBufferLength = dataSize;
	hdr->dwLoops        = 1;

	ReturnCode result = waveOutPrepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR));
	ErrorHandler_CheckOrFail(result, "Audio - prepare header");
	result = waveOutWrite(ctx->Handle, hdr, sizeof(WAVEHDR));
	ErrorHandler_CheckOrFail(result, "Audio - write header");
}

bool Platform_AudioIsCompleted(Int32 handle, Int32 idx) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];
	if (!(hdr->dwFlags & WHDR_DONE)) return false;

	if (hdr->dwFlags & WHDR_PREPARED) {
		ReturnCode result = waveOutUnprepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR));
		ErrorHandler_CheckOrFail(result, "Audio - unprepare header");
	}
	return true;
}

bool Platform_AudioIsFinished(Int32 handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	Int32 i;
	for (i = 0; i < ctx->NumBuffers; i++) {
		if (!Platform_AudioIsCompleted(handle, i)) return false;
	}
	return true;
}
#endif
