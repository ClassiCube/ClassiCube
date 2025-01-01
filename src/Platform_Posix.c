#include "Core.h"
#if defined CC_BUILD_POSIX

#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "SystemFonts.h"
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
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;
#define SUPPORTS_GETADDRINFO 1

#if defined CC_BUILD_ANDROID
const char* Platform_AppNameSuffix = " android alpha";
#elif defined CC_BUILD_IOS
const char* Platform_AppNameSuffix = " iOS alpha";
#else
const char* Platform_AppNameSuffix = "";
#endif
cc_bool Platform_SingleProcess;
cc_bool Platform_ReadonlyFilesystem;

/* Operating system specific include files */
#if defined CC_BUILD_DARWIN
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#if defined CC_BUILD_MACOS
#include <ApplicationServices/ApplicationServices.h>
#endif
#elif defined CC_BUILD_SOLARIS
#include <sys/filio.h>
#include <sys/systeminfo.h>
#elif defined CC_BUILD_BSD
#include <sys/sysctl.h>
#elif defined CC_BUILD_HAIKU || defined CC_BUILD_BEOS
/* TODO: Use load_image/resume_thread instead of fork */
/* Otherwise opening browser never works because fork fails */
#include <kernel/image.h>
#elif defined CC_BUILD_OS2
#include <libcx/net.h>
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_PM
#include <os2.h>
#endif


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_Set(void*  dst, cc_uint8 value,  unsigned numBytes) { return memset( dst, value, numBytes); }
void* Mem_Copy(void* dst, const void* src, unsigned numBytes) { return memcpy( dst, src,   numBytes); }
void* Mem_Move(void* dst, const void* src, unsigned numBytes) { return memmove(dst, src,   numBytes); }

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
#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
void Platform_Log(const char* msg, int len) {
	int ret;
	/* Avoid "ignoring return value of 'write' declared with attribute 'warn_unused_result'" warning */
	ret = write(STDOUT_FILENO, msg,  len);
	ret = write(STDOUT_FILENO, "\n",   1);
}
#endif

TimeMS DateTime_CurrentUTC(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
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
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#define NS_PER_SEC 1000000000ULL

#if defined CC_BUILD_HAIKU || defined CC_BUILD_BEOS
/* Implemented in interop_BeOS.cpp */
#elif defined CC_BUILD_DARWIN
static cc_uint64 sw_freqMul, sw_freqDiv;
static void Stopwatch_Init(void) {
	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);

	sw_freqMul = tb.numer;
	/* tb.denom may be large, so multiplying by 1000 overflows 32 bits */
	/* (one powerpc system had tb.denom of 33329426) */
	sw_freqDiv = (cc_uint64)tb.denom * 1000;
}

cc_uint64 Stopwatch_Measure(void) { return mach_absolute_time(); }

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ((end - beg) * sw_freqMul) / sw_freqDiv;
}
#elif defined CC_BUILD_SOLARIS
/* https://docs.oracle.com/cd/E86824_01/html/E54766/gethrtime-3c.html */
/* The gethrtime() function returns the current high-resolution real time. Time is expressed as nanoseconds since some arbitrary time in the past */
cc_uint64 Stopwatch_Measure(void) { return gethrtime(); }

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / 1000;
}
#else
/* clock_gettime is optional, see http://pubs.opengroup.org/onlinepubs/009696899/functions/clock_getres.html */
/* "... These functions are part of the Timers option and need not be available on all implementations..." */
cc_uint64 Stopwatch_Measure(void) {
	struct timespec t;
	#if defined CC_BUILD_IRIX || defined CC_BUILD_HPUX
	clock_gettime(CLOCK_REALTIME, &t);
	#else
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &t);
	#endif
	return (cc_uint64)t.tv_sec * NS_PER_SEC + t.tv_nsec;
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / 1000;
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
static const char* SignalDescribe(int type) {
	switch (type) {
	case SIGSEGV: return "SIGSEGV";
	case SIGBUS:  return "SIGBUS";
	case SIGILL:  return "SIGILL";
	case SIGABRT: return "SIGABRT";
	case SIGFPE:  return "SIGFPE";
	}
	return NULL;
}

static void SignalHandler(int sig, siginfo_t* info, void* ctx) {
	cc_string msg; char msgBuffer[128 + 1];
	struct sigaction sa = { 0 };
	const char* desc;
	int type, code;
	cc_uintptr addr;

	/* Uninstall handler to avoid chance of infinite loop */
	sa.sa_handler = SIG_DFL;
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS,  &sa, NULL);
	sigaction(SIGILL,  &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE,  &sa, NULL);

	type = info->si_signo;
	code = info->si_code;
	addr = (cc_uintptr)info->si_addr;
	desc = SignalDescribe(type);

	String_InitArray_NT(msg, msgBuffer);
	if (desc) {
		String_Format3(&msg, "Unhandled signal %c (code %i) at %x", desc,  &code, &addr);
	} else {
		String_Format3(&msg, "Unhandled signal %i (code %i) at %x", &type, &code, &addr);
	}
	msg.buffer[msg.length] = '\0';

	#if defined CC_BUILD_ANDROID
	/* deliberate Dalvik VM abort, try to log a nicer error for this */
	if (type == SIGSEGV && addr == 0xDEADD00D) Platform_TryLogJavaError();
	#endif
	Logger_DoAbort(0, msg.buffer, ctx);
}

void CrashHandler_Install(void) {
	struct sigaction sa = { 0 };
	/* sigemptyset(&sa.sa_mask); */
	/* NOTE: Calling sigemptyset breaks when using recent Android NDK and trying to run on old devices */
	sa.sa_sigaction = SignalHandler;
	sa.sa_flags     = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS,  &sa, NULL);
	sigaction(SIGILL,  &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE,  &sa, NULL);
}

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	String_EncodeUtf8(str, path);
}

