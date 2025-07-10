#include "../../src/Core.h"
#include "../../src/_PlatformBase.h"
#include "../../src/Stream.h"
#include "../../src/SystemFonts.h"
#include "../../src/Funcs.h"
#include "../../src/Utils.h"
#include "../../src/Errors.h"

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>
#include <wincrypt.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::System;

static HANDLE heap;
const cc_result ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = ERROR_FILE_NOT_FOUND;
const cc_result ReturnCode_DirectoryExists  = ERROR_ALREADY_EXISTS;
const cc_result ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = WSAECONNRESET;

const char* Platform_AppNameSuffix = "";
cc_bool Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS;
#define UWP_STRING(str) ((wchar_t*)(str)->uni)

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_Set(void*  dst, cc_uint8 value,  unsigned numBytes) { return memset( dst, value, numBytes); }
void* Mem_Copy(void* dst, const void* src, unsigned numBytes) { return memcpy( dst, src,   numBytes); }
void* Mem_Move(void* dst, const void* src, unsigned numBytes) { return memmove(dst, src,   numBytes); }

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? HeapAlloc(heap, 0, size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? HeapAlloc(heap, HEAP_ZERO_MEMORY, size) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? HeapReAlloc(heap, 0, mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) HeapFree(heap, 0, mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
/* TODO: check this is actually accurate */
static cc_uint64 sw_freqMul = 1, sw_freqDiv = 1;
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ((end - beg) * sw_freqMul) / sw_freqDiv;
}

static HANDLE conHandle;
static BOOL hasDebugger;

void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	DWORD wrote;

	if (conHandle) {
		WriteFile(conHandle, msg,  len, &wrote, NULL);
		WriteFile(conHandle, "\n",   1, &wrote, NULL);
	}

	if (!hasDebugger) return;
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';

	OutputDebugStringA(tmp);
	OutputDebugStringA("\n");
}

#define FILETIME_EPOCH      50491123200ULL
#define FILETIME_UNIX_EPOCH 11644473600ULL
#define FileTime_TotalSecs(time) ((time / 10000000) + FILETIME_EPOCH)
#define FileTime_UnixTime(time)  ((time / 10000000) - FILETIME_UNIX_EPOCH)

TimeMS DateTime_CurrentUTC(void) {
	FILETIME ft; 
	cc_uint64 raw;
	
	GetSystemTimeAsFileTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	raw = ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
	return FileTime_TotalSecs(raw);
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	t->year   = localTime.wYear;
	t->month  = localTime.wMonth;
	t->day    = localTime.wDay;
	t->hour   = localTime.wHour;
	t->minute = localTime.wMinute;
	t->second = localTime.wSecond;
}

