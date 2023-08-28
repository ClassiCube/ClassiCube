#include "Core.h"
#if defined CC_BUILD_XBOX
#include "_PlatformBase.h"
#include "Stream.h"
#include "Funcs.h"
#include "Utils.h"
#include "Errors.h"

#include <windows.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <nxdk/net.h>
#include <nxdk/mount.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = ERROR_FILE_NOT_FOUND;
const cc_result ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = ERROR_ALREADY_EXISTS;
const char* Platform_AppNameSuffix = " XBox";


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	
	// log to on-screen display
	debugPrint(tmp);
	debugPrint("\n");
	// log to cxbx-reloaded console
	OutputDebugStringA(tmp);
}

#define FILETIME_EPOCH 50491123200000ULL
#define FILETIME_UNIX_EPOCH 11644473600LL
#define FileTime_TotalMS(time)  ((time / 10000)    + FILETIME_EPOCH)
#define FileTime_UnixTime(time) ((time / 10000000) - FILETIME_UNIX_EPOCH)
TimeMS DateTime_CurrentUTC_MS(void) {
	LARGE_INTEGER ft;
	
	KeQuerySystemTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	return FileTime_TotalMS(ft.QuadPart);
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

/* TODO: check this is actually accurate */
static cc_uint64 sw_freqMul = 1, sw_freqDiv = 1;
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ((end - beg) * sw_freqMul) / sw_freqDiv;
}

cc_uint64 Stopwatch_Measure(void) {
	return KeQueryPerformanceCounter();
}

static void Stopwatch_Init(void) {
	ULONGLONG freq = KeQueryPerformanceFrequency();

	sw_freqMul = 1000 * 1000;
	sw_freqDiv = freq;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static cc_string root_path = String_FromConst("E:\\ClassiCube\\");
static BOOL hdd_mounted;

static void GetNativePath(char* str, const cc_string* src) {
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	
	// XBox kernel doesn't seem to convert /
	for (int i = 0; i < src->length; i++) 
	{
		char c = (char)src->buffer[i];
		if (c == '/') c = '\\';
		*str++ = c;
	}
	*str = '\0';
}

cc_result Directory_Create(const cc_string* path) {
	if (!hdd_mounted) return ERR_NOT_SUPPORTED;
	
	char str[NATIVE_STR_LEN];
	cc_result res;

	GetNativePath(str, path);
	return CreateDirectoryA(str, NULL) ? 0 : GetLastError();
}

int File_Exists(const cc_string* path) {
	if (!hdd_mounted) return ERR_NOT_SUPPORTED;
	
	char str[NATIVE_STR_LEN];
	DWORD attribs;

	GetNativePath(str, path);
	attribs = GetFileAttributesA(str);

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
	if (!hdd_mounted) return ERR_NOT_SUPPORTED;
	
	cc_string path; char pathBuffer[MAX_PATH + 10];
	WIN32_FIND_DATAA eA;
	char str[NATIVE_STR_LEN];
	HANDLE find;
	cc_result res;	

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	GetNativePath(str, &path);
	
	find = FindFirstFileA(str, &eA);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		path.length = 0;
		for (int i = 0; i < MAX_PATH && eA.cFileName[i]; i++) 
		{
			String_Append(&path, Convert_CodepointToCP437(eA.cFileName[i]));
		}
		if ((res = Directory_EnumCore(dirPath, &path, eA.dwFileAttributes, obj, callback))) return res;
	} while (FindNextFileA(find, &eA));

	res = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return res == ERROR_NO_MORE_FILES ? 0 : res;
}

static cc_result DoFile(cc_file* file, const cc_string* path, DWORD access, DWORD createMode) {
	if (!hdd_mounted) return ERR_NOT_SUPPORTED;
	
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	cc_result res;

	*file = CreateFileA(str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
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
*#############################################################################################################p############*/
void Thread_Sleep(cc_uint32 milliseconds) { Sleep(milliseconds); }
static DWORD WINAPI ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	func();
	return 0;
}

void* Thread_Create(Thread_StartFunc func) {
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, ExecThread, (void*)func, CREATE_SUSPENDED, &threadID);
	if (!handle) {
		Logger_Abort2(GetLastError(), "Creating thread");
	}
	return handle;
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	NtResumeThread((HANDLE)handle, NULL);
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
	RtlInitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(void* handle)   { 
	RtlDeleteCriticalSection((CRITICAL_SECTION*)handle); 
	Mem_Free(handle);
}
void Mutex_Lock(void* handle)   { RtlEnterCriticalSection((CRITICAL_SECTION*)handle); }
void Mutex_Unlock(void* handle) { RtlLeaveCriticalSection((CRITICAL_SECTION*)handle); }

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

void Waitable_Signal(void* handle) { NtSetEvent((HANDLE)handle, NULL); }
void Waitable_Wait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	WaitForSingleObject((HANDLE)handle, milliseconds);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in v4;
};

static int ParseHost(union SocketAddress* addr, const char* host) {
	struct addrinfo hints = { 0 };
	struct addrinfo* result;
	struct addrinfo* cur;
	int family = 0, res;

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	res = getaddrinfo(host, NULL, &hints, &result);
	if (res) return 0;

	for (cur = result; cur; cur = cur->ai_next) {
		if (cur->ai_family != AF_INET) continue;
		family = AF_INET;

		Mem_Copy(addr, cur->ai_addr, cur->ai_addrlen);
		break;
	}

	freeaddrinfo(result);
	return family;
}

static int ParseAddress(union SocketAddress* addr, const cc_string* address) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, address);

	if (inet_pton(AF_INET,  str, &addr->v4.sin_addr)  > 0) return AF_INET;
	return ParseHost(addr, str);
}

int Socket_ValidAddress(const cc_string* address) {
	union SocketAddress addr;
	return ParseAddress(&addr, address);
}

cc_result Socket_Connect(cc_socket* s, const cc_string* address, int port, cc_bool nonblocking) {
	int family, addrSize = 0;
	union SocketAddress addr;
	cc_result res;

	*s = -1;
	if (!(family = ParseAddress(&addr, address)))
		return ERR_INVALID_ARGUMENT;

	*s = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return errno;

	if (nonblocking) {
		int blocking_raw = -1; /* non-blocking mode */
		ioctl(*s, FIONBIO, &blocking_raw);
	}

	addr.v4.sin_family  = AF_INET;
	addr.v4.sin_port    = htons(port);
	addrSize = sizeof(addr.v4);

	res = connect(*s, &addr.raw, addrSize);
	return res == -1 ? errno : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = recv(s, data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return errno;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = send(s, data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return errno;
}

void Socket_Close(cc_socket s) {
	shutdown(s, SHUT_RDWR);
	close(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	if (poll(&pfd, 1, 0) == -1) { *success = false; return errno; }
	
	/* to match select, closed socket still counts as readable */
	flags    = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success = (pfd.revents & flags) != 0;
	return 0;
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	socklen_t resultSize = sizeof(socklen_t);
	cc_result res = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	if (res || *writable) return res;

	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	getsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

static void InitHDD(void) {
    hdd_mounted = nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
    if (!hdd_mounted) {
        Platform_LogConst("Failed to mount E:\ from Data partition");
        return;
    }
    Directory_Create(&String_Empty); // create root ClassiCube folder
}

void Platform_Init(void) {
	InitHDD();
	Stopwatch_Init();
	nxNetInit(NULL);
}

void Platform_Free(void) {
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return false;
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
static cc_result GetMachineID(cc_uint32* key) {
	return ERR_NOT_SUPPORTED;
}
#endif