#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
void Directory_GetCachePath(cc_string* path) { }
#endif

cc_result Directory_Create(const cc_filepath* path) {
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return mkdir(path->buffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

int File_Exists(const cc_filepath* path) {
	struct stat sb;
	return stat(path->buffer, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	DIR* dirPtr;
	struct dirent* entry;
	char* src;
	int len, res, is_dir;

	Platform_EncodePath(&str, dirPath);
	dirPtr = opendir(str.buffer);
	if (!dirPtr) return errno;

	/* POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed." */
	/* errno is sometimes leftover from previous calls, so always reset it before readdir gets called */
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		len = String_Length(src);
		String_AppendUtf8(&path, src, len);

#if defined CC_BUILD_HAIKU || defined CC_BUILD_SOLARIS || defined CC_BUILD_HPUX || defined CC_BUILD_IRIX || defined CC_BUILD_BEOS
		{
			char full_path[NATIVE_STR_LEN];
			struct stat sb;
			String_EncodeUtf8(full_path, &path);
			is_dir = stat(full_path, &sb) == 0 && S_ISDIR(sb.st_mode);
		}
#else
		is_dir = entry->d_type == DT_DIR;
		/* TODO: fallback to stat when this fails */
#endif

		callback(&path, obj, is_dir);
		errno = 0;
	}

	res = errno; /* return code from readdir */
	closedir(dirPtr);
	return res;
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	*file = open(path, mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	return *file == -1 ? errno : 0;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
#if !defined CC_BUILD_OS2
	return File_Do(file, path->buffer, O_RDONLY);
#else
	return File_Do(file, path->buffer, O_RDONLY | O_BINARY);
#endif
}
cc_result File_Create(cc_file* file, const cc_filepath* path) {
#if !defined CC_BUILD_OS2
	return File_Do(file, path->buffer, O_RDWR | O_CREAT | O_TRUNC);
#else
	return File_Do(file, path->buffer, O_RDWR | O_CREAT | O_TRUNC | O_BINARY);
#endif
}
cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
#if !defined CC_BUILD_OS2
	return File_Do(file, path->buffer, O_RDWR | O_CREAT);
#else
	return File_Do(file, path->buffer, O_RDWR | O_CREAT | O_BINARY);
#endif
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
void Thread_Sleep(cc_uint32 milliseconds) { usleep(milliseconds * 1000); }

#ifdef CC_BUILD_ANDROID
/* All threads using JNI must detach BEFORE they exit */
/* (see https://developer.android.com/training/articles/perf-jni#threads */
static void* ExecThread(void* param) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);

	((Thread_StartFunc)param)();
	(*VM_Ptr)->DetachCurrentThread(VM_Ptr);
	return NULL;
}
#else
static void* ExecThread(void* param) {
	((Thread_StartFunc)param)();
	return NULL;
}
#endif

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	pthread_t* ptr = (pthread_t*)Mem_Alloc(1, sizeof(pthread_t), "thread");
	pthread_attr_t attrs;
	int res;
	
	*handle = ptr;
	pthread_attr_init(&attrs);
	pthread_attr_setstacksize(&attrs, stackSize);
	
	res = pthread_create(ptr, &attrs, ExecThread, (void*)func);
	if (res) Process_Abort2(res, "Creating thread");
	pthread_attr_destroy(&attrs);
	
#if defined CC_BUILD_LINUX || defined CC_BUILD_HAIKU
	extern int pthread_setname_np(pthread_t thread, const char* name);
	pthread_setname_np(*ptr, name);
#elif defined CC_BUILD_FREEBSD || defined CC_BUILD_OPENBSD
	extern int pthread_set_name_np(pthread_t thread, const char* name);
	pthread_set_name_np(*ptr, name);
#elif defined CC_BUILD_NETBSD
	pthread_setname_np(*ptr, "%s", name);
#endif
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_detach(*ptr);
	if (res) Process_Abort2(res, "Detaching thread");
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_join(*ptr, NULL);
	if (res) Process_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(const char* name) {
	pthread_mutex_t* ptr = (pthread_mutex_t*)Mem_Alloc(1, sizeof(pthread_mutex_t), "mutex");
	int res = pthread_mutex_init(ptr, NULL);
	if (res) Process_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = pthread_mutex_destroy((pthread_mutex_t*)handle);
	if (res) Process_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = pthread_mutex_lock((pthread_mutex_t*)handle);
	if (res) Process_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = pthread_mutex_unlock((pthread_mutex_t*)handle);
	if (res) Process_Abort2(res, "Unlocking mutex");
}

struct WaitData {
	pthread_cond_t  cond;
	pthread_mutex_t mutex;
	int signalled; /* For when Waitable_Signal is called before Waitable_Wait */
};

void* Waitable_Create(const char* name) {
	struct WaitData* ptr = (struct WaitData*)Mem_Alloc(1, sizeof(struct WaitData), "waitable");
	int res;
	
	res = pthread_cond_init(&ptr->cond, NULL);
	if (res) Process_Abort2(res, "Creating waitable");
	res = pthread_mutex_init(&ptr->mutex, NULL);
	if (res) Process_Abort2(res, "Creating waitable mutex");

	ptr->signalled = false;
	return ptr;
}

void Waitable_Free(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;
	
	res = pthread_cond_destroy(&ptr->cond);
	if (res) Process_Abort2(res, "Destroying waitable");
	res = pthread_mutex_destroy(&ptr->mutex);
	if (res) Process_Abort2(res, "Destroying waitable mutex");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	ptr->signalled = true;
	Mutex_Unlock(&ptr->mutex);

	res = pthread_cond_signal(&ptr->cond);
	if (res) Process_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = pthread_cond_wait(&ptr->cond, &ptr->mutex);
		if (res) Process_Abort2(res, "Waitable wait");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	struct WaitData* ptr = (struct WaitData*)handle;
	struct timeval tv;
	struct timespec ts;
	int res;
	gettimeofday(&tv, NULL);

	/* absolute time for some silly reason */
	ts.tv_sec  = tv.tv_sec + milliseconds / 1000;
	ts.tv_nsec = 1000 * (tv.tv_usec + 1000 * (milliseconds % 1000));

	/* statement above might exceed max nsec, so adjust for that */
	while (ts.tv_nsec >= NS_PER_SEC) {
		ts.tv_sec++;
		ts.tv_nsec -= NS_PER_SEC;
	}

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = pthread_cond_timedwait(&ptr->cond, &ptr->mutex, &ts);
		if (res && res != ETIMEDOUT) Process_Abort2(res, "Waitable wait for");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
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
	int i;
#if defined CC_BUILD_ANDROID
	static const cc_string dirs[] = {
		String_FromConst("/system/fonts"),
		String_FromConst("/system/font"),
		String_FromConst("/data/fonts"),
	};
#elif defined CC_BUILD_NETBSD
	static const cc_string dirs[] = {
		String_FromConst("/usr/X11R7/lib/X11/fonts"),
		String_FromConst("/usr/pkg/lib/X11/fonts"),
		String_FromConst("/usr/pkg/share/fonts")
	};
#elif defined CC_BUILD_OPENBSD
	static const cc_string dirs[] = {
		String_FromConst("/usr/X11R6/lib/X11/fonts"),
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};
#elif defined CC_BUILD_HAIKU
	static const cc_string dirs[] = {
		String_FromConst("/system/data/fonts")
	};
#elif defined CC_BUILD_BEOS
	static const cc_string dirs[] = {
		String_FromConst("/boot/beos/etc/fonts")
	};
#elif defined CC_BUILD_DARWIN
	static const cc_string dirs[] = {
		String_FromConst("/System/Library/Fonts"),
		String_FromConst("/Library/Fonts")
	};
#elif defined CC_BUILD_SERENITY
	static const cc_string dirs[] = {
		String_FromConst("/res/fonts")
	};
#elif defined CC_BUILD_OS2
	static const cc_string dirs[] = {
		String_FromConst("/@unixroot/usr/share/fonts"),
		String_FromConst("/@unixroot/usr/local/share/fonts")
	};
#else
	static const cc_string dirs[] = {
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};
#endif
	for (i = 0; i < Array_Elems(dirs); i++) 
	{
		Platform_Log1("Searching for fonts in %s", &dirs[i]);
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_OS2
#undef AF_INET6
#endif

union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in  v4;
	#ifdef AF_INET6
	struct sockaddr_in6 v6;
	struct sockaddr_storage total;
	#endif
};
/* Sanity check to ensure cc_sockaddr struct is large enough to contain all socket addresses supported by this platform */
static char sockaddr_size_check[sizeof(union SocketAddress) < CC_SOCKETADDR_MAXSIZE ? 1 : -1];

#if SUPPORTS_GETADDRINFO
static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
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

	res = getaddrinfo(host, portRaw, &hints, &result);
	if (res == EAI_AGAIN) return SOCK_ERR_UNKNOWN_HOST;
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
#else
static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	struct hostent* res = gethostbyname(host);
	struct sockaddr_in* addr4;
	char* src_addr;
	int i;
	
	// Must have at least one IPv4 address
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)          return ERR_INVALID_ARGUMENT;

	for (i = 0; i < SOCKET_MAX_ADDRS; i++) 
	{
		src_addr = res->h_addr_list[i];
		if (!src_addr) break;
		addrs[i].size = sizeof(struct sockaddr_in);

		addr4 = (struct sockaddr_in*)addrs[i].data;
		addr4->sin_family = AF_INET;
		addr4->sin_port   = htons(port);
		addr4->sin_addr   = *(struct in_addr*)src_addr;
	}

	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}