static cc_bool sw_highRes;
cc_uint64 Stopwatch_Measure(void) {
	LARGE_INTEGER t;
	FILETIME ft;

	if (sw_highRes) {
		QueryPerformanceCounter(&t);
		return (cc_uint64)t.QuadPart;
	} else {		
		GetSystemTimeAsFileTime(&ft);
		return (cc_uint64)ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
static const char* ExceptionDescribe(cc_uint32 code) {
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:    return "ACCESS_VIOLATION";
	case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:  return "DIVIDE_BY_ZERO";
	}
	return NULL;
}

static LONG WINAPI UnhandledFilter(struct _EXCEPTION_POINTERS* info) {
	cc_string msg; char msgBuffer[128 + 1];
	const char* desc;
	cc_uint32 code;
	cc_uintptr addr;

	code =  (cc_uint32)info->ExceptionRecord->ExceptionCode;
	addr = (cc_uintptr)info->ExceptionRecord->ExceptionAddress;
	desc = ExceptionDescribe(code);

	String_InitArray_NT(msg, msgBuffer);
	if (desc) {
		String_Format2(&msg, "Unhandled %c error at %x", desc, &addr);
	} else {
		String_Format2(&msg, "Unhandled exception 0x%h at %x", &code, &addr);
	}

	DWORD numArgs = info->ExceptionRecord->NumberParameters;
	if (numArgs) {
		numArgs = min(numArgs, EXCEPTION_MAXIMUM_PARAMETERS);
		String_AppendConst(&msg, " [");

		for (DWORD i = 0; i < numArgs; i++) 
		{
			String_Format1(&msg, "0x%x,", &info->ExceptionRecord->ExceptionInformation[i]);
		}
		String_Append(&msg, ']');
	}

	msg.buffer[msg.length] = '\0';
	Logger_DoAbort(0, msg.buffer, info->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void CrashHandler_Install(void) {
	SetUnhandledExceptionFilter(UnhandledFilter);
}

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }

void Platform_EncodePath(cc_filepath* dst, const cc_string* src) {
	Platform_EncodeString(dst, src);
}

cc_result Directory_Create(const cc_filepath* path) {
	if (CreateDirectoryW(UWP_STRING(path), NULL)) return 0;
	return GetLastError();
}

int File_Exists(const cc_filepath* path) {
	DWORD attribs = GetFileAttributesW(UWP_STRING(path));

	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

static cc_result Directory_EnumCore(const cc_string* dirPath, const cc_string* file, DWORD attribs,
									void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	int is_dir;
	
	/* ignore . and .. entry */
	if (file->length == 1 && file->buffer[0] == '.') return 0;
	if (file->length == 2 && file->buffer[0] == '.' && file->buffer[1] == '.') return 0;

	String_InitArray(path, pathBuffer);
	String_Format2(&path, "%s/%s", dirPath, file);

	is_dir = attribs & FILE_ATTRIBUTE_DIRECTORY;
	callback(&path, obj, is_dir);
	return 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	WIN32_FIND_DATAW eW;
	int i, ansi = false;
	cc_filepath str;
	HANDLE find;
	cc_result res;	

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_EncodePath(&str, &path);
	
	find = FindFirstFileW(UWP_STRING(&str), &eW);
	if (!find || find == INVALID_HANDLE_VALUE)
		return GetLastError();

	do {
		path.length = 0;
		for (i = 0; i < MAX_PATH && eW.cFileName[i]; i++) {
			/* TODO: UTF16 to codepoint conversion */
			String_Append(&path, Convert_CodepointToCP437(eW.cFileName[i]));
		}
		if ((res = Directory_EnumCore(dirPath, &path, eW.dwFileAttributes, obj, callback))) return res;
	} while (FindNextFileW(find, &eW));

	res = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return res == ERROR_NO_MORE_FILES ? 0 : res;
}

static cc_result DoFile(cc_file* file, const cc_filepath* path, DWORD access, DWORD createMode) {
	*file = CreateFile2(UWP_STRING(path), access, FILE_SHARE_READ, createMode, NULL);
	if (*file && *file != INVALID_HANDLE_VALUE) return 0;
	return GetLastError();
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return DoFile(file, path, GENERIC_READ, OPEN_EXISTING);
}
cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return DoFile(file, path, GENERIC_WRITE | GENERIC_READ, CREATE_ALWAYS);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return DoFile(file, path, GENERIC_WRITE | GENERIC_READ, OPEN_ALWAYS);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	/* NOTE: the (DWORD*) cast assumes that sizeof(long) is 4 */
	BOOL success = ReadFile(file, data, count, (DWORD*)bytesRead, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	/* NOTE: the (DWORD*) cast assumes that sizeof(long) is 4 */
	BOOL success = WriteFile(file, data, count, (DWORD*)bytesWrote, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Close(cc_file file) {
	return CloseHandle(file) ? 0 : GetLastError();
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
	DWORD pos = SetFilePointer(file, offset, NULL, modes[seekType]);
	return pos != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	*pos = SetFilePointer(file, 0, NULL, FILE_CURRENT);
	return *pos != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	LARGE_INTEGER raw;
	if (!GetFileSizeEx(file, &raw)) return GetLastError();

	*len = raw.QuadPart;
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#############################################################################################################p############*/
void Thread_Sleep(cc_uint32 milliseconds) { Sleep(milliseconds); }
static DWORD WINAPI ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	func();
	return 0;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	DWORD threadID;
	HANDLE thread = CreateThread(NULL, 0, ExecThread, (void*)func, CREATE_SUSPENDED, &threadID);
	if (!thread) Process_Abort2(GetLastError(), "Creating thread");
	
	*handle = thread;
	ResumeThread(thread);
}

void Thread_Detach(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		Process_Abort2(GetLastError(), "Freeing thread handle");
	}
}

void Thread_Join(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
	Thread_Detach(handle);
}

void* Mutex_Create(const char* name) {
	CRITICAL_SECTION* ptr = (CRITICAL_SECTION*)Mem_Alloc(1, sizeof(CRITICAL_SECTION), "mutex");
	InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(void* handle)   { 
	DeleteCriticalSection((CRITICAL_SECTION*)handle); 
	Mem_Free(handle);
}
void Mutex_Lock(void* handle)   { EnterCriticalSection((CRITICAL_SECTION*)handle); }
void Mutex_Unlock(void* handle) { LeaveCriticalSection((CRITICAL_SECTION*)handle); }

void* Waitable_Create(const char* name) {
	void* handle = CreateEventA(NULL, false, false, NULL);
	if (!handle) {
		Process_Abort2(GetLastError(), "Creating waitable");
	}
	return handle;
}

void Waitable_Free(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		Process_Abort2(GetLastError(), "Freeing waitable");
	}
}

void Waitable_Signal(void* handle) { SetEvent((HANDLE)handle); }
void Waitable_Wait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	WaitForSingleObject((HANDLE)handle, milliseconds);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
/* Sanity check to ensure cc_sockaddr struct is large enough to contain all socket addresses supported by this platform */
static char sockaddr_size_check[sizeof(SOCKADDR_STORAGE) < CC_SOCKETADDR_MAXSIZE ? 1 : -1];

static cc_result ParseHostNew(char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	char portRaw[32]; cc_string portStr;
	struct addrinfo hints = { 0 };
	struct addrinfo* result;
	struct addrinfo* cur;
	int i = 0;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	String_InitArray(portStr,  portRaw);
	String_AppendInt(&portStr, port);
	portRaw[portStr.length] = '\0';

	int res = getaddrinfo(host, portRaw, &hints, &result);
	if (res == WSAHOST_NOT_FOUND) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	/* Prefer IPv4 addresses first */
	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next) 
	{
		if (cur->ai_family != AF_INET) continue;
		SocketAddr_Set(&addrs[i], cur->ai_addr, cur->ai_addrlen); i++;
	}
	
	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next) 
	{
		if (cur->ai_family == AF_INET) continue;
		SocketAddr_Set(&addrs[i], cur->ai_addr, cur->ai_addrlen); i++;
	}

	freeaddrinfo(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	SOCKADDR_IN*  addr4 = (SOCKADDR_IN* )addrs[0].data;
	SOCKADDR_IN6* addr6 = (SOCKADDR_IN6*)addrs[0].data;
	cc_winstring str;
	INT size;

	*numValidAddrs = 0;
	Platform_EncodeString(&str, address);

	size = sizeof(*addr4);
	if (!WSAStringToAddressW(UWP_STRING(&str), AF_INET,  NULL, (SOCKADDR*)addr4, &size)) {
		addr4->sin_port  = htons(port);

		addrs[0].size  = size;
		*numValidAddrs = 1;
		return 0;
	}

	size = sizeof(*addr6);
	if (!WSAStringToAddressW(UWP_STRING(&str), AF_INET6, NULL, (SOCKADDR*)addr6, &size)) {
		addr6->sin6_port = htons(port);

		addrs[0].size  = size;
		*numValidAddrs = 1;
		return 0;
	}

	return ParseHostNew(str.ansi, port, addrs, numValidAddrs);
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	SOCKADDR* raw_addr = (SOCKADDR*)addr->data;

	*s = socket(raw_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return WSAGetLastError();

	if (nonblocking) {
		u_long blockingMode = -1; /* non-blocking mode */
		ioctlsocket(*s, FIONBIO, &blockingMode);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	SOCKADDR* raw_addr = (SOCKADDR*)addr->data;

	int res = connect(s, raw_addr, addr->size);
	return res == -1 ? WSAGetLastError() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = recv(s, (char*)data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return WSAGetLastError();
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = send(s, (const char*)data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return WSAGetLastError();
}

void Socket_Close(cc_socket s) {
	shutdown(s, SD_BOTH);
	closesocket(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	set.fd_count    = 1;
	set.fd_array[0] = s;

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return WSAGetLastError(); }

	*success = set.fd_count != 0; return 0;
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	int resultSize = sizeof(cc_result);
	cc_result res  = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	if (res || *writable) return res;

	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&res, &resultSize);
	return res;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = true;

static cc_result Process_RawGetExePath(cc_winstring* path, int* len) {
	*len = GetModuleFileNameW(NULL, UWP_STRING(path), NATIVE_STR_LEN);
	path->uni[*len]  = '\0';
	if (*len) return 0;

	return GetLastError();
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	union STARTUPINFO_union {
		STARTUPINFOW wide;
		STARTUPINFOA ansi;
	} si = { 0 }; // less compiler warnings this way
	
	cc_winstring path;
	cc_string argv; char argvBuffer[NATIVE_STR_LEN];
	PROCESS_INFORMATION pi = { 0 };
	cc_winstring raw;
	cc_result res;
	int len, i;

	if (Platform_IsSingleProcess()) return SetGameArgs(args, numArgs);

	if ((res = Process_RawGetExePath(&path, &len))) return res;
	si.wide.cb = sizeof(STARTUPINFOW);
	
	String_InitArray(argv, argvBuffer);
	/* Game doesn't actually care about argv[0] */
	String_AppendConst(&argv, "cc");
	for (i = 0; i < numArgs; i++)
	{
		if (String_IndexOf(&args[i], ' ') >= 0) {
			String_Format1(&argv, " \"%s\"", &args[i]);
		} else {
			String_Format1(&argv, " %s",     &args[i]);
		}
	}
	Platform_EncodeString(&raw, &argv);

	if (!CreateProcessW(UWP_STRING(&path), UWP_STRING(&raw), NULL, NULL,
				false, 0, NULL, NULL, &si.wide, &pi)) return GetLastError();

	/* Don't leak memory for process return code */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

void Process_Exit(cc_result code) { ExitProcess(code); }
cc_result Process_StartOpen(const cc_string* args) {
	cc_winstring raw;
	Platform_EncodeString(&raw, args);

	auto str = hstring(UWP_STRING(&raw));
	auto uri = Uri(str);

	auto options = Windows::System::LauncherOptions();
	options.TreatAsUntrusted(true);
	Windows::System::Launcher::LaunchUriAsync(uri, options);
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
#define UPDATE_TMP TEXT("CC_prev.exe")
#define UPDATE_SRC TEXT(UPDATE_FILE)
cc_bool Updater_Supported = true;

#if defined _M_IX86
const struct UpdaterInfo Updater_Info = {
	"&eDirect3D 9 is recommended", 2,
	{
		{ "Direct3D9", "ClassiCube.exe" },
		{ "OpenGL",    "ClassiCube.opengl.exe" }
	}
};
#elif defined _M_X64
const struct UpdaterInfo Updater_Info = {
	"&eDirect3D 9 is recommended", 2,
	{
		{ "Direct3D9", "ClassiCube.64.exe" },
		{ "OpenGL",    "ClassiCube.64-opengl.exe" }
	}
};
#elif defined _M_ARM64
const struct UpdaterInfo Updater_Info = { "", 1, { { "Direct3D11", "cc-arm64-d3d11.exe" } } };
#elif defined _M_ARM
const struct UpdaterInfo Updater_Info = { "", 1, { { "Direct3D11", "cc-arm32-d3d11.exe" } } };
#else
const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
#endif

cc_bool Updater_Clean(void) {
	return DeleteFile(UPDATE_TMP) || GetLastError() == ERROR_FILE_NOT_FOUND;
}

cc_result Updater_Start(const char** action) {
	cc_winstring path;
	cc_result res;
	int len;

	*action = "Getting executable path";
	if ((res = Process_RawGetExePath(&path, &len))) return res;

	*action = "Moving executable to CC_prev.exe"; 
	if (!path.uni[0]) return ERR_NOT_SUPPORTED; /* MoveFileA returns ERROR_ACCESS_DENIED on Win 9x anyways */
	if (!MoveFileExW(UWP_STRING(&path), UPDATE_TMP, MOVEFILE_REPLACE_EXISTING)) return GetLastError();

	*action = "Replacing executable";
	if (!MoveFileExW(UPDATE_SRC, UWP_STRING(&path), MOVEFILE_REPLACE_EXISTING)) return GetLastError();

	*action = "Restarting game";
	return Process_StartGame2(NULL, 0);
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	cc_winstring path;
	cc_file file;
	FILETIME ft;
	cc_uint64 raw;
	cc_result res;
	int len;

	if ((res = Process_RawGetExePath(&path, &len))) return res;
	if ((res = File_Open(&file, &path)))            return res;

	if (GetFileTime(file, NULL, NULL, &ft)) {
		raw        = ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
		*timestamp = FileTime_UnixTime(raw);
	} else {
		res = GetLastError();
	}

	File_Close(file);
	return res;
}

/* Don't need special execute permission on windows */
cc_result Updater_MarkExecutable(void) { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	static const cc_string path = String_FromConst(UPDATE_FILE);
	cc_filepath str;
	cc_file file;
	FILETIME ft;
	cc_uint64 raw;
	cc_result res;
	
	Platform_EncodePath(&str, &path);
	res = File_OpenOrCreate(&file, &str);
	if (res) return res;

	raw = 10000000 * (timestamp + FILETIME_UNIX_EPOCH);
	ft.dwLowDateTime  = (cc_uint32)raw;
	ft.dwHighDateTime = (cc_uint32)(raw >> 32);

	if (!SetFileTime(file, NULL, NULL, &ft)) res = GetLastError();
	File_Close(file);
	return res;
}


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
const cc_string DynamicLib_Ext = String_FromConst(".dll");
static cc_result dynamicErr;
static cc_bool loadingPlugin;

void* DynamicLib_Load2(const cc_string* path) {
	return NULL;
}

void* DynamicLib_Get2(void* lib, const char* name) {
	return NULL;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	return false;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodeString(cc_winstring* dst, const cc_string* src) {
	cc_unichar* uni;
	char* ansi;
	int i;
	if (src->length > FILENAME_SIZE) Process_Abort("String too long to expand");

	uni = dst->uni;
	for (i = 0; i < src->length; i++) 
	{
		*uni++ = Convert_CP437ToUnicode(src->buffer[i]);
	}
	*uni = '\0';

	ansi = dst->ansi;
	for (i = 0; i < src->length; i++) 
	{
		*ansi++ = (char)dst->uni[i];
	}
	*ansi = '\0';
}

static void Platform_InitStopwatch(void) {
	LARGE_INTEGER freq;
	sw_highRes = QueryPerformanceFrequency(&freq);

	if (sw_highRes) {
		sw_freqMul = 1000 * 1000;
		sw_freqDiv = freq.QuadPart;
	} else { sw_freqDiv = 10; }
}

void Platform_Init(void) {
	WSADATA wsaData;
	cc_result res;

	Platform_InitStopwatch();
	heap = GetProcessHeap();

	hasDebugger = IsDebuggerPresent();
	AttachConsole(-1); /* ATTACH_PARENT_PROCESS */

	conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (conHandle == INVALID_HANDLE_VALUE) conHandle = NULL;
	
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_SysWarn(res, "starting WSA");
}

void Platform_Free(void) {
	WSACleanup();
	HeapDestroy(heap);
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	WCHAR chars[NATIVE_STR_LEN];
	DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

	res = FormatMessageW(flags, NULL, res, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						 chars, NATIVE_STR_LEN, NULL);
	if (!res) return false;

	String_AppendUtf16(dst, chars, res * 2);
	return true;
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return 200;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return 200;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char** argv) {
	return 0;
}
