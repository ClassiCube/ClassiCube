#include "Core.h"
#if defined CC_BUILD_WINCE

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
#include <tchar.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

static HANDLE heap;
const cc_result ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = ERROR_FILE_NOT_FOUND;
const cc_result ReturnCode_PathNotFound     = ERROR_PATH_NOT_FOUND;
const cc_result ReturnCode_DirectoryExists  = ERROR_ALREADY_EXISTS;
const cc_result ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = WSAECONNRESET;

const char* Platform_AppNameSuffix = " CE";
cc_bool  Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS;
#include "_PlatformBase.h"

// Current directory management for Windows CE
static WCHAR current_directory[MAX_PATH] = L"\\";
static cc_string Platform_NextArg(STRING_REF cc_string* args);
static CRITICAL_SECTION dir_lock;

/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "main_impl.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	cc_result res;
	SetupProgram(0, NULL);

	do {
		res = RunProgram(0, NULL);
	} while (Window_Main.Exists);

	Window_Free();
	ExitProcess(res);
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
void Platform_Log(const char* msg, int len) {
	WCHAR wbuf[2048];
	int wlen = MultiByteToWideChar(CP_UTF8, 0, msg, len, wbuf, 2047);
	if (wlen > 0) {
		wbuf[wlen] = 0;
		OutputDebugStringW(wbuf);
		OutputDebugStringW(L"\n");
	}
}

#define FILETIME_EPOCH 50491123200ULL
#define FileTime_TotalSecs(time) ((time / 10000000) + FILETIME_EPOCH)

TimeMS DateTime_CurrentUTC(void) {
	SYSTEMTIME st;
	FILETIME ft;
	cc_uint64 raw;
	
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
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

cc_uint64 Stopwatch_Measure(void) {
	return GetTickCount();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) * 1000; // GetTickCount returns milliseconds
}

/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) {
	// SetUnhandledExceptionFilter not available on Windows CE
}

void Process_Abort2(cc_result result, const char* raw_msg) {
	WCHAR wbuf[512];
    WCHAR fbuf[512];
	int wlen = MultiByteToWideChar(CP_UTF8, 0, raw_msg, -1, wbuf, 511);
	if (wlen > 0) {
        swprintf(fbuf, L"Error %s: 0x%04x", result, wbuf);
		MessageBoxW(NULL, fbuf, L"Error", MB_OK | MB_ICONERROR);
	}
	ExitProcess(result);
}

/*########################################################################################################################*
*-----------------------------------------------Current Directory Management---------------------------------------------*
*#########################################################################################################################*/

static void NormalizePath(WCHAR* path) {
	WCHAR* src = path;
	WCHAR* dst = path;
	WCHAR* component_start;
	
	while (*src) {
		if (*src == L'/' || *src == L'\\') {
			*dst++ = L'\\';
			src++;
			while (*src == L'/' || *src == L'\\') src++;
			continue;
		}
		
		component_start = dst;
		while (*src && *src != L'/' && *src != L'\\') {
			*dst++ = *src++;
		}
		
		if (dst - component_start == 1 && component_start[0] == L'.') {
			dst = component_start;
			if (dst > path && dst[-1] == L'\\') dst--;
		} else if (dst - component_start == 2 && component_start[0] == L'.' && component_start[1] == L'.') {
			dst = component_start;
			if (dst > path && dst[-1] == L'\\') dst--;
			while (dst > path && dst[-1] != L'\\') dst--;
			if (dst > path) dst--;
		}
	}
	
	if (dst == path) {
		*dst++ = L'\\';
	}
	*dst = L'\0';
}

static void SetCurrentDirectoryImpl(const WCHAR* path) {
	EnterCriticalSection(&dir_lock);
	
	if (path[0] == L'\\') {
		wcscpy(current_directory, path);
	} else {
		wcscat(current_directory, L"\\");
		wcscat(current_directory, path);
	}
	
	NormalizePath(current_directory);
	LeaveCriticalSection(&dir_lock);
}