#endif

cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	union SocketAddress* addr = (union SocketAddress*)addrs[0].data;
	char str[NATIVE_STR_LEN];

	String_EncodeUtf8(str, address);
	*numValidAddrs = 0;

	if (inet_pton(AF_INET,  str, &addr->v4.sin_addr)  > 0) {
		addr->v4.sin_family = AF_INET;
		addr->v4.sin_port   = htons(port);
		
		addrs[0].size  = sizeof(addr->v4);
		*numValidAddrs = 1;
		return 0;
	}
	
	#ifdef AF_INET6
	if (inet_pton(AF_INET6, str, &addr->v6.sin6_addr) > 0) {
		addr->v6.sin6_family = AF_INET6;
		addr->v6.sin6_port   = htons(port);
		
		addrs[0].size  = sizeof(addr->v6);
		*numValidAddrs = 1;
		return 0;
	}
	#endif
	
	return ParseHost(str, port, addrs, numValidAddrs);
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = socket(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return errno;

	if (nonblocking) {
		int blocking_raw = -1; /* non-blocking mode */
		ioctl(*s, FIONBIO, &blocking_raw);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	
	int res = connect(s, raw, addr->size);
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

#if defined CC_BUILD_DARWIN || defined CC_BUILD_BEOS
/* poll is broken on old OSX apparently https://daniel.haxx.se/docs/poll-vs-select.html */
/* BeOS lacks support for poll */
static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	FD_ZERO(&set);
	FD_SET(s, &set);

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(s + 1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(s + 1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return errno; }
	*success = FD_ISSET(s, &set) != 0; return 0;
}
#else
#include <poll.h>
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
#endif

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
cc_bool Process_OpenSupported = true;

#if defined CC_BUILD_MOBILE
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return SetGameArgs(args, numArgs);
}
#else
static cc_result Process_RawStart(const char* path, char** argv) {
	pid_t pid = fork();
	int err;
	if (pid == -1) return errno;

	if (pid == 0) {
		/* Executed in child process */
		execvp(path, argv);

		err = errno;
		Platform_Log2("execv %c failed = %i", path, &err);
		_exit(127); /* "command not found" */
	} else {
		/* Executed in parent process */
		/* We do nothing here.. */
		return 0;
	}
}

static cc_result Process_RawGetExePath(char* path, int* len);

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	char raw[GAME_MAX_CMDARGS][NATIVE_STR_LEN];
	char path[NATIVE_STR_LEN];
	int i, j, len = 0;
	char* argv[15];
	cc_result res;
	if (Platform_SingleProcess) return SetGameArgs(args, numArgs);

	res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';
	argv[0]   = path;

	for (i = 0, j = 1; i < numArgs; i++, j++) 
	{
		String_EncodeUtf8(raw[i], &args[i]);
		argv[j] = raw[i];
	}

	argv[j] = NULL;
	return Process_RawStart(path, argv);
}
#endif
void Process_Exit(cc_result code) { exit(code); }

