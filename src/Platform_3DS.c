#include "Core.h"
#if defined CC_BUILD_3DS

#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "PackedCol.h"
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <stdio.h>
#include <malloc.h>
#include <netdb.h>
#include <3ds.h>
#include <citro3d.h>

#define US_PER_SEC 1000000LL
#define NS_PER_MS 1000000LL

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;

// https://gbatemp.net/threads/homebrew-development.360646/page-245
// 3DS defaults to stack size of *32 KB*.. way too small
unsigned int __stacksize__ = 256 * 1024;


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void*  dst, cc_uint8 value,  cc_uint32 numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, const void* src, cc_uint32 numBytes) { memcpy(dst, src,   numBytes); }

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? malloc(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	return calloc(numElems, elemsSize);
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? realloc(mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) free(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	//cc_file fd = -1;
	// CITRA LOGGING
	//cc_string path = String_Init(msg, len, len);
	//File_Open(&fd, &path);
	//if (fd > 0) File_Close(fd);
	
	write(STDOUT_FILENO, msg,  len);
	write(STDOUT_FILENO, "\n",   1);
}

#define UnixTime_TotalMS(time) ((cc_uint64)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

void DateTime_CurrentLocal(struct DateTime* t) {
	struct timeval cur; 
	struct tm loc_time;
	gettimeofday(&cur, NULL);
	localtime_r(&cur.tv_sec, &loc_time);

	t->year   = loc_time.tm_year + 1900;
	t->month  = loc_time.tm_mon  + 1;
	t->day    = loc_time.tm_mday;
	t->hour   = loc_time.tm_hour;
	t->minute = loc_time.tm_min;
	t->second = loc_time.tm_sec;
}

cc_uint64 Stopwatch_Measure(void) {
	return svcGetSystemTick();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;	
	// TODO: This doesn't seem to accurately measure time in Citra.
	// hopefully it works better on a real 3DS?
	
	// See CPU_TICKS_PER_USEC in libctru/include/3ds/os.h
	return (end - beg) * US_PER_SEC / SYSCLOCK_ARM11;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }
static const cc_string root_path = String_FromConst("sdmc:/3ds/ClassiCube/");

static void GetNativePath(char* str, const cc_string* path) {
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, path);
}

cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	return mkdir(str, 0666) == -1 ? errno : 0; // FS has no permissions anyways
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	struct stat sb;
	GetNativePath(str, path);
	
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	struct dirent* entry;
	char* src;
	int len, res, is_dir;

	GetNativePath(str, dirPath);
	DIR* dirPtr = opendir(str);
	if (!dirPtr) return errno;

	/* POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed." */
	/* errno is sometimes leftover from previous calls, so always reset it before readdir gets called */
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);
		src = entry->d_name;

		len = String_Length(src);
		String_AppendUtf8(&path, src, len);
		is_dir = entry->d_type == DT_DIR;
		/* TODO: fallback to stat when this fails */

		if (is_dir) {
			res = Directory_Enum(&path, obj, callback);
			if (res) { closedir(dirPtr); return res; }
		} else {
			callback(&path, obj);
		}
		errno = 0;
	}

	res = errno; /* return code from readdir */
	closedir(dirPtr);
	return res;
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);

	*file = open(str, mode, 0666); // FS has no permissions anyways
	return *file == -1 ? errno : 0;
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return File_Do(file, path, O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	return File_Do(file, path, O_RDWR | O_CREAT | O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return File_Do(file, path, O_RDWR | O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	*bytesRead = read(file, data, count);
	return *bytesRead == -1 ? errno : 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	*bytesWrote = write(file, data, count);
	return *bytesWrote == -1 ? errno : 0;
}

cc_result File_Close(cc_file file) {
	return close(file) == -1 ? errno : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return lseek(file, offset, modes[seekType]) == -1 ? errno : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	*pos = lseek(file, 0, SEEK_CUR);
	return *pos == -1 ? errno : 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	struct stat st;
	if (fstat(file, &st) == -1) { *len = -1; return errno; }
	*len = st.st_size; return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { 
	svcSleepThread(milliseconds * NS_PER_MS); 
}

static void Exec3DSThread(void* param) {
	((Thread_StartFunc)param)(); 
}

void* Thread_Create(Thread_StartFunc func) {
	//TODO: Not quite correct, but eh
	return threadCreate(Exec3DSThread, (void*)func, 256 * 1024, 0x3f, -2, false);
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	
}

void Thread_Detach(void* handle) {
	Thread thread = (Thread)handle;
	threadDetach(thread);
}

void Thread_Join(void* handle) {
	Thread thread = (Thread)handle;
	threadJoin(thread, U64_MAX);
	threadFree(thread);
}

void* Mutex_Create(void) {
	LightLock* lock = (LightLock*)Mem_Alloc(1, sizeof(LightLock), "mutex");
	LightLock_Init(lock);
	return lock;
}

void Mutex_Free(void* handle) {
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	LightLock_Lock((LightLock*)handle);
}

void Mutex_Unlock(void* handle) {
	LightLock_Unlock((LightLock*)handle);
}

void* Waitable_Create(void) {
	LightEvent* event = (LightEvent*)Mem_Alloc(1, sizeof(LightEvent), "waitable");
	LightEvent_Init(event, RESET_ONESHOT);
	return event;
}

void Waitable_Free(void* handle) {
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	LightEvent_Signal((LightEvent*)handle);
}

void Waitable_Wait(void* handle) {
	LightEvent_Wait((LightEvent*)handle);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	s64 timeout_ns = milliseconds * (1000 * 1000); // milliseconds to nanoseconds
	int res = LightEvent_WaitTimeout((LightEvent*)handle, timeout_ns);
	if (res) Logger_Abort2(res, "Waiting timed event");
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
	int found = false;

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int res = getaddrinfo(host, NULL, &hints, &result);
	if (res == -NO_DATA) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	for (cur = result; cur; cur = cur->ai_next) {
		if (cur->ai_family != AF_INET) continue;
		found = true;

		Mem_Copy(addr, cur->ai_addr, cur->ai_addrlen);
		break;
	}

	freeaddrinfo(result);
	return found ? 0 : ERR_INVALID_ARGUMENT;
}

static int ParseAddress(union SocketAddress* addr, const cc_string* address) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, address);

	if (inet_pton(AF_INET, str, &addr->v4.sin_addr) > 0) return 0;
	return ParseHost(addr, str);
}

int Socket_ValidAddress(const cc_string* address) {
	union SocketAddress addr;
	return ParseAddress(&addr, address) == 0;
}

cc_result Socket_Connect(cc_socket* s, const cc_string* address, int port, cc_bool nonblocking) {
	union SocketAddress addr;
	int res;

	*s = -1;
	if ((res = ParseAddress(&addr, address))) return res;

	*s = socket(AF_INET, SOCK_STREAM, 0); // https://www.3dbrew.org/wiki/SOCU:socket
	if (*s == -1) return errno;
	
	if (nonblocking) {
		int flags = fcntl(*s, F_GETFL, 0);
		if (flags >= 0) fcntl(*s, F_SETFL, flags | O_NONBLOCK);
	}

	addr.v4.sin_family = AF_INET;
	addr.v4.sin_port   = htons(port);

	res = connect(*s, &addr.raw, sizeof(addr.v4));
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

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	if (poll(&pfd, 1, 0) == -1) { *success = false; return errno; }
	
	/* to match select, closed socket still counts as readable */
	int flags = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success  = (pfd.revents & flags) != 0;
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
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;
static cc_bool gameHasArgs;

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	for (int i = 0; i < numArgs; i++) 
	{
		String_CopyToRawArray(gameArgs[i], &args[i]);
	}
	
	Platform_LogConst("START GAME");
	gameHasArgs = true;
	gameNumArgs = numArgs;
	return 0;
}

static int GetGameArgs(cc_string* args) {
	int count = gameNumArgs;
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromRawArray(gameArgs[i]);
	}
	
	// clear arguments so after game is closed, launcher is started
	gameNumArgs = 0;
	return count;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	if (gameHasArgs) return GetGameArgs(args);
	// 3DS *sometimes* doesn't use argv[0] for program name and so argc will be 0
	//   (e.g. when running from Citra)
	if (!argc) return 0;
	
	argc--; argv++; // skip executable path argument
	
	int count = min(argc, GAME_MAX_CMDARGS);
	Platform_Log1("ARGS: %i", &count);
	
	for (int i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i]);
		Platform_Log2("  ARG %i = %c", &i, argv[i]);
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartOpen(const cc_string* args) {
	char url[NATIVE_STR_LEN];
	int len = String_EncodeUtf8(url, args);
	
	// TODO: Not sure if this works or not
	APT_PrepareToStartSystemApplet(APPID_WEB);
	return APT_StartSystemApplet(APPID_WEB, url, len + 1, CUR_PROCESS_HANDLE); 
	// len + 1 for null terminator
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
const char* const Updater_D3D9 = NULL;
cc_bool Updater_Clean(void) { return true; }

const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };

cc_result Updater_Start(const char** action) { *action = "Starting"; return ERR_NOT_SUPPORTED; }

cc_result Updater_GetBuildTime(cc_uint64* timestamp) { return ERR_NOT_SUPPORTED; }

cc_result Updater_MarkExecutable(void) { return ERR_NOT_SUPPORTED; }

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/

const cc_string DynamicLib_Ext = String_FromConst(".so");

void* DynamicLib_Load2(const cc_string* path) { return NULL; }

void* DynamicLib_Get2(void* lib, const char* name) { return NULL; }

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	String_AppendConst(dst, "Dynamic linking unsupported");
	return true;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#define SOC_CTX_ALIGN 0x1000
#define SOC_CTX_SIZE  0x1000 * 128

static void CreateRootDirectory(const char* path) {
	// create root directories (no permissions anyways)
	int res = mkdir(path, 0666);
	if (res >= 0) return;
	
	int err = errno;
	Platform_Log2("mkdir %c FAILED: %i", path, &err);
}

void Platform_Init(void) { 
	Platform_SingleProcess = true;
	
	// create root directories (no permissions anyways)
	CreateRootDirectory("sdmc:/3ds");
	CreateRootDirectory("sdmc:/3ds/ClassiCube");
	
	// See https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/services/soc.h
	//  * @param context_addr Address of a page-aligned (0x1000) buffer to be used.
	//  * @param context_size Size of the buffer, a multiple of 0x1000.
	//  * @note The specified context buffer can no longer be accessed by the process which called this function, since the userland permissions for this block are set to no-access.
	void* buffer = memalign(SOC_CTX_ALIGN, SOC_CTX_SIZE);
	if (!buffer) return;
	socInit(buffer, SOC_CTX_SIZE);
}

void Platform_Free(void) {
	socExit();
	// TODO free soc buffer? probably no point
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char chars[NATIVE_STR_LEN];
	int len;

	/* For unrecognised error codes, strerror_r might return messages */
	/*  such as 'No error information', which is not very useful */
	/* (could check errno here but quicker just to skip entirely) */
	if (res >= 1000) return false;

	len = strerror_r(res, chars, NATIVE_STR_LEN);
	if (len == -1) return false;

	len = String_CalcLen(chars, NATIVE_STR_LEN);
	String_AppendUtf8(dst, chars, len);
	return true;
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}
cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}
#endif