static void GetCurrentDirectoryImpl(WCHAR* buffer, DWORD size) {
	EnterCriticalSection(&dir_lock);
	wcsncpy(buffer, current_directory, size - 1);
	buffer[size - 1] = L'\0';
	LeaveCriticalSection(&dir_lock);
}

static void MakeAbsolutePath(const WCHAR* relative_path, WCHAR* absolute_path, DWORD size) {
	if (relative_path[0] == L'\\') {
		wcsncpy(absolute_path, relative_path, size - 1);
	} else {
		GetCurrentDirectoryImpl(absolute_path, size);
		if (wcslen(absolute_path) + wcslen(relative_path) + 2 < size) {
			wcscat(absolute_path, L"\\");
			wcscat(absolute_path, relative_path);
		}
	}
	absolute_path[size - 1] = L'\0';
	NormalizePath(absolute_path);
}
/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) {
	String_AppendConst(path, "\\cache");
}

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
	WCHAR fullPath[MAX_PATH];
	
	MakeAbsolutePath(path->uni, fullPath, MAX_PATH);

	return CreateDirectoryW(fullPath, NULL) ? 0 : GetLastError();
}

int File_Exists(const cc_filepath* path) {
	WCHAR fullPath[MAX_PATH];
	DWORD attribs;
	
	MakeAbsolutePath(path->uni, fullPath, MAX_PATH);

	
	attribs = GetFileAttributesW(fullPath);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

static cc_result Directory_EnumCore(const cc_string* dirPath, const cc_string* file, DWORD attribs,
									void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	int is_dir;
	
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
	cc_filepath str;
	HANDLE find;
	cc_result res;
	int i;

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_EncodePath(&str, &path);
	
	find = FindFirstFileW(str.uni, &eW);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		path.length = 0;
		for (i = 0; i < MAX_PATH && eW.cFileName[i]; i++) {
			// Convert wide char to UTF-8
			char utf8[4];
			int len = WideCharToMultiByte(CP_UTF8, 0, &eW.cFileName[i], 1, utf8, 4, NULL, NULL);
			for (int j = 0; j < len; j++) {
				String_Append(&path, utf8[j]);
			}
		}
		if ((res = Directory_EnumCore(dirPath, &path, eW.dwFileAttributes, obj, callback))) return res;
	} while (FindNextFileW(find, &eW));

	res = GetLastError();
	FindClose(find);
	return res == ERROR_NO_MORE_FILES ? 0 : res;
}