/* Opening browser/starting shell is not really standardised */
#if defined CC_BUILD_ANDROID
/* Implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#elif defined CC_BUILD_MACOS
cc_result Process_StartOpen(const cc_string* args) {
	UInt8 str[NATIVE_STR_LEN];
	CFURLRef urlCF;
	int len;
	
	len   = String_EncodeUtf8(str, args);
	urlCF = CFURLCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, NULL);
	LSOpenCFURLRef(urlCF, NULL);
	CFRelease(urlCF);
	return 0;
}
#elif defined CC_BUILD_HAIKU || defined CC_BUILD_BEOS
/* Implemented in interop_BeOS.cpp */
#elif defined CC_BUILD_OS2
inline static void ShowErrorMessage(const char *url) {
	static char errorMsg[] = "Could not open browser. Please go to: ";
	cc_string message = String_Init(errorMsg, strlen(errorMsg), 500);
	String_AppendConst(&message, url);
	Logger_DialogWarn(&message);
}

cc_result Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	APIRET rc;
	UCHAR path[CCHMAXPATH], params[100], parambuffer[500], *paramptr;
	UCHAR userPath[CCHMAXPATH], sysPath[CCHMAXPATH];
	PRFPROFILE profile = { sizeof(userPath), userPath, sizeof(sysPath), sysPath };
	HINI os2Ini;
	HAB hAnchor = WinQueryAnchorBlock(WinQueryActiveWindow(HWND_DESKTOP));
	RESULTCODES result = { 0 };
   PROGDETAILS details;
	
	// We get URL
	String_EncodeUtf8(str, args);
	
	// Initialize buffers
	Mem_Set(path, 0, sizeof(path));
	Mem_Set(parambuffer, 0, sizeof(parambuffer));
	Mem_Set(params, 0, sizeof(params));

	// We have to look in the OS/2 configuration for the default browser.
	// First step: Find the configuration files
 	if (!PrfQueryProfile(hAnchor, &profile)) {
		ShowErrorMessage(str);
		return 0;
	}
	
	// Second step: Open the configuration files and read exe path and parameters
	os2Ini = PrfOpenProfile(hAnchor, userPath);
	if (os2Ini == NULLHANDLE) {
		ShowErrorMessage(str);
		return 0;
	}
	if (!PrfQueryProfileString(os2Ini, "WPURLDEFAULTSETTINGS", "DefaultBrowserExe",
		  NULL, path, sizeof(path))) {
		PrfCloseProfile(os2Ini);
		ShowErrorMessage(str);
		return 0;
	}

	PrfQueryProfileString(os2Ini, "WPURLDEFAULTSETTINGS", "DefaultBrowserParameters",
		NULL, params, sizeof(params));
	PrfCloseProfile(os2Ini);

	// concat arguments
	if (strlen(params) > 0) strncat(params, " ", 20);
	strncat(params, str, sizeof(str));

	// Build parameter buffer
	strcpy(parambuffer, "Browser");
	paramptr = &parambuffer[strlen(parambuffer)+1];
	// copy params to buffer
	strcpy(paramptr, params);
	printf("params %p %p %s\n", parambuffer, paramptr, paramptr);
	paramptr += strlen(params) + 1;
	// To be sure: Terminate parameter list with NULL
	*paramptr = '\0';

	// Last step: Execute detached browser
   rc = DosExecPgm(userPath, sizeof(userPath), EXEC_ASYNC,
		parambuffer, NULL, &result, path);
	if (rc != NO_ERROR) {
		ShowErrorMessage(str);
		return 0;
	}
	
	return 0;
}
#else
cc_result Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	char* cmd[3];
	String_EncodeUtf8(str, args);

	/* TODO: Can xdg-open be used on original Solaris, or is it just an OpenIndiana thing */
	cmd[0] = "xdg-open"; cmd[1] = str; cmd[2] = NULL;
	Process_RawStart("xdg-open", cmd);
	return 0;
}
#endif

