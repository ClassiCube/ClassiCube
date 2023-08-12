#include "Core.h"
#if defined CC_BUILD_DREAMCAST

#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>
#include <kos.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;


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
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / 1000;
}

cc_uint64 Stopwatch_Measure(void) {
	return timer_ns_gettime64();
}

void Platform_Log(const char* msg, int len) {
	fs_write(STDOUT_FILENO, msg,  len);
	fs_write(STDOUT_FILENO, "\n",   1);
}

TimeMS DateTime_CurrentUTC_MS(void) {
	uint32 secs, ms;
	timer_ms_gettime(&secs, &ms);
	
	// TODO: Can we get away with not adding boot time.. ?
	return (rtc_boot_time() + secs) * 1000 + ms;
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


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }

extern int __path_absolute(const char *in, char *out, int len);
static void GetNativePath(char* str, const cc_string* path) {
	//char tmp[NATIVE_STR_LEN + 1];
	String_EncodeUtf8(str, path);
	//__path_absolute(tmp, str, NATIVE_STR_LEN);
}

cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	int res = fs_mkdir(str);
	return res == -1 ? errno : 0;
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	struct stat sb;
	GetNativePath(str, path);
	return fs_stat(str, &sb, 0) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	int res;

	GetNativePath(str, dirPath);
	int fd = fs_open(str, O_DIR | O_RDONLY);
	if (fd < 0) return errno;

	String_InitArray(path, pathBuffer);
	dirent_t* entry;
	errno = 0;
	
	while ((entry = fs_readdir(fd))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry (PSP does return them)
		// TODO: Does Dreamcast?
		char* src = entry->name;
		if (src[0] == '.' && src[1] == '\0')                  continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;
		
		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		// negative size indicates a directory entry
		if (entry->size < 0) {
			res = Directory_Enum(&path, obj, callback);
			if (res) break;
		} else {
			callback(&path, obj);
		}
	}

	int err = errno; // save error from fs_readdir
	fs_close(fd);
	return err;
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	int res = fs_open(str, mode);
	*file   = res;
	return res == -1 ? errno : 0;
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
	int res    = fs_read(file, data, count);
	*bytesRead = res;
	return res == -1 ? errno : 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	int res     = fs_write(file, data, count);
	*bytesWrote = res;
	return res == -1 ? errno : 0;
}

cc_result File_Close(cc_file file) {
	int res = fs_close(file);
	return res == -1 ? errno : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	
	int res = fs_seek(file, offset, modes[seekType]);
	return res == -1 ? errno : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int res = fs_seek(file, 0, SEEK_CUR);
	*pos    = res;
	return res == -1 ? errno : 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int res = fs_total(file);
	*len    = res;
	return res == -1 ? errno : 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: Dreamcast uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	thd_sleep(milliseconds); 
}

static void* ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	(func)();
	return NULL;
}

void* Thread_Create(Thread_StartFunc func) {
	kthread_attr_t attrs = { 0 };
	attrs.stack_size     = 64 * 1024;
	attrs.label          = "CC thread";
	return thd_create_ex(&attrs, ExecThread, func);
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
}

void Thread_Detach(void* handle) {
	thd_detach((kthread_t*)handle);
}

void Thread_Join(void* handle) {
	thd_join((kthread_t*)handle, NULL);
}

void* Mutex_Create(void) {
	mutex_t* ptr = (mutex_t*)Mem_Alloc(1, sizeof(mutex_t), "mutex");
	int res = mutex_init(ptr, MUTEX_TYPE_NORMAL);
	if (res) Logger_Abort2(errno, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = mutex_destroy((mutex_t*)handle);
	if (res) Logger_Abort2(errno, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = mutex_lock((mutex_t*)handle);
	if (res) Logger_Abort2(errno, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = mutex_unlock((mutex_t*)handle);
	if (res) Logger_Abort2(errno, "Unlocking mutex");
}

void* Waitable_Create(void) {
	semaphore_t* ptr = (semaphore_t*)Mem_Alloc(1, sizeof(semaphore_t), "waitable");
	int res = sem_init(ptr, 0);
	if (res) Logger_Abort2(errno, "Creating waitable");
	return ptr;
}

void Waitable_Free(void* handle) {
	int res = sem_destroy((semaphore_t*)handle);
	if (res) Logger_Abort2(errno, "Destroying waitable");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	int res = sem_signal((semaphore_t*)handle);
	if (res < 0) Logger_Abort2(errno, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int res = sem_wait((semaphore_t*)handle);
	if (res < 0) Logger_Abort2(errno, "Event wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	int res = sem_wait_timed((semaphore_t*)handle, milliseconds);
	if (res >= 0) return;
	
	int err = errno;
	if (err != ETIMEDOUT) Logger_Abort2(err, "Event timed wait");
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in  v4;
	#ifdef AF_INET6
	struct sockaddr_in6 v6;
	struct sockaddr_storage total;
	#endif
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
	#ifdef AF_INET6
	if (inet_pton(AF_INET6, str, &addr->v6.sin6_addr) > 0) return AF_INET6;
	#endif
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

	// TODO is this even right
	if (nonblocking) {
		int blocking_raw = -1; /* non-blocking mode */
		fs_ioctl(*s, FNONBIO, &blocking_raw);
	}

	#ifdef AF_INET6
	if (family == AF_INET6) {
		addr.v6.sin6_family = AF_INET6;
		addr.v6.sin6_port   = htons(port);
		addrSize = sizeof(addr.v6);
	}
	#endif
	if (family == AF_INET) {
		addr.v4.sin_family  = AF_INET;
		addr.v4.sin_port    = htons(port);
		addrSize = sizeof(addr.v4);
	}

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
	// PSP *sometimes* doesn't use argv[0] for program name and so argc will be 0
	if (!argc) return 0;
	
	argc--; argv++; // skip executable path argument

	int count = min(argc, GAME_MAX_CMDARGS);
	Platform_Log1("ARGS: %i", &count);
	
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromReadonly(argv[i]);
		Platform_Log2("  ARG %i = %c", &i, argv[i]);
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0; // TODO switch to RomFS ??
}

void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
const char* const Updater_D3D9 = NULL;
cc_bool Updater_Clean(void) { return true; }

const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };

cc_result Updater_Start(const char** action) {
	*action = "Starting game";
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_MarkExecutable(void) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
/* TODO can this actually be supported somehow */
const cc_string DynamicLib_Ext = String_FromConst(".so");

void* DynamicLib_Load2(const cc_string* path)      { return NULL; }
void* DynamicLib_Get2(void* lib, const char* name) { return NULL; }

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	String_AppendConst(dst, "Dynamic linking unsupported");
	return true;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Init(void) {
	Platform_SingleProcess = true;
	/*pspDebugSioInit();*/ 
	
	char cwd[600] = { 0 };
	char* ptr = getcwd(cwd, 600);
	Platform_Log1("WORKING DIR: %c", ptr);
}
void Platform_Free(void) { }

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