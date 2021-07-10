#include "Core.h"
#if defined CC_BUILD_WIN

#include "_PlatformBase.h"
#include "Stream.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "Utils.h"
#include "Errors.h"

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

static HANDLE heap;
const cc_result ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = ERROR_FILE_NOT_FOUND;
const cc_result ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = ERROR_ALREADY_EXISTS;

static void LoadDynamicFuncs(const cc_string* path, const struct DynamicLibSym* syms, int count) {
	void* lib = DynamicLib_Load2(path);
	if (!lib) { Logger_DynamicLibWarn("loading", path); return; }
	DynamicLib_GetAll(lib, syms, count);
}

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void*  dst, cc_uint8 value,  cc_uint32 numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, const void* src, cc_uint32 numBytes) { memcpy(dst, src,   numBytes); }

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

#define FILETIME_EPOCH 50491123200000ULL
#define FILETIME_UNIX_EPOCH 11644473600LL
#define FileTime_TotalMS(time)  ((time / 10000)    + FILETIME_EPOCH)
#define FileTime_UnixTime(time) ((time / 10000000) - FILETIME_UNIX_EPOCH)
TimeMS DateTime_CurrentUTC_MS(void) {
	FILETIME ft; 
	cc_uint64 raw;
	
	GetSystemTimeAsFileTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	raw = ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
	return FileTime_TotalMS(raw);
}