/* Retrieving exe path is completely OS dependant */
#if defined CC_BUILD_MACOS
static cc_result Process_RawGetExePath(char* path, int* len) {
	Mem_Set(path, '\0', NATIVE_STR_LEN);
	cc_uint32 size = NATIVE_STR_LEN;
	if (_NSGetExecutablePath(path, &size)) return ERR_INVALID_ARGUMENT;

	/* despite what you'd assume, size is NOT changed to length of path */
	*len = String_CalcLen(path, NATIVE_STR_LEN);
	return 0;
}
#elif defined CC_BUILD_LINUX || defined CC_BUILD_SERENITY
static cc_result Process_RawGetExePath(char* path, int* len) {
	*len = readlink("/proc/self/exe", path, NATIVE_STR_LEN);
	return *len == -1 ? errno : 0;
}
#elif defined CC_BUILD_FREEBSD
static cc_result Process_RawGetExePath(char* path, int* len) {
	static int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
	size_t size       = NATIVE_STR_LEN;

	if (sysctl(mib, 4, path, &size, NULL, 0) == -1) return errno;
	*len = String_CalcLen(path, NATIVE_STR_LEN);
	return 0;
}
#elif defined CC_BUILD_OPENBSD
static cc_result Process_RawGetExePath(char* path, int* len) {
	static int mib[4] = { CTL_KERN, KERN_PROC_ARGS, 0, KERN_PROC_ARGV };
	char tmp[NATIVE_STR_LEN];
	size_t size;
	char* argv[100];
	char* str;

	/* NOTE: OpenBSD doesn't seem to let us get executable's location, so fallback to argv[0] */
	/* See OpenBSD sysctl manpage for why argv array is so large: */
	/* "... The buffer pointed to by oldp is filled with an array of char pointers followed by the strings themselves..." */
	mib[2] = getpid();
	size   = 100 * sizeof(char*);
	if (sysctl(mib, 4, argv, &size, NULL, 0) == -1) return errno;

	str = argv[0];
	if (str[0] != '/') {
		/* relative path */
		if (!realpath(str, tmp)) return errno;
		str = tmp;
	}

	*len = String_CalcLen(str, NATIVE_STR_LEN);
	Mem_Copy(path, str, *len);
	return 0;
}
#elif defined CC_BUILD_NETBSD
static cc_result Process_RawGetExePath(char* path, int* len) {
	static int mib[4] = { CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME };
	size_t size       = NATIVE_STR_LEN;

	if (sysctl(mib, 4, path, &size, NULL, 0) == -1) return errno;
	*len = String_CalcLen(path, NATIVE_STR_LEN);
	return 0;
}
#elif defined CC_BUILD_SOLARIS
static cc_result Process_RawGetExePath(char* path, int* len) {
	*len = readlink("/proc/self/path/a.out", path, NATIVE_STR_LEN);
	return *len == -1 ? errno : 0;
}
#elif defined CC_BUILD_HAIKU
static cc_result Process_RawGetExePath(char* path, int* len) {
	image_info info;
	int32 cookie = 0;

	cc_result res = get_next_image_info(B_CURRENT_TEAM, &cookie, &info);
	if (res != B_OK) return res;

	*len = String_CalcLen(info.name, NATIVE_STR_LEN);
	Mem_Copy(path, info.name, *len);
	return 0;
}
#elif defined CC_BUILD_IRIX || defined CC_BUILD_HPUX
static cc_result Process_RawGetExePath(char* path, int* len) {
	static cc_string file = String_FromConst("ClassiCube");

	/* TODO properly get exe path */
	/* Maybe use PIOCOPENM from https://nixdoc.net/man-pages/IRIX/man4/proc.4.html */
	Mem_Copy(path, file.buffer, file.length);
	*len = file.length;
	return 0;
}
#elif defined CC_BUILD_OS2
static cc_result Process_RawGetExePath(char* path, int* len) {
	PPIB pib;
	DosGetInfoBlocks(NULL, &pib);
	if (pib && pib->pib_pchcmd) {
		Mem_Copy(path, pib->pib_pchcmd, strlen(pib->pib_pchcmd));
		*len = strlen(pib->pib_pchcmd);
	}
	return 0;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_FLATPAK
cc_bool Updater_Supported = false;
#else
cc_bool Updater_Supported = true;
#endif

#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
cc_bool Updater_Clean(void) { return true; }

#if defined CC_BUILD_RPI
	#if __aarch64__
	const struct UpdaterInfo Updater_Info = { "", 1, { { "OpenGL ES", "cc-rpi64" } } };
	#else
	const struct UpdaterInfo Updater_Info = { "", 1, { { "OpenGL ES", "ClassiCube.rpi" } } };
	#endif
#elif defined CC_BUILD_LINUX
	#if __x86_64__
	const struct UpdaterInfo Updater_Info = {
		"&eModernGL is recommended for newer machines (2015 or later)", 2,
		{
			{ "ModernGL", "cc-nix64-gl2" },
			{ "OpenGL",   "ClassiCube" }
		}
	};
	#elif __i386__
	const struct UpdaterInfo Updater_Info = {
		"&eModernGL is recommended for newer machines (2015 or later)", 2,
		{
			{ "ModernGL", "cc-nix32-gl2" },
			{ "OpenGL",   "ClassiCube.32" }
		}
	};
	#else
	const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
	#endif
#elif defined CC_BUILD_MACOS
	#if __x86_64__
	const struct UpdaterInfo Updater_Info = {
		"&eModernGL is recommended for newer machines (2015 or later)", 2,
		{
			{ "ModernGL", "cc-osx64-gl2" },
			{ "OpenGL",   "ClassiCube.64.osx" }
		}
	};
	#elif __i386__
	const struct UpdaterInfo Updater_Info = {
		"&eModernGL is recommended for newer machines (2015 or later)", 2,
		{
			{ "ModernGL", "cc-osx32-gl2" },
			{ "OpenGL",   "ClassiCube.osx" }
		}
	};
	#else
	const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
	#endif
#elif defined CC_BUILD_HAIKU
	#if __x86_64__
	const struct UpdaterInfo Updater_Info = { "", 1, { { "OpenGL", "cc-haiku-64" } } };
	#else
	const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
	#endif
#elif defined CC_BUILD_FREEBSD
	#if __x86_64__
	const struct UpdaterInfo Updater_Info = { "", 1, { { "OpenGL", "cc-freebsd-64" } } };
	#elif __i386__
	const struct UpdaterInfo Updater_Info = { "", 1, { { "OpenGL", "cc-freebsd-32" } } };
	#else
	const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
	#endif
#elif defined CC_BUILD_NETBSD
	#if __x86_64__
	const struct UpdaterInfo Updater_Info = { "", 1, { { "OpenGL", "cc-netbsd-64" } } };
	#else
	const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
	#endif
#else
	const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };
#endif

cc_result Updater_Start(const char** action) {
	char path[NATIVE_STR_LEN + 1];
	char* argv[2];
	cc_result res;
	int len = 0;

	*action = "Getting executable path";
	if ((res = Process_RawGetExePath(path, &len))) return res;
	path[len] = '\0';
	
	/* Because the process is only referenced by inode, we can */
	/* just unlink current filename and rename updated file to it */
	*action = "Deleting executable";
	if (unlink(path) == -1) return errno;
	*action = "Replacing executable";
	if (rename(UPDATE_FILE, path) == -1) return errno;

	argv[0] = path; argv[1] = NULL;
	*action = "Restarting game";
	return Process_RawStart(path, argv);
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	char path[NATIVE_STR_LEN + 1];
	struct stat sb;
	int len = 0;

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';

	if (stat(path, &sb) == -1) return errno;
	*timestamp = (cc_uint64)sb.st_mtime;
	return 0;
}

cc_result Updater_MarkExecutable(void) {
	struct stat st;
	if (stat(UPDATE_FILE, &st) == -1) return errno;

	st.st_mode |= S_IXUSR;
	return chmod(UPDATE_FILE, st.st_mode) == -1 ? errno : 0;
}

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	struct utimbuf times = { 0 };
	times.modtime = timestamp;
	return utime(UPDATE_FILE, &times) == -1 ? errno : 0;
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
#if defined MAC_OS_X_VERSION_MIN_REQUIRED && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
/* Really old mac OS versions don't have the dlopen/dlsym API */
const cc_string DynamicLib_Ext = String_FromConst(".dylib");