static cc_result DoFile(cc_file* file, const cc_filepath* path, DWORD access, DWORD createMode) {
	WCHAR fullPath[MAX_PATH];
	
	MakeAbsolutePath(path->uni, fullPath, MAX_PATH);
	
	*file = CreateFileW(fullPath, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
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
	BOOL success = ReadFile(file, data, count, (DWORD*)bytesRead, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
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
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { 
	Sleep(milliseconds); 
}

static DWORD WINAPI ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	func();
	return 0;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	DWORD threadID;
	HANDLE thread = CreateThread(NULL, stackSize, ExecThread, (void*)func, 0, &threadID);
	if (!thread) Process_Abort2(GetLastError(), "Creating thread");
	*handle = thread;
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


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	CRITICAL_SECTION* ptr = (CRITICAL_SECTION*)Mem_Alloc(1, sizeof(CRITICAL_SECTION), "mutex");
	InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(void* handle) { 
	DeleteCriticalSection((CRITICAL_SECTION*)handle); 
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) { 
	EnterCriticalSection((CRITICAL_SECTION*)handle); 
}

void Mutex_Unlock(void* handle) { 
	LeaveCriticalSection((CRITICAL_SECTION*)handle); 
}

void* Waitable_Create(const char* name) {
	void* handle = CreateEventW(NULL, false, false, NULL);
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

void Waitable_Signal(void* handle) {
	SetEvent((HANDLE)handle);
}

void Waitable_Wait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	WaitForSingleObject((HANDLE)handle, milliseconds);
}

/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj, int isDirectory) {
	if (isDirectory) {
		Directory_Enum(path, NULL, FontDirCallback);
	} else {
		SysFonts_Register(path, NULL);
	}
}

void Platform_LoadSysFonts(void) { 
	cc_string fontDir = String_FromConst("\\Windows\\Fonts");
	Directory_Enum(&fontDir, NULL, FontDirCallback);
}

/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
static char sockaddr_size_check[sizeof(SOCKADDR_STORAGE) < CC_SOCKETADDR_MAXSIZE ? 1 : -1];

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	SOCKADDR_IN* addr4 = (SOCKADDR_IN*)addr->data;

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
	return false;
}

static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	struct hostent* res = gethostbyname(host);
	SOCKADDR_IN* addr4;
	char* src_addr;
	int i;

	if (!res) {
		cc_result wsa_res = WSAGetLastError();
		if (wsa_res == WSAHOST_NOT_FOUND) return SOCK_ERR_UNKNOWN_HOST;
		return ERR_INVALID_ARGUMENT;
	}
		
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)          return ERR_INVALID_ARGUMENT;

	for (i = 0; i < SOCKET_MAX_ADDRS; i++) {
		src_addr = res->h_addr_list[i];
		if (!src_addr) break;
		addrs[i].size = sizeof(SOCKADDR_IN);

		addr4 = (SOCKADDR_IN*)addrs[i].data;
		addr4->sin_family = AF_INET;
		addr4->sin_port   = SockAddr_EncodePort(port);
		addr4->sin_addr   = *(IN_ADDR*)src_addr;
	}

	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}
cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	SOCKADDR* raw_addr = (SOCKADDR*)addr->data;
	*s = socket(raw_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == INVALID_SOCKET) return WSAGetLastError();

	if (nonblocking) {
		u_long blockingMode = 1;
		ioctlsocket(*s, FIONBIO, &blockingMode);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	SOCKADDR* raw_addr = (SOCKADDR*)addr->data;
	int res = connect(s, raw_addr, addr->size);
	return res == SOCKET_ERROR ? WSAGetLastError() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = recv(s, (char*)data, count, 0);
	if (recvCount != SOCKET_ERROR) { *modified = recvCount; return 0; }
	*modified = 0; return WSAGetLastError();
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = send(s, (const char*)data, count, 0);
	if (sentCount != SOCKET_ERROR) { *modified = sentCount; return 0; }
	*modified = 0; return WSAGetLastError();
}

void Socket_Close(cc_socket s) {
	shutdown(s, SD_BOTH);
	closesocket(s);
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
		selectCount = select(1, &set1, NULL, NULL, &time);

		*success = set1.fd_count != 0; 
	} else {
		selectCount = select(1, NULL, &set1, &set2, &time);

		*success = set1.fd_count != 0 || set2.fd_count != 0;
	}

	if (selectCount >= 0) return 0;
	*success = false; 
	return WSAGetLastError();
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
	getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error, &errSize);
	return error;
}

/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = false;

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return SetGameArgs(args, numArgs);
}

void Process_Exit(cc_result code) { 
	ExitProcess(code); 
}

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Updater_Supported = false;
const struct UpdaterInfo Updater_Info = { "&eUpdate not supported on Windows CE", 0 };

cc_bool Updater_Clean(void) { return true; }
cc_result Updater_Start(const char** action) { return ERR_NOT_SUPPORTED; }
cc_result Updater_GetBuildTime(cc_uint64* timestamp) { return ERR_NOT_SUPPORTED; }
cc_result Updater_MarkExecutable(void) { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) { return ERR_NOT_SUPPORTED; }

/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
const cc_string DynamicLib_Ext = String_FromConst(".dll");
static cc_result dynamicErr;

void* DynamicLib_Load2(const cc_string* path) {
	cc_filepath str;
	void* lib;

	Platform_EncodePath(&str, path);
	lib = LoadLibraryW(str.uni);
	if (!lib) dynamicErr = GetLastError();
	return lib;
}