void DateTime_CurrentLocal(struct DateTime* t) {
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
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
cc_result Directory_Create(const cc_string* path) {
	WCHAR str[NATIVE_STR_LEN];
	cc_result res;

	Platform_EncodeUtf16(str, path);
	if (CreateDirectoryW(str, NULL)) return 0;
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	Platform_Utf16ToAnsi(str);
	return CreateDirectoryA((LPCSTR)str, NULL) ? 0 : GetLastError();
}

int File_Exists(const cc_string* path) {
	WCHAR str[NATIVE_STR_LEN];
	DWORD attribs;

	Platform_EncodeUtf16(str, path);
	attribs = GetFileAttributesW(str);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

static cc_result Directory_EnumCore(const cc_string* dirPath, const cc_string* file, DWORD attribs,
									void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	/* ignore . and .. entry */
	if (file->length == 1 && file->buffer[0] == '.') return 0;
	if (file->length == 2 && file->buffer[0] == '.' && file->buffer[1] == '.') return 0;

	String_InitArray(path, pathBuffer);
	String_Format2(&path, "%s/%s", dirPath, file);

	if (attribs & FILE_ATTRIBUTE_DIRECTORY) return Directory_Enum(&path, obj, callback);
	callback(&path, obj);
	return 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	WCHAR str[NATIVE_STR_LEN];
	WIN32_FIND_DATAW eW;
	WIN32_FIND_DATAA eA;
	int i, ansi = false;
	HANDLE find;
	cc_result res;	

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_EncodeUtf16(str, &path);
	
	find = FindFirstFileW(str, &eW);
	if (!find || find == INVALID_HANDLE_VALUE) {
		if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;
		ansi = true;

		/* Windows 9x does not support W API functions */
		Platform_Utf16ToAnsi(str);
		find = FindFirstFileA((LPCSTR)str, &eA);
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

static cc_result DoFile(cc_file* file, const cc_string* path, DWORD access, DWORD createMode) {
	WCHAR str[NATIVE_STR_LEN];
	cc_result res;
	Platform_EncodeUtf16(str, path);

	*file = CreateFileW(str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	if (*file && *file != INVALID_HANDLE_VALUE) return 0;
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	/* Windows 9x does not support W API functions */
	Platform_Utf16ToAnsi(str);
	*file = CreateFileA((LPCSTR)str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return *file != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return DoFile(file, path, GENERIC_READ, OPEN_EXISTING);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	return DoFile(file, path, GENERIC_WRITE | GENERIC_READ, CREATE_ALWAYS);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return DoFile(file, path, GENERIC_WRITE | GENERIC_READ, OPEN_ALWAYS);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	BOOL success = ReadFile(file, data, count, bytesRead, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	BOOL success = WriteFile(file, data, count, bytesWrote, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Close(cc_file file) {
	return CloseHandle(file) ? 0 : GetLastError();
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
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
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { Sleep(milliseconds); }
static DWORD WINAPI ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	func();
	return 0;
}

void* Thread_Start(Thread_StartFunc func) {
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, ExecThread, (void*)func, 0, &threadID);
	if (!handle) {
		Logger_Abort2(GetLastError(), "Creating thread");
	}
	return handle;
}

void Thread_Detach(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		Logger_Abort2(GetLastError(), "Freeing thread handle");
	}
}

void Thread_Join(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
	Thread_Detach(handle);
}

void* Mutex_Create(void) {
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

void* Waitable_Create(void) {
	void* handle = CreateEventA(NULL, false, false, NULL);
	if (!handle) {
		Logger_Abort2(GetLastError(), "Creating waitable");
	}
	return handle;
}

void Waitable_Free(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		Logger_Abort2(GetLastError(), "Freeing waitable");
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
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj) {
	static const cc_string fonExt = String_FromConst(".fon");
	/* Completely skip windows .FON files */
	if (String_CaselessEnds(path, &fonExt)) return;
	SysFonts_Register(path);
}

void Platform_LoadSysFonts(void) { 
	int i;
	char winFolder[FILENAME_SIZE];
	WCHAR winTmp[FILENAME_SIZE];
	UINT winLen;
	/* System folder path may not be C:/Windows */
	cc_string dirs[1];
	String_InitArray(dirs[0], winFolder);

	winLen = GetWindowsDirectoryW(winTmp, FILENAME_SIZE);
	if (winLen) {
		String_AppendUtf16(&dirs[0], winTmp, winLen * 2);
	} else {
		String_AppendConst(&dirs[0], "C:/Windows");
	}
	String_AppendConst(&dirs[0], "/fonts");

	for (i = 0; i < Array_Elems(dirs); i++) {
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
static INT (WSAAPI *_WSAStringToAddressW)(LPWSTR addressString, INT addressFamily, LPVOID protocolInfo, LPVOID address, LPINT addressLength);

static INT WSAAPI FallbackParseAddress(LPWSTR addressString, INT addressFamily, LPVOID protocolInfo, LPVOID address, LPINT addressLength) {
	SOCKADDR_IN* addr4 = (SOCKADDR_IN*)address;
	cc_uint8*    addr  = (cc_uint8*)&addr4->sin_addr;
	cc_string ip, parts[4 + 1];
	WCHAR tmp[NATIVE_STR_LEN];

	Mem_Copy(tmp, addressString, sizeof(tmp));
	Platform_Utf16ToAnsi(tmp);
	ip = String_FromReadonly((char*)tmp);

	/* 4+1 in case user tries '1.1.1.1.1' */
	if (String_UNSAFE_Split(&ip, '.', parts, 4 + 1) != 4)
		return ERR_INVALID_ARGUMENT;

	if (!Convert_ParseUInt8(&parts[0], &addr[0]) || !Convert_ParseUInt8(&parts[1], &addr[1]) ||
		!Convert_ParseUInt8(&parts[2], &addr[2]) || !Convert_ParseUInt8(&parts[3], &addr[3]))
		return ERR_INVALID_ARGUMENT;

	addr4->sin_family = AF_INET;
	return 0;
}

static void LoadWinsockFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(WSAStringToAddressW)
	};

	static const cc_string winsock32 = String_FromConst("WS2_32.DLL");
	LoadDynamicFuncs(&winsock32, funcs, Array_Elems(funcs));
	/* Fallback for older OS versions which lack WSAStringToAddressW */
	if (!_WSAStringToAddressW) _WSAStringToAddressW = FallbackParseAddress;
}

cc_result Socket_Available(cc_socket s, int* available) {
	return ioctlsocket(s, FIONREAD, available);
}

cc_result Socket_GetError(cc_socket s, cc_result* result) {
	int resultSize = sizeof(cc_result);
	return getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)result, &resultSize);
}

static int ParseHost(void* dst, WCHAR* host, int port) {
	SOCKADDR_IN* addr4 = (SOCKADDR_IN*)dst;
	struct hostent* res;

	Platform_Utf16ToAnsi(host);
	res = gethostbyname((char*)host);
	if (!res || res->h_addrtype != AF_INET) return 0;

	/* Must have at least one IPv4 address */
	if (!res->h_addr_list[0]) return 0;

	addr4->sin_family = AF_INET;
	addr4->sin_port   = htons(port);
	addr4->sin_addr   = *(IN_ADDR*)res->h_addr_list[0];
	return AF_INET;
}

static int Socket_ParseAddress(void* dst, const cc_string* address, int port) {
	SOCKADDR_IN*  addr4 =  (SOCKADDR_IN*)dst;
	SOCKADDR_IN6* addr6 = (SOCKADDR_IN6*)dst;
	WCHAR str[NATIVE_STR_LEN];
	INT size;
	Platform_EncodeUtf16(str, address);

	size = sizeof(*addr4);
	if (!_WSAStringToAddressW(str, AF_INET, NULL, addr4, &size)) {
		addr4->sin_port  = htons(port);
		return AF_INET;
	}
	size = sizeof(*addr6);
	if (!_WSAStringToAddressW(str, AF_INET6, NULL, (SOCKADDR*)addr6, &size)) {
		addr6->sin6_port = htons(port);
		return AF_INET6;
	}
	return ParseHost(dst, str, port);
}

int Socket_ValidAddress(const cc_string* address) {
	SOCKADDR_STORAGE addr;
	return Socket_ParseAddress(&addr, address, 0);
}

cc_result Socket_Connect(cc_socket* s, const cc_string* address, int port) {
	int family, blockingMode = -1; /* non-blocking mode */
	SOCKADDR_STORAGE addr;
	cc_result res;

	*s = -1;
	if (!(family = Socket_ParseAddress(&addr, address, port)))
		return ERR_INVALID_ARGUMENT;

	*s = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return WSAGetLastError();
	ioctlsocket(*s, FIONBIO, &blockingMode);

	res = connect(*s, (SOCKADDR*)&addr, sizeof(addr));
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

cc_result Socket_Close(cc_socket s) {
	cc_result res = 0;
	cc_result res1, res2;

	res1 = shutdown(s, SD_BOTH);
	if (res1 == -1) res = WSAGetLastError();

	res2 = closesocket(s);
	if (res2 == -1) res = WSAGetLastError();
	return res;
}

cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
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


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
static cc_result Process_RawGetExePath(WCHAR* path, int* len) {
	*len = GetModuleFileNameW(NULL, path, NATIVE_STR_LEN);
	return *len ? 0 : GetLastError();
}

cc_result Process_StartGame(const cc_string* args) {
	WCHAR path[NATIVE_STR_LEN + 1], raw[NATIVE_STR_LEN];
	cc_string argv; char argvBuffer[NATIVE_STR_LEN];
	STARTUPINFOW si        = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	cc_result res;
	int len;

	Process_RawGetExePath(path, &len);
	path[len] = '\0';
	si.cb = sizeof(STARTUPINFOW);
	
	String_InitArray(argv, argvBuffer);
	/* Game doesn't actually care about argv[0] */
	String_Format1(&argv, "cc %s", args);
	String_UNSAFE_TrimEnd(&argv);
	Platform_EncodeUtf16(raw, &argv);

	if (CreateProcessW(path, raw, NULL, NULL, 
			false, 0, NULL, NULL, &si, &pi)) goto success;
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	/* Windows 9x does not support W API functions */
	len = GetModuleFileNameA(NULL, (LPSTR)path, NATIVE_STR_LEN);
	((char*)path)[len] = '\0';
	Platform_Utf16ToAnsi(raw);

	if (CreateProcessA((LPCSTR)path, (LPSTR)raw, NULL, NULL,
			false, 0, NULL, NULL, &si, &pi)) goto success;
	return GetLastError();

success:
	/* Don't leak memory for process return code */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

void Process_Exit(cc_result code) { ExitProcess(code); }
cc_result Process_StartOpen(const cc_string* args) {
	WCHAR str[NATIVE_STR_LEN];
	cc_uintptr res;
	Platform_EncodeUtf16(str, args);

	res = (cc_uintptr)ShellExecuteW(NULL, NULL, str, NULL, NULL, SW_SHOWNORMAL);
	/* MSDN: "If the function succeeds, it returns a value greater than 32. If the function fails, */
	/*  it returns an error value that indicates the cause of the failure" */
	return res > 32 ? 0 : (cc_result)res;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
#define UPDATE_TMP TEXT("CC_prev.exe")
#define UPDATE_SRC TEXT(UPDATE_FILE)

#if _WIN64
const char* const Updater_D3D9 = "ClassiCube.64.exe";
const char* const Updater_OGL  = "ClassiCube.64-opengl.exe";
#else
const char* const Updater_D3D9 = "ClassiCube.exe";
const char* const Updater_OGL  = "ClassiCube.opengl.exe";
#endif

cc_bool Updater_Clean(void) {
	return DeleteFile(UPDATE_TMP) || GetLastError() == ERROR_FILE_NOT_FOUND;
}

cc_result Updater_Start(const char** action) {
	WCHAR path[NATIVE_STR_LEN + 1];
	cc_result res;
	int len = 0;

	*action = "Getting executable path";
	if ((res = Process_RawGetExePath(path, &len))) return res;
	path[len] = '\0';

	*action = "Moving executable to CC_prev.exe";
	if (!MoveFileExW(path, UPDATE_TMP, MOVEFILE_REPLACE_EXISTING)) return GetLastError();
	*action = "Replacing executable";
	if (!MoveFileExW(UPDATE_SRC, path, MOVEFILE_REPLACE_EXISTING)) return GetLastError();

	*action = "Restarting game";
	return Process_StartGame(&String_Empty);
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	WCHAR path[NATIVE_STR_LEN + 1];
	cc_file file;
	FILETIME ft;
	cc_uint64 raw;
	int len = 0;

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';

	file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE) return GetLastError();

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
	cc_file file;
	FILETIME ft;
	cc_uint64 raw;
	cc_result res = File_OpenOrCreate(&file, &path);
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

void* DynamicLib_Load2(const cc_string* path) {
	WCHAR str[NATIVE_STR_LEN];
	void* lib;
	Platform_EncodeUtf16(str, path);

	if ((lib = LoadLibraryW(str))) return lib;
	dynamicErr = GetLastError();
	if (dynamicErr != ERROR_CALL_NOT_IMPLEMENTED) return NULL;

	/* Windows 9x only supports A variants */
	Platform_Utf16ToAnsi(str);
	lib = LoadLibraryA((char*)str);
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
	String_Format1(dst, " (error %i)", &res);
	return true;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
int Platform_EncodeUtf16(void* data, const cc_string* src) {
	WCHAR* dst = (WCHAR*)data;
	int i;
	if (src->length > FILENAME_SIZE) Logger_Abort("String too long to expand");

	for (i = 0; i < src->length; i++) {
		*dst++ = Convert_CP437ToUnicode(src->buffer[i]);
	}
	*dst = '\0';
	return src->length * 2;
}

void Platform_Utf16ToAnsi(void* data) {
	WCHAR* src = (WCHAR*)data;
	char* dst  = (char*)data;

	while (*src) { *dst++ = (char)(*src++); }
	*dst = '\0';
}

static void Platform_InitStopwatch(void) {
	LARGE_INTEGER freq;
	sw_highRes = QueryPerformanceFrequency(&freq);

	if (sw_highRes) {
		sw_freqMul = 1000 * 1000;
		sw_freqDiv = freq.QuadPart;
	} else { sw_freqDiv = 10; }
}

static BOOL (WINAPI *_AttachConsole)(DWORD processId);
static BOOL (WINAPI *_IsDebuggerPresent)(void);

static void LoadKernelFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(AttachConsole), DynamicLib_Sym(IsDebuggerPresent)
	};

	static const cc_string kernel32 = String_FromConst("KERNEL32.DLL");
	LoadDynamicFuncs(&kernel32, funcs, Array_Elems(funcs));
}

void Platform_Init(void) {
	WSADATA wsaData;
	cc_result res;

	Platform_InitStopwatch();
	heap = GetProcessHeap();
	
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_SysWarn(res, "starting WSA");

	LoadKernelFuncs();
	LoadWinsockFuncs();
	if (_IsDebuggerPresent) hasDebugger = _IsDebuggerPresent();
	/* For when user runs from command prompt */
	if (_AttachConsole) _AttachConsole(-1); /* ATTACH_PARENT_PROCESS */

	conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (conHandle == INVALID_HANDLE_VALUE) conHandle = NULL;
}

void Platform_Free(void) {
	WSACleanup();
	HeapDestroy(heap);
}

cc_bool Platform_DescribeErrorExt(cc_result res, cc_string* dst, void* lib) {
	WCHAR chars[NATIVE_STR_LEN];
	DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	if (lib) flags |= FORMAT_MESSAGE_FROM_HMODULE;

	res = FormatMessageW(flags, lib, res, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						 chars, NATIVE_STR_LEN, NULL);
	if (!res) return false;

	String_AppendUtf16(dst, chars, res * 2);
	return true;
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return Platform_DescribeErrorExt(res, dst, NULL);
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
static BOOL (WINAPI *_CryptProtectData  )(DATA_BLOB* dataIn, PCWSTR dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);
static BOOL (WINAPI *_CryptUnprotectData)(DATA_BLOB* dataIn, PWSTR* dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);

static void LoadCryptFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(CryptProtectData), DynamicLib_Sym(CryptUnprotectData)
	};

	static const cc_string crypt32 = String_FromConst("CRYPT32.DLL");
	LoadDynamicFuncs(&crypt32, funcs, Array_Elems(funcs));
}

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	DATA_BLOB input, output;
	int i;
	input.cbData = len; input.pbData = (BYTE*)data;

	if (!_CryptProtectData) LoadCryptFuncs();
	if (!_CryptProtectData) return ERR_NOT_SUPPORTED;
	if (!_CryptProtectData(&input, NULL, NULL, NULL, NULL, 0, &output)) return GetLastError();

	for (i = 0; i < output.cbData; i++) {
		String_Append(dst, output.pbData[i]);
	}
	LocalFree(output.pbData);
	return 0;
}
cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	DATA_BLOB input, output;
	int i;
	input.cbData = len; input.pbData = (BYTE*)data;

	if (!_CryptUnprotectData) LoadCryptFuncs();
	if (!_CryptUnprotectData) return ERR_NOT_SUPPORTED;
	if (!_CryptUnprotectData(&input, NULL, NULL, NULL, NULL, 0, &output)) return GetLastError();

	for (i = 0; i < output.cbData; i++) {
		String_Append(dst, output.pbData[i]);
	}
	LocalFree(output.pbData);
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

	for (i = 0; i < GAME_MAX_CMDARGS; i++) {
		args[i] = Platform_NextArg(&cmdArgs);

		if (!args[i].length) break;
	}
	return i;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	WCHAR path[NATIVE_STR_LEN + 1];
	int i, len;
	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;

	/* Get rid of filename at end of directory */
	for (i = len - 1; i >= 0; i--, len--) {
		if (path[i] == '/' || path[i] == '\\') break;
	}

	path[len] = '\0';
	return SetCurrentDirectoryW(path) ? 0 : GetLastError();
}
#endif