void* DynamicLib_Load2(const cc_string* path) {
	cc_filepath str;
	Platform_EncodePath(&str, path);
	return NSAddImage(str.buffer, NSADDIMAGE_OPTION_WITH_SEARCHING |
								NSADDIMAGE_OPTION_RETURN_ON_ERROR);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	cc_string tmp; char tmpBuffer[128];
	NSSymbol sym;
	String_InitArray_NT(tmp, tmpBuffer);

	/* NS linker api rquires symbols to have a _ prefix */
	String_Append(&tmp, '_');
	String_AppendConst(&tmp, name);
	tmp.buffer[tmp.length] = '\0';

	sym = NSLookupSymbolInImage(lib, tmp.buffer, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW |
												NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
	return sym ? NSAddressOfSymbol(sym) : NULL;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	NSLinkEditErrors err = 0;
	const char* name = "";
	const char* msg  = "";
	int errNum = 0;

	NSLinkEditError(&err, &errNum, &name, &msg);
	String_Format4(dst, "%c in %c (%i, sys %i)", msg, name, &err, &errNum);
	return true;
}
#else
#include <dlfcn.h>
/* TODO: Should we use .bundle instead of .dylib? */

#ifdef CC_BUILD_DARWIN
const cc_string DynamicLib_Ext = String_FromConst(".dylib");
#else
const cc_string DynamicLib_Ext = String_FromConst(".so");
#endif

void* DynamicLib_Load2(const cc_string* path) {
	cc_filepath str;
	Platform_EncodePath(&str, path);
	return dlopen(str.buffer, RTLD_NOW);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	void *result = dlsym(lib, name);
	return result;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	char* err = dlerror();
	if (err) String_AppendConst(dst, err);
	return err && err[0];
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
static void Platform_InitPosix(void) {
	struct sigaction sa = { 0 };
	sa.sa_handler = SIG_IGN;

	sigaction(SIGCHLD, &sa, NULL);
	/* So writing to closed socket doesn't raise SIGPIPE */
	sigaction(SIGPIPE, &sa, NULL);
}
void Platform_Free(void) { }

#if defined CC_BUILD_IRIX || defined CC_BUILD_HPUX
cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	const char* err = strerror(res);
	if (!err || res >= 1000) return false;

	String_AppendUtf8(dst, err, String_Length(err));
	return true;
}
#else
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
#endif

#if defined CC_BUILD_DARWIN

#if defined CC_BUILD_MACOS
static void Platform_InitSpecific(void) {
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	#ifdef __ppc__
	/* TransformProcessType doesn't work with kCurrentProcess on older macOS */
	GetCurrentProcess(&psn);
	#endif

	/* NOTE: Call as soon as possible, otherwise can't click on dialog boxes or create windows */
	/* NOTE: TransformProcessType is macOS 10.3 or later */
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}
#else
static void Platform_InitSpecific(void) {
	Platform_SingleProcess = true;
	/* Always foreground process on iOS */
}
#endif

void Platform_Init(void) {
	Stopwatch_Init();
	Platform_InitPosix();
	Platform_InitSpecific();
}
#else
void Platform_Init(void) {
	#ifdef CC_BUILD_MOBILE
	Platform_SingleProcess = true;
	#endif
	
	Platform_InitPosix();
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
/* Encrypts data using XTEA block cipher, with OS specific method to get machine-specific key */

static void EncipherBlock(cc_uint32* v, const cc_uint32* key, cc_string* dst) {
	cc_uint32 v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	int i;

    for (i = 0; i < 12; i++) 
	{
        v0  += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1  += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0] = v0; v[1] = v1;
	String_AppendAll(dst, v, 8);
}

static void DecipherBlock(cc_uint32* v, const cc_uint32* key) {
	cc_uint32 v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta * 12;
	int i;

    for (i = 0; i < 12; i++) 
	{
        v1  -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
        sum -= delta;
        v0  -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0] = v0; v[1] = v1;
}

#define ENC1 0xCC005EC0
#define ENC2 0x0DA4A0DE
#define ENC3 0xC0DED000
#define MACHINEID_LEN 32
#define ENC_SIZE 8 /* 2 32 bit ints per block */

/* "b3 c5a-0d9" --> 0xB3C5A0D9 */
static void DecodeMachineID(char* tmp, int len, cc_uint32* key) {
	int hex[MACHINEID_LEN] = { 0 }, i, j, c;
	cc_uint8* dst = (cc_uint8*)key;

	/* Get each valid hex character */
	for (i = 0, j = 0; i < len && j < MACHINEID_LEN; i++) 
	{
		c = PackedCol_DeHex(tmp[i]);
		if (c != -1) hex[j++] = c;
	}

	for (i = 0; i < MACHINEID_LEN / 2; i++) 
	{
		dst[i] = (hex[i * 2] << 4) | hex[i * 2 + 1];
	}
}

#if defined CC_BUILD_LINUX
/* Read /var/lib/dbus/machine-id or /etc/machine-id for the key */
static cc_result GetMachineID(cc_uint32* key) {
	const cc_string idFile  = String_FromConst("/var/lib/dbus/machine-id");
	const cc_string altFile = String_FromConst("/etc/machine-id");
	char tmp[MACHINEID_LEN];
	struct Stream s;
	cc_result res;

	/* Some machines only have dbus id, others only have etc id */
	res = Stream_OpenFile(&s, &idFile);
	if (res) res = Stream_OpenFile(&s, &altFile);
	if (res) return res;

	res = Stream_Read(&s, (cc_uint8*)tmp, MACHINEID_LEN);
	if (!res) DecodeMachineID(tmp, MACHINEID_LEN, key);

	(void)s.Close(&s);
	return res;
}
#elif defined CC_BUILD_MACOS
/* kIOMasterPortDefault is deprecated since macOS 12.0 (replaced with kIOMainPortDefault) */
/* And since kIOMasterPortDefault is just 0/NULL anyways, just manually declare it */
static const mach_port_t masterPortDefault = 0;

/* Read kIOPlatformUUIDKey from I/O registry for the key */
static cc_result GetMachineID(cc_uint32* key) {
	io_registry_entry_t registry;
	CFStringRef devID = NULL;
	char tmp[256] = { 0 };

#ifdef kIOPlatformUUIDKey
    registry = IORegistryEntryFromPath(masterPortDefault, "IOService:/");
    if (!registry) return ERR_NOT_SUPPORTED;

	devID = IORegistryEntryCreateCFProperty(registry, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
	if (devID && CFStringGetCString(devID, tmp, sizeof(tmp), kCFStringEncodingUTF8)) {
		DecodeMachineID(tmp, String_Length(tmp), key);	
	}
#else
    registry = IOServiceGetMatchingService(masterPortDefault,
                                           IOServiceMatching("IOPlatformExpertDevice"));

    devID = IORegistryEntryCreateCFProperty(registry, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
    if (devID && CFStringGetCString(devID, tmp, sizeof(tmp), kCFStringEncodingUTF8)) {
        Mem_Copy(key, tmp, MACHINEID_LEN / 2);
    }
#endif

	if (devID) CFRelease(devID);
	IOObjectRelease(registry);
	return tmp[0] ? 0 : ERR_NOT_SUPPORTED;
}
#elif defined CC_BUILD_FREEBSD
/* Use kern.hostuuid sysctl for the key */
/* Possible alternatives: kenv("smbios.system.uuid"), /etc/hostid */
static cc_result GetMachineID(cc_uint32* key) {
	static int mib[2] = { CTL_KERN, KERN_HOSTUUID };
	char buf[128];
	size_t size = 128;

	if (sysctl(mib, 2, buf, &size, NULL, 0) == -1) return errno;
	DecodeMachineID(buf, size, key);
	return 0;
}
#elif defined CC_BUILD_OPENBSD
/* Use hw.uuid sysctl for the key */
static cc_result GetMachineID(cc_uint32* key) {
	static int mib[2] = { CTL_HW, HW_UUID };
	char buf[128];
	size_t size = 128;

	if (sysctl(mib, 2, buf, &size, NULL, 0) == -1) return errno;
	DecodeMachineID(buf, size, key);
	return 0;
}
#elif defined CC_BUILD_NETBSD
/* Use hw.uuid for the key */
static cc_result GetMachineID(cc_uint32* key) {
	char buf[128];
	size_t size = 128;

	if (sysctlbyname("machdep.dmi.system-uuid", buf, &size, NULL, 0) == -1) return errno;
	DecodeMachineID(buf, size, key);
	return 0;
}
#elif defined CC_BUILD_SOLARIS
/* Use SI_HW_SERIAL for the key */
/* TODO: Should be using SMBIOS UUID for this (search it in illomos source) */
/* NOTE: Got a '0' for serial number when running in a VM */
#ifndef HW_HOSTID_LEN
#define HW_HOSTID_LEN 11
#endif

static cc_result GetMachineID(cc_uint32* key) {
	char host[HW_HOSTID_LEN] = { 0 };
	if (sysinfo(SI_HW_SERIAL, host, sizeof(host)) == -1) return errno;

	DecodeMachineID(host, HW_HOSTID_LEN, key);
	return 0;
}
#elif defined CC_BUILD_ANDROID
static cc_result GetMachineID(cc_uint32* key) {
	cc_string dir; char dirBuffer[STRING_SIZE];
	String_InitArray(dir, dirBuffer);

	JavaCall_Void_String("getUUID", &dir);
	DecodeMachineID(dirBuffer, dir.length, key);
	return 0;
}
#elif defined CC_BUILD_IOS
extern void GetDeviceUUID(cc_string* str);

static cc_result GetMachineID(cc_uint32* key) {
    cc_string str; char strBuffer[STRING_SIZE];
    String_InitArray(str, strBuffer);

    GetDeviceUUID(&str);
    if (!str.length) return ERR_NOT_SUPPORTED;

    DecodeMachineID(strBuffer, str.length, key);
    return 0;
}
#else
static cc_result GetMachineID(cc_uint32* key) { return ERR_NOT_SUPPORTED; }
#endif

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4];
	cc_result res;
	if ((res = GetMachineID(key))) return res;

	header[0] = ENC1; header[1] = ENC2;
	header[2] = ENC3; header[3] = len;
	EncipherBlock(header + 0, key, dst);
	EncipherBlock(header + 2, key, dst);

	for (; len > 0; len -= ENC_SIZE, src += ENC_SIZE) 
	{
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));
		EncipherBlock(header, key, dst);
	}
	return 0;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4];
	cc_result res;
	int dataLen;

	/* Total size must be >= header size */
	if (len < 16) return ERR_END_OF_STREAM;
	if ((res = GetMachineID(key))) return res;

	Mem_Copy(header, src, 16);
	DecipherBlock(header + 0, key);
	DecipherBlock(header + 2, key);

	if (header[0] != ENC1 || header[1] != ENC2 || header[2] != ENC3) return ERR_INVALID_ARGUMENT;
	len -= 16; src += 16;

	if (header[3] > len) return ERR_INVALID_ARGUMENT;
	dataLen = header[3];

	for (; dataLen > 0; len -= ENC_SIZE, src += ENC_SIZE, dataLen -= ENC_SIZE) 
	{
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));

		DecipherBlock(header, key);
		String_AppendAll(dst, header, min(dataLen, ENC_SIZE));
	}
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	int ret;
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) return ERR_NOT_SUPPORTED;
	
	// TODO: check return code? and partial reads?
	ret = read(fd, data, len);
	close(fd);
	return ret == -1 ? errno : 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_MOBILE
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}
#else
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */
	if (gameHasArgs) return GetGameArgs(args);

	#if defined CC_BUILD_MACOS
	/* Sometimes a "-psn_0_[number]" argument is added before actual args */
	if (argc) {
		static const cc_string psn = String_FromConst("-psn_0_");
		cc_string arg0 = String_FromReadonly(argv[0]);
		if (String_CaselessStarts(&arg0, &psn)) { argc--; argv++; }
	}
	#endif

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) 
	{
		/* -d[directory] argument used to change directory data is stored in */
		if (argv[i][0] == '-' && argv[i][1] == 'd' && argv[i][2]) {
			Process_Abort("-d argument no longer supported - cd to desired working directory instead");
			continue;
		}
		args[i] = String_FromReadonly(argv[i]);
	}
	return count;
}