void* DynamicLib_Get2(void* lib, const char* name) {
	WCHAR wname[256];
	MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
	
	void* addr = GetProcAddressW((HMODULE)lib, wname);
	if (!addr) dynamicErr = GetLastError();
	return addr;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	cc_result res = dynamicErr;
	dynamicErr = 0;
	Platform_DescribeError(res, dst);
	String_Format1(dst, " (error %e)", &res);
	return true;
}

/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodeString(cc_winstring* dst, const cc_string* src) {
	int i;
	
	MultiByteToWideChar(CP_UTF8, 0, src->buffer, src->length, dst->uni, NATIVE_STR_LEN - 1);
	dst->uni[src->length] = 0;
	
	WideCharToMultiByte(CP_ACP, 0, dst->uni, -1, dst->ansi, NATIVE_STR_LEN - 1, NULL, NULL);
}

void Platform_Init(void) {
	WSADATA wsaData;
	cc_result res;

	heap = GetProcessHeap();
	
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_SysWarn(res, "starting WSA");
}

void Platform_Free(void) {
	WSACleanup();
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
static const cc_uint8 xor_key[] = { 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x9A };

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* bytes = (const cc_uint8*)data;
	int i;
	
	for (i = 0; i < len; i++) {
		String_Append(dst, bytes[i] ^ xor_key[i % 8]);
	}
	return 0;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* bytes = (const cc_uint8*)data;
	int i;
	
	for (i = 0; i < len; i++) {
		String_Append(dst, bytes[i] ^ xor_key[i % 8]);
	}
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	// Simple random generator
	static cc_uint32 seed = 1;
	cc_uint8* bytes = (cc_uint8*)data;
	int i;
	
	if (seed == 1) {
		seed = GetTickCount();
	}
	
	for (i = 0; i < len; i++) {
		seed = seed * 1103515245 + 12345;
		bytes[i] = (cc_uint8)(seed >> 8);
	}
	return 0;
}

/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	LPWSTR cmdLine = GetCommandLineW();
	char cmdLineUtf8[1024];
	cc_string cmdArgs;
	int i;
	
	if (gameHasArgs) return GetGameArgs(args);
	
	WideCharToMultiByte(CP_UTF8, 0, cmdLine, -1, cmdLineUtf8, 1024, NULL, NULL);
	cmdArgs = String_FromReadonly(cmdLineUtf8);
	
	Platform_NextArg(&cmdArgs);
	
	for (i = 0; i < GAME_MAX_CMDARGS; i++) {
		args[i] = Platform_NextArg(&cmdArgs);
		if (!args[i].length) break;
	}
	return i;
}

static cc_string Platform_NextArg(STRING_REF cc_string* args) {
	cc_string arg;
	int end;

	// Remove leading spaces
	while (args->length && args->buffer[0] == ' ') {
		*args = String_UNSAFE_SubstringAt(args, 1);
	}

	if (args->length && args->buffer[0] == '"') {
		// Quoted argument
		*args = String_UNSAFE_SubstringAt(args, 1);
		end = String_IndexOf(args, '"');
	} else {
		end = String_IndexOf(args, ' ');
	}

	if (end == -1) {
		arg = *args;
		args->length = 0;
	} else {
		arg = String_UNSAFE_Substring(args, 0, end);
		*args = String_UNSAFE_SubstringAt(args, end + 1);
	}
	return arg;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char** argv) {
	WCHAR module_path[MAX_PATH];
	WCHAR* last_slash;
	DWORD len;
	
	len = GetModuleFileNameW(NULL, module_path, MAX_PATH);
	if (len == 0) {
		SetCurrentDirectoryImpl(L"\\");
		return 0;
	}
	
	last_slash = wcsrchr(module_path, L'\\');
	if (last_slash) {
		*last_slash = L'\0';
		SetCurrentDirectoryImpl(module_path);
	} else {
		SetCurrentDirectoryImpl(L"\\");
	}
	
	return 0;
}


#endif
