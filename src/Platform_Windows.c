#include "Core.h"
#if defined CC_BUILD_WIN

#include "String_.h"
#include "Stream.h"
#include "SystemFonts.h"
#include "Funcs.h"
#include "Utils.h"
#include "Errors.h"
#define OVERRIDE_MEM_FUNCTIONS

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>
/* Use own minimal versions of WinAPI headers so that compiling works on older Windows SDKs */
#include "../misc/windows/min-winsock2.h" /* #include <winsock2.h> #include <ws2tcpip.h> */
#include "../misc/windows/min-shellapi.h" /* #include <shellapi.h> */
#include "../misc/windows/min-wincrypt.h" /* #include <wincrypt.h> */
#include "../misc/windows/min-kernel32.h"

static HANDLE heap;
const cc_result ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = ERROR_FILE_NOT_FOUND;
const cc_result ReturnCode_PathNotFound     = ERROR_PATH_NOT_FOUND;
const cc_result ReturnCode_DirectoryExists  = ERROR_ALREADY_EXISTS;
const cc_result ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = WSAECONNRESET;

const char* Platform_AppNameSuffix = "";
cc_bool  Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags;
#include "_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "main_impl.h"

/* NOTE: main_real is used for when compiling with MinGW without linking to startup files. */
/*  Normally, the final code produced for "main" is our "main" combined with crt's main */
/*  (mingw-w64-crt/crt/gccmain.c) - alas this immediately crashes the game on startup. */
/* Using main_real instead and setting main_real as the entrypoint fixes the crash. */
#if defined CC_NOMAIN
int main_real(int argc, char** argv) {
#else
int main(int argc, char** argv) {
#endif
	cc_result res;
	SetupProgram(argc, argv);

	/* If single process mode, then the loop is launcher -> game -> launcher etc */
	do {
		res = RunProgram(argc, argv);
	} while (Platform_IsSingleProcess() && Window_Main.Exists);

	Window_Free();
	Process_Exit(res);
	return res;
}


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
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
	
	_GetSystemTimeAsFileTime(&ft);
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
		_GetSystemTimeAsFileTime(&ft);
		return (cc_uint64)ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
	}
}


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
/* In EXCEPTION_ACCESS_VIOLATION case, arg 1 is access type and arg 2 is virtual address */
/* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record */
#define IsNullReadException(r)  (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->ExceptionInformation[1] == 0 && r->ExceptionInformation[0] == 0) 
#define IsNullWriteException(r) (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->ExceptionInformation[1] == 0 && r->ExceptionInformation[0] == 1)

static const char* ExceptionDescribe(struct _EXCEPTION_RECORD* rec) {
	if (IsNullReadException(rec))  return "NULL_POINTER_READ";
	if (IsNullWriteException(rec)) return "NULL_POINTER_WRITE";

	switch (rec->ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:    return "ACCESS_VIOLATION";
	case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:  return "DIVIDE_BY_ZERO";
	}
	return NULL;
}