/* Avoid "ignoring return value of 'write' declared with attribute 'warn_unused_result'" warning */
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66425#c34 */
#define IGNORE_RETURN_VALUE(func) (void)!(func)

/* Detects if the game is running in $HOME directory */
static cc_bool IsProblematicWorkingDirectory(void) {
	#ifdef CC_BUILD_MACOS
	/* TODO: Only change working directory when necessary */
	/* When running from bundle, working directory is "/" */
	return true;
	#else
	cc_string curDir, homeDir;
	char path[2048] = { 0 };
	const char* home;

	IGNORE_RETURN_VALUE(getcwd(path, 2048));
	curDir = String_FromReadonly(path);
	
	home = getenv("HOME");
	if (!home) return false;
	homeDir = String_FromReadonly(home);
	
	if (String_Equals(&curDir, &homeDir)) {
		Platform_LogConst("Working directory is $HOME! Changing to executable directory..");
		return true;
	}
	return false;
	#endif
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	char path[NATIVE_STR_LEN];
	int i, len = 0;
	cc_result res;
	if (!IsProblematicWorkingDirectory()) return 0;
	
	res = Process_RawGetExePath(path, &len);
	if (res) return res;

	/* get rid of filename at end of directory */
	for (i = len - 1; i >= 0; i--, len--) {
		if (path[i] == '/') break;
	}

	#ifdef CC_BUILD_MACOS
	static const cc_string bundle = String_FromConst(".app/Contents/MacOS/");
	cc_string raw = String_Init(path, len, 0);

	/* If running from within a bundle, set data folder to folder containing bundle */
	if (String_CaselessEnds(&raw, &bundle)) {
		len -= bundle.length;

		for (i = len - 1; i >= 0; i--, len--) {
			if (path[i] == '/') break;
		}
	}
	#endif

	path[len] = '\0';
	return chdir(path) == -1 ? errno : 0;
}
#endif
#endif