static LONG WINAPI UnhandledFilter(struct _EXCEPTION_POINTERS* info) {
	cc_string msg; char msgBuffer[128 + 1];
	struct _EXCEPTION_RECORD* rec;
	const char* desc;
	cc_uint32 code;
	cc_uintptr addr;
	DWORD i, numArgs;

	rec  = info->ExceptionRecord;
	code = (cc_uint32)rec->ExceptionCode;
	addr = (cc_uintptr)rec->ExceptionAddress;
	desc = ExceptionDescribe(rec);

	String_InitArray_NT(msg, msgBuffer);
	if (desc) {
		String_Format2(&msg, "Unhandled %c error at %x", desc, &addr);
	} else {
		String_Format2(&msg, "Unhandled exception 0x%h at %x", &code, &addr);
	}

	numArgs = rec->NumberParameters;
	if (IsNullReadException(rec) || IsNullWriteException(rec)) {
		/* Pointless to log exception arguments in this case */
	} else if (numArgs) {
		numArgs = min(numArgs, EXCEPTION_MAXIMUM_PARAMETERS);
		String_AppendConst(&msg, " [");

		for (i = 0; i < numArgs; i++) {
			String_Format1(&msg, "0x%x,", &rec->ExceptionInformation[i]);
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

#if __clang__
void __attribute__((optnone)) Process_Abort2(cc_result result, const char* raw_msg) {
#elif __GNUC__
/* Don't want compiler doing anything fancy with registers */
void __attribute__((optimize("O0"))) Process_Abort2(cc_result result, const char* raw_msg) {
#else
void Process_Abort2(cc_result result, const char* raw_msg) {
#endif
	CONTEXT ctx;
	CONTEXT* ctx_ptr;
	#if _M_IX86 && __GNUC__
	/* Stack frame layout on x86: */
	/*  [ebp] is previous frame's EBP */
	/*  [ebp+4] is previous frame's EIP (return address) */
	/*  address of [ebp+8] is previous frame's ESP */
	__asm__(
		"mov 0(%%ebp), %%eax \n\t" /* mov eax, [ebp]     */
		"mov %%eax, %0       \n\t" /* mov [ctx.Ebp], eax */
		"mov 4(%%ebp), %%eax \n\t" /* mov eax, [ebp+4]   */
		"mov %%eax, %1       \n\t" /* mov [ctx.Eip], eax */
		"lea 8(%%ebp), %%eax \n\t" /* lea eax, [ebp+8]   */
		"mov %%eax, %2"            /* mov [ctx.Esp], eax */
		: "=m" (ctx.Ebp), "=m" (ctx.Eip), "=m" (ctx.Esp)
		:
		: "eax", "memory"
	);
	ctx.ContextFlags = CONTEXT_CONTROL;
	ctx_ptr = &ctx;
	#else
	/* This method is guaranteed to exist on 64 bit windows. */
	/* NOTE: This is missing in 32 bit Windows 2000 however  */
	if (_RtlCaptureContext) {
		_RtlCaptureContext(&ctx);
		ctx_ptr = &ctx;
	} else { ctx_ptr = NULL; }
	#endif
	Logger_DoAbort(result, raw_msg, ctx_ptr);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }

void Platform_EncodePath(cc_filepath* dst, const cc_string* src) {
	Platform_EncodeString(dst, src);
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	int i;
	for (i = 0; i < FILENAME_SIZE && path->uni[i]; i++) 
	{
		String_Append(dst, Convert_CodepointToCP437(path->uni[i]));
	}
}

cc_result Directory_Create2(const cc_filepath* path) {
	cc_result res;
	if (CreateDirectoryW(path->uni, NULL)) return 0;
	/* Windows 9x does not support W API functions */
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	return CreateDirectoryA(path->ansi, NULL) ? 0 : GetLastError();
}

int File_Exists(const cc_filepath* path) {
	DWORD attribs = GetFileAttributesW(path->uni);
	
	if (attribs == INVALID_FILE_ATTRIBUTES)
		attribs = GetFileAttributesA(path->ansi);

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
	WIN32_FIND_DATAA eA;
	int i, ansi = false;
	cc_filepath str;
	HANDLE find;
	cc_result res;	

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_EncodePath(&str, &path);
	
	find = FindFirstFileW(str.uni, &eW);
	if (!find || find == INVALID_HANDLE_VALUE) {
		if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;
		ansi = true;

		/* Windows 9x does not support W API functions */
		find = FindFirstFileA(str.ansi, &eA);
		if (find == INVALID_HANDLE_VALUE) return GetLastError();
	}

	if (ansi) {
		do {
			path.length = 0;
			for (i = 0; i < MAX_PATH && eA.cFileName[i]; i++) {
				String_Append(&path, Convert_CodepointToCP437(eA.cFileName[i]));
			}
			if ((res = Directory_EnumCore(dirPath, &path, eA.dwFileAttributes, obj, callback))) return res;
		} while (FindNextFileA(find, &eA));
	} else {
		do {
			path.length = 0;
			for (i = 0; i < MAX_PATH && eW.cFileName[i]; i++) {
				/* TODO: UTF16 to codepoint conversion */
				String_Append(&path, Convert_CodepointToCP437(eW.cFileName[i]));
			}
			if ((res = Directory_EnumCore(dirPath, &path, eW.dwFileAttributes, obj, callback))) return res;
		} while (FindNextFileW(find, &eW));
	}

	res = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return res == ERROR_NO_MORE_FILES ? 0 : res;
}

static cc_result DoFile(cc_file* file, const cc_filepath* path, DWORD access, DWORD createMode) {
	cc_result res;

	*file = CreateFileW(path->uni,  access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	if (*file && *file != INVALID_HANDLE_VALUE) return 0;
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	/* Windows 9x does not support W API functions */
	*file = CreateFileA(path->ansi, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return *file != INVALID_HANDLE_VALUE ? 0 : GetLastError();
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
	*len = GetFileSize(file, NULL);
	return *len != INVALID_FILE_SIZE ? 0 : GetLastError();
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
#ifndef CC_BUILD_COOPTHREADED
	DWORD threadID;
	HANDLE thread = CreateThread(NULL, 0, ExecThread, (void*)func, CREATE_SUSPENDED, &threadID);
	if (!thread) Process_Abort2(GetLastError(), "Creating thread");
	
	*handle = thread;
	ResumeThread(thread);
#endif
}

void Thread_Detach(void* handle) {
#ifndef CC_BUILD_COOPTHREADED
	if (!CloseHandle((HANDLE)handle)) {
		Process_Abort2(GetLastError(), "Freeing thread handle");
	}
#endif
}

void Thread_Join(void* handle) {
#ifndef CC_BUILD_COOPTHREADED
	WaitForSingleObject((HANDLE)handle, INFINITE);
	Thread_Detach(handle);
#endif
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
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
#ifndef CC_BUILD_COOPTHREADED
	void* handle = CreateEventA(NULL, false, false, NULL);
	if (!handle) {
		Process_Abort2(GetLastError(), "Creating waitable");
	}
	return handle;
#else
	return NULL;
#endif
}

void Waitable_Free(void* handle) {
#ifndef CC_BUILD_COOPTHREADED
	if (!CloseHandle((HANDLE)handle)) {
		Process_Abort2(GetLastError(), "Freeing waitable");
	}
#endif
}

void Waitable_Signal(void* handle) {
#ifndef CC_BUILD_COOPTHREADED
	SetEvent((HANDLE)handle);
#endif
}

void Waitable_Wait(void* handle) {
#ifndef CC_BUILD_COOPTHREADED
	WaitForSingleObject((HANDLE)handle, INFINITE);
#endif
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
#ifndef CC_BUILD_COOPTHREADED
	WaitForSingleObject((HANDLE)handle, milliseconds);
#endif
}


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj, int isDirectory) {
	static const cc_string fonExt = String_FromConst(".fon");
	/* Completely skip windows .FON files */
	if (String_CaselessEnds(path, &fonExt)) return;
	
	if (isDirectory) {
		Directory_Enum(path, NULL, FontDirCallback);
	} else {
		SysFonts_Register(path, NULL);
	}
}

void Platform_LoadSysFonts(void) { 
	int i;
	char winFolder[FILENAME_SIZE];
	WCHAR winTmp[FILENAME_SIZE];
	UINT winLen;
	
	cc_string dirs[2];
	String_InitArray(dirs[0], winFolder);
	dirs[1] = String_FromReadonly("C:/WINNT35/system");

	/* System folder path may not be C:/Windows */
	winLen = GetWindowsDirectoryW(winTmp, FILENAME_SIZE);
	if (winLen) {
		String_AppendUtf16(&dirs[0], winTmp, winLen * 2);
	} else {
		String_AppendConst(&dirs[0], "C:/Windows");
	}
	String_AppendConst(&dirs[0], "/fonts");

	for (i = 0; i < Array_Elems(dirs); i++) 
	{
		Platform_Log1("Searching for fonts in %s", &dirs[i]);
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
/* Sanity check to ensure cc_sockaddr struct is large enough to contain all socket addresses supported by this platform */
static char sockaddr_size_check[sizeof(SOCKADDR_STORAGE) < CC_SOCKETADDR_MAXSIZE ? 1 : -1];

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addr->data;

	if (addr4->sin_family == AF_INET) 
		return IPv4_ToString(&addr4->sin_addr, &addr4->sin_port, dst);
	return false;
}

static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst) {
	SOCKADDR_IN* addr4 = (SOCKADDR_IN*)dst->data;
	cc_uint32 ip_addr;
	if (!ParseIPv4Address(ip, &ip_addr)) return false;

	addr4->sin_addr.S_un.S_addr = ip_addr;
	addr4->sin_family      = AF_INET;
	addr4->sin_port        = SockAddr_EncodePort(port);
		
	dst->size = sizeof(*addr4);
	return true;
}

static cc_bool ParseIPv6(const char* ip, int port, cc_sockaddr* dst) {
#ifdef AF_INET6
	SOCKADDR_IN6* addr6 = (SOCKADDR_IN6*)dst->data;
	INT size = sizeof(*addr6);
	if (!_WSAStringToAddressA) return false;

	if (!_WSAStringToAddressA((char*)ip, AF_INET6, NULL, addr6, &size)) {
		addr6->sin6_port = SockAddr_EncodePort(port);

		dst->size = size;
		return true;
	}
#endif
	return false;
}

static cc_result ParseHostOld(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	struct hostent* res;
	cc_result wsa_res;
	SOCKADDR_IN* addr4;
	char* src_addr;
	int i;

	res = _gethostbyname(host);

	if (!res) {
		wsa_res = _WSAGetLastError();

		if (wsa_res == WSAHOST_NOT_FOUND) return SOCK_ERR_UNKNOWN_HOST;
		return ERR_INVALID_ARGUMENT;
	}
		
	/* per MSDN, should only be getting AF_INET returned from this */
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)          return ERR_INVALID_ARGUMENT;

	for (i = 0; i < SOCKET_MAX_ADDRS; i++) 
	{
		src_addr = res->h_addr_list[i];
		if (!src_addr) break;
		addrs[i].size = sizeof(SOCKADDR_IN);

		addr4 = (SOCKADDR_IN*)addrs[i].data;
		addr4->sin_family = AF_INET;
		addr4->sin_port   = SockAddr_EncodePort(port);
		addr4->sin_addr   = *(IN_ADDR*)src_addr;
	}

	*numValidAddrs = i;
	/* Must have at least one IPv4 address */
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

static cc_result ParseHostNew(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	char portRaw[32]; cc_string portStr;
	struct addrinfo hints = { 0 };
	struct addrinfo* result;
	struct addrinfo* cur;
	int res, i = 0;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	String_InitArray(portStr,  portRaw);
	String_AppendInt(&portStr, port);
	portRaw[portStr.length] = '\0';

	res = _getaddrinfo(host, portRaw, &hints, &result);
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

	_freeaddrinfo(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	if (_getaddrinfo) {
		return ParseHostNew(host, port, addrs, numValidAddrs);
	} else {
		return ParseHostOld(host, port, addrs, numValidAddrs);
	}
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	SOCKADDR* raw_addr = (SOCKADDR*)addr->data;

	*s = _socket(raw_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return _WSAGetLastError();

	if (nonblocking) {
		u_long blockingMode = -1; /* non-blocking mode */
		_ioctlsocket(*s, FIONBIO, &blockingMode);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	SOCKADDR* raw_addr = (SOCKADDR*)addr->data;

	int res = _connect(s, raw_addr, addr->size);
	return res == -1 ? _WSAGetLastError() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = _recv(s, (char*)data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return _WSAGetLastError();
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = _send(s, (const char*)data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return _WSAGetLastError();
}

void Socket_Close(cc_socket s) {
	_shutdown(s, SD_BOTH);
	_closesocket(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set1, set2;
	struct timeval time = { 0 };
	int selectCount;

	set1.fd_count    = 1; set2.fd_count    = 1;
	set1.fd_array[0] = s; set2.fd_array[0] = s;

	/* As per https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select */
	/* A socket will be pollable (select returns true) in following cases): */
	/* - readfds: Data is available for reading, or Connection has been closed/reset/terminated. */
	/* - writefds: Non-blocking connection attempt succeeded, or data can be sent */
	/* - exceptfds: Non-blocking connection attempt failed, or OOB data is available for reading */

	if (mode == SOCKET_POLL_READ) {
		selectCount = _select(1, &set1, NULL, NULL, &time);

		*success = set1.fd_count != 0; 
	} else {
		selectCount = _select(1, NULL, &set1, &set2, &time);

		*success = set1.fd_count != 0 || set2.fd_count != 0;
	}

	if (selectCount >= 0) return 0;
	*success = false; 
	return _WSAGetLastError();
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return Socket_Poll(s, SOCKET_POLL_WRITE, writable);
}

cc_result Socket_GetLastError(cc_socket s) {
	int error   = ERR_INVALID_ARGUMENT;
	int errSize = sizeof(error);

	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	_getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error, &errSize);
	return error;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = true;

static cc_result Process_RawGetExePath(cc_winstring* path, int* len) {
	cc_result res;

	/* If GetModuleFileNameA fails.. that's a serious problem */
	*len = GetModuleFileNameA(NULL, path->ansi, NATIVE_STR_LEN);
	path->ansi[*len] = '\0';
	if (!(*len)) return GetLastError();
	
	*len = GetModuleFileNameW(NULL, path->uni, NATIVE_STR_LEN);
	path->uni[*len]  = '\0';
	if (*len) return 0;

	/* GetModuleFileNameW can fail on Win 9x */
	res = GetLastError();
	if (res == ERROR_CALL_NOT_IMPLEMENTED) res = 0;
	return res;
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

	if (path.uni[0]) {
		if (!CreateProcessW(path.uni, raw.uni, NULL, NULL,
				false, 0, NULL, NULL, &si.wide, &pi)) return GetLastError();
	} else {
		/* Windows 9x does not support W API functions */
		if (!CreateProcessA(path.ansi, raw.ansi, NULL, NULL,
				false, 0, NULL, NULL, &si.ansi, &pi)) return GetLastError();
	}

	/* Don't leak memory for process return code */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

void Process_Exit(cc_result code) { ExitProcess(code); }
cc_result Process_StartOpen(const cc_string* args) {
	cc_winstring str;
	cc_uintptr res;
	Platform_EncodeString(&str, args);

	res = (cc_uintptr)ShellExecuteW(NULL, NULL, str.uni, NULL, NULL, SW_SHOWNORMAL);
	/* Windows 9x always returns "error 0" for ShellExecuteW */
	if (res == 0) res = (cc_uintptr)ShellExecuteA(NULL, NULL, str.ansi, NULL, NULL, SW_SHOWNORMAL);

	/* MSDN: "If the function succeeds, it returns a value greater than 32. If the function fails, */
	/*  it returns an error value that indicates the cause of the failure" */
	return res > 32 ? 0 : (cc_result)res;
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
	if (!MoveFileExW(path.uni, UPDATE_TMP, MOVEFILE_REPLACE_EXISTING)) return GetLastError();

	*action = "Replacing executable";
	if (!MoveFileExW(UPDATE_SRC, path.uni, MOVEFILE_REPLACE_EXISTING)) return GetLastError();

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
	static cc_string plugins_dir = String_FromConst("plugins/");
	cc_filepath str;
	void* lib;

	Platform_EncodePath(&str, path);
	loadingPlugin = String_CaselessStarts(path, &plugins_dir);

	if ((lib = LoadLibraryW(str.uni))) return lib;
	dynamicErr = GetLastError();
	if (dynamicErr != ERROR_CALL_NOT_IMPLEMENTED) return NULL;

	/* Windows 9x only supports A variants */
	lib = LoadLibraryA(str.ansi);
	if (!lib) dynamicErr = GetLastError();
	return lib;
}

void* DynamicLib_Get2(void* lib, const char* name) {
	void* addr = GetProcAddress((HMODULE)lib, name);
	if (!addr) dynamicErr = GetLastError();
	return addr;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	cc_result res = dynamicErr;
	dynamicErr = 0; /* Reset error (match posix behaviour) */

	Platform_DescribeError(res, dst);
	String_Format1(dst, " (error %e)", &res);

	/* Plugin may have been compiled to load symbols from ClassiCube.exe, */
	/*  but the user might have renamed it to something else */
	if (res == ERROR_MOD_NOT_FOUND  && loadingPlugin) {
		String_AppendConst(dst, "\n    Make sure the ClassiCube executable is named ClassiCube.exe");
	}
	if (res == ERROR_PROC_NOT_FOUND && loadingPlugin) {
		String_AppendConst(dst, "\n    The plugin or your game may be outdated");
	}

	/* User might be trying to use 32 bit plugin with 64 bit executable, or vice versa */
	if (res == ERROR_BAD_EXE_FORMAT && loadingPlugin) {
		if (sizeof(cc_uintptr) == 4) {
			String_AppendConst(dst, "\n    Try using a 32 bit version of the plugin instead");
		} else {
			String_AppendConst(dst, "\n    Try using a 64 bit version of the plugin instead");
		}
	}
	return true;
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
	Kernel32_LoadDynamicFuncs();

	if (_IsDebuggerPresent) hasDebugger = _IsDebuggerPresent();
	/* For when user runs from command prompt */
#if CC_WIN_BACKEND != CC_WIN_BACKEND_TERMINAL
	if (_AttachConsole) _AttachConsole(-1); /* ATTACH_PARENT_PROCESS */
#endif

	conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (conHandle == INVALID_HANDLE_VALUE) conHandle = NULL;

	Winsock_LoadDynamicFuncs();

	res = _WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_SysWarn(res, "starting WSA");
}

void Platform_Free(void) {
	_WSACleanup();
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
	DATA_BLOB input, output;
	int i;
	input.cbData = len; input.pbData = (BYTE*)data;

	Crypt32_LoadDynamicFuncs();
	if (!_CryptProtectData) return ERR_NOT_SUPPORTED;
	if (!_CryptProtectData(&input, NULL, NULL, NULL, NULL, 0, &output)) return GetLastError();

	for (i = 0; i < output.cbData; i++) 
	{
		String_Append(dst, output.pbData[i]);
	}
	LocalFree(output.pbData);
	return 0;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	DATA_BLOB input, output;
	int i;
	input.cbData = len; input.pbData = (BYTE*)data;

	Crypt32_LoadDynamicFuncs();
	if (!_CryptUnprotectData) return ERR_NOT_SUPPORTED;
	if (!_CryptUnprotectData(&input, NULL, NULL, NULL, NULL, 0, &output)) return GetLastError();

	for (i = 0; i < output.cbData; i++) 
	{
		String_Append(dst, output.pbData[i]);
	}
	LocalFree(output.pbData);
	return 0;
}

static BOOL (WINAPI *_RtlGenRandom)(PVOID data, ULONG len);

cc_result Platform_GetEntropy(void* data, int len) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_ReqSym2("SystemFunction036", RtlGenRandom)
	};

	if (!_RtlGenRandom) {
		static const cc_string kernel32 = String_FromConst("ADVAPI32.DLL");
		void* lib;
		
		DynamicLib_LoadAll(&kernel32, funcs, Array_Elems(funcs), &lib);
		if (!_RtlGenRandom) return ERR_NOT_SUPPORTED;
	}
	
	if (!_RtlGenRandom(data, len)) return GetLastError();
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
static cc_string Platform_NextArg(STRING_REF cc_string* args) {
	cc_string arg;
	int end;

	/* get rid of leading spaces before arg */
	while (args->length && args->buffer[0] == ' ') {
		*args = String_UNSAFE_SubstringAt(args, 1);
	}

	if (args->length && args->buffer[0] == '"') {
		/* "xy za" is used for arg with spaces */
		*args = String_UNSAFE_SubstringAt(args, 1);
		end = String_IndexOf(args, '"');
	} else {
		end = String_IndexOf(args, ' ');
	}

	if (end == -1) {
		arg   = *args;
		args->length = 0;
	} else {
		arg   = String_UNSAFE_Substring(args, 0, end);
		*args = String_UNSAFE_SubstringAt(args, end + 1);
	}
	return arg;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	cc_string cmdArgs = String_FromReadonly(GetCommandLineA());
	int i;
	Platform_NextArg(&cmdArgs); /* skip exe path */
	if (gameHasArgs) return GetGameArgs(args);

	for (i = 0; i < GAME_MAX_CMDARGS; i++) 
	{
		args[i] = Platform_NextArg(&cmdArgs);

		if (!args[i].length) break;
	}
	return i;
}

/* Detects if the game is running in Windows directory */
/* This happens when ClassiCube is launched directly from shell process */
/*  (e.g. via clicking a search result in Windows 10 start menu) */
static cc_bool IsProblematicWorkingDirectory(void) {
	cc_string curDir, winDir;
	char curPath[2048] = { 0 };
	char winPath[2048] = { 0 };

	GetCurrentDirectoryA(2048, curPath);
	GetSystemDirectoryA(winPath, 2048);

	curDir = String_FromReadonly(curPath);
	winDir = String_FromReadonly(winPath);
	
	if (String_Equals(&curDir, &winDir)) {
		Platform_LogConst("Working directory is System32! Changing to executable directory..");
		return true;
	}
	return false;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char** argv) {
	cc_winstring path;
	int i, len;
	cc_result res;
	if (!IsProblematicWorkingDirectory()) return 0;

	res = Process_RawGetExePath(&path, &len);
	if (res) return res;
	if (!path.uni[0]) return ERR_NOT_SUPPORTED; 
	/* Not implemented on ANSI only systems due to laziness */

	/* Get rid of filename at end of directory */
	for (i = len - 1; i >= 0; i--, len--) 
	{
		if (path.uni[i] == '/' || path.uni[i] == '\\') break;
	}

	path.uni[len] = '\0';
	return SetCurrentDirectoryW(path.uni) ? 0 : GetLastError();
}
#endif
