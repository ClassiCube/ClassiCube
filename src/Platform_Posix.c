#include "Core.h"
#if defined CC_BUILD_POSIX

#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Drawer2D.h"
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

#define Socket__Error() errno
const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;

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
#elif defined CC_BUILD_HAIKU
/* TODO: Use load_image/resume_thread instead of fork */
/* Otherwise opening browser never works because fork fails */
#include <kernel/image.h>
#elif defined CC_BUILD_PSP
/* pspsdk doesn't seem to support IPv6 */
#undef AF_INET6
#include <pspkernel.h>

PSP_MODULE_INFO("ClassiCube", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
#endif


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
/* TODO: check this is actually accurate */
static cc_uint64 sw_freqMul = 1, sw_freqDiv = 1;
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ((end - beg) * sw_freqMul) / sw_freqDiv;
}

#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
void Platform_Log(const char* msg, int len) {
	write(STDOUT_FILENO, msg,  len);
	write(STDOUT_FILENO, "\n",   1);
}
#endif

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

#define NS_PER_SEC 1000000000ULL
/* clock_gettime is optional, see http://pubs.opengroup.org/onlinepubs/009696899/functions/clock_getres.html */
/* "... These functions are part of the Timers option and need not be available on all implementations..." */
#if defined CC_BUILD_DARWIN
cc_uint64 Stopwatch_Measure(void) { return mach_absolute_time(); }
#elif defined CC_BUILD_SOLARIS
cc_uint64 Stopwatch_Measure(void) { return gethrtime(); }
#else
cc_uint64 Stopwatch_Measure(void) {
	struct timespec t;
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (cc_uint64)t.tv_sec * NS_PER_SEC + t.tv_nsec;
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
void Directory_GetCachePath(cc_string* path) { }
#endif

cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	struct stat sb;
	String_EncodeUtf8(str, path);
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	DIR* dirPtr;
	struct dirent* entry;
	char* src;
	int len, res, is_dir;

	String_EncodeUtf8(str, dirPath);
	dirPtr = opendir(str);
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

#if defined CC_BUILD_HAIKU || defined CC_BUILD_SOLARIS
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
	String_EncodeUtf8(str, path);
	*file = open(str, mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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

void* Thread_Create(Thread_StartFunc func) {
	return Mem_Alloc(1, sizeof(pthread_t), "thread");
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_create(ptr, NULL, ExecThread, (void*)func);
	if (res) Logger_Abort2(res, "Creating thread");
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_detach(*ptr);
	if (res) Logger_Abort2(res, "Detaching thread");
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_join(*ptr, NULL);
	if (res) Logger_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(void) {
	pthread_mutex_t* ptr = (pthread_mutex_t*)Mem_Alloc(1, sizeof(pthread_mutex_t), "mutex");
	int res = pthread_mutex_init(ptr, NULL);
	if (res) Logger_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = pthread_mutex_destroy((pthread_mutex_t*)handle);
	if (res) Logger_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = pthread_mutex_lock((pthread_mutex_t*)handle);
	if (res) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = pthread_mutex_unlock((pthread_mutex_t*)handle);
	if (res) Logger_Abort2(res, "Unlocking mutex");
}

struct WaitData {
	pthread_cond_t  cond;
	pthread_mutex_t mutex;
	int signalled; /* For when Waitable_Signal is called before Waitable_Wait */
};

void* Waitable_Create(void) {
	struct WaitData* ptr = (struct WaitData*)Mem_Alloc(1, sizeof(struct WaitData), "waitable");
	int res;
	
	res = pthread_cond_init(&ptr->cond, NULL);
	if (res) Logger_Abort2(res, "Creating waitable");
	res = pthread_mutex_init(&ptr->mutex, NULL);
	if (res) Logger_Abort2(res, "Creating waitable mutex");

	ptr->signalled = false;
	return ptr;
}

void Waitable_Free(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;
	
	res = pthread_cond_destroy(&ptr->cond);
	if (res) Logger_Abort2(res, "Destroying waitable");
	res = pthread_mutex_destroy(&ptr->mutex);
	if (res) Logger_Abort2(res, "Destroying waitable mutex");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	ptr->signalled = true;
	Mutex_Unlock(&ptr->mutex);

	res = pthread_cond_signal(&ptr->cond);
	if (res) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = pthread_cond_wait(&ptr->cond, &ptr->mutex);
		if (res) Logger_Abort2(res, "Waitable wait");
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
		if (res && res != ETIMEDOUT) Logger_Abort2(res, "Waitable wait for");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
}


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj) {
	SysFonts_Register(path);
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
#elif defined CC_BUILD_DARWIN
	static const cc_string dirs[] = {
		String_FromConst("/System/Library/Fonts"),
		String_FromConst("/Library/Fonts")
	};
#elif defined CC_BUILD_SERENITY
	static const cc_string dirs[] = {
		String_FromConst("/res/fonts")
	};
#else
	static const cc_string dirs[] = {
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};
#endif
	for (i = 0; i < Array_Elems(dirs); i++) {
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
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

cc_result Socket_Connect(cc_socket* s, const cc_string* address, int port) {
	int family, addrSize = 0, blocking_raw = -1; /* non-blocking mode */
	union SocketAddress addr;
	cc_result res;

	*s = -1;
	if (!(family = ParseAddress(&addr, address)))
		return ERR_INVALID_ARGUMENT;

	*s = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return errno;
	
	#if defined CC_BUILD_PSP
	int on = 1;
	setsockopt(*s, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int));
	#else
	ioctl(*s, FIONBIO, &blocking_raw);
	#endif

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

#if defined CC_BUILD_DARWIN || defined CC_BUILD_PSP
/* poll is broken on old OSX apparently https://daniel.haxx.se/docs/poll-vs-select.html */
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
	socklen_t resultSize;
	cc_result res = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	if (res || *writable) return res;

	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	getsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
static cc_result Process_RawStart(const char* path, char** argv) {
	pid_t pid = fork();
	if (pid == -1) return errno;

	if (pid == 0) {
		/* Executed in child process */
		execvp(path, argv);
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

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';
	argv[0]   = path;

	for (i = 0, j = 1; i < numArgs; i++, j++) {
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
#elif defined CC_BUILD_HAIKU
cc_result Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	char* cmd[3];
	String_EncodeUtf8(str, args);

	cmd[0] = "open"; cmd[1] = str; cmd[2] = NULL;
	Process_RawStart("open", cmd);
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
#endif


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
const char* const Updater_D3D9 = NULL;
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
		"&eModernGL is recommended for newer machines (2010 or later)", 2,
		{
			{ "ModernGL", "cc-nix64-gl2" },
			{ "OpenGL",   "ClassiCube" }
		}
	};
	#elif __i386__
	const struct UpdaterInfo Updater_Info = {
		"&eModernGL is recommended for newer machines (2010 or later)", 2,
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
		"&eModernGL is recommended for newer machines (2010 or later)", 2,
		{
			{ "ModernGL", "cc-osx64-gl2" },
			{ "OpenGL",   "ClassiCube.64.osx" }
		}
	};
	#elif __i386__
	const struct UpdaterInfo Updater_Info = {
		"&eModernGL is recommended for newer machines (2010 or later)", 2,
		{
			{ "ModernGL", "cc-osx32-gl2" },
			{ "OpenGL",   "ClassiCube.osx" }
		}
	};
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
#if defined CC_BUILD_PSP
/* TODO can this actually be supported somehow */
const cc_string DynamicLib_Ext = String_FromConst(".so");

void* DynamicLib_Load2(const cc_string* path)      { return NULL; }
void* DynamicLib_Get2(void* lib, const char* name) { return NULL; }

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	String_AppendConst(dst, "Dynamic linking unsupported");
	return true;
}
#elif defined MAC_OS_X_VERSION_MIN_REQUIRED && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
/* Really old mac OS versions don't have the dlopen/dlsym API */
const cc_string DynamicLib_Ext = String_FromConst(".dylib");

void* DynamicLib_Load2(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, path);
	return NSAddImage(str, NSADDIMAGE_OPTION_WITH_SEARCHING | 
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
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, path);
	return dlopen(str, RTLD_NOW);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	return dlsym(lib, name);
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
	signal(SIGCHLD, SIG_IGN);
	/* So writing to closed socket doesn't raise SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	/* Assume stopwatch is in nanoseconds */
	/* Some platforms (e.g. macOS) override this */
	sw_freqDiv = 1000;
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

#if defined CC_BUILD_DARWIN
static void Platform_InitStopwatch(void) {
	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);

	sw_freqMul = tb.numer;
	/* tb.denom may be large, so multiplying by 1000 overflows 32 bits */
	/* (one powerpc system had tb.denom of 33329426) */
	sw_freqDiv = (cc_uint64)tb.denom * 1000;
}

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
/* Always foreground process on iOS */
static void Platform_InitSpecific(void) { }
#endif

void Platform_Init(void) {
	Platform_InitPosix();
	Platform_InitStopwatch();
	Platform_InitSpecific();
}
#else
void Platform_Init(void) { Platform_InitPosix(); }
#endif


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
/* Encrypts data using XTEA block cipher, with OS specific method to get machine-specific key */

static void EncipherBlock(cc_uint32* v, const cc_uint32* key, cc_string* dst) {
	cc_uint32 v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	int i;

    for (i = 0; i < 12; i++) {
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

    for (i = 0; i < 12; i++) {
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
	for (i = 0, j = 0; i < len && j < MACHINEID_LEN; i++) {
		c = PackedCol_DeHex(tmp[i]);
		if (c != -1) hex[j++] = c;
	}

	for (i = 0; i < MACHINEID_LEN / 2; i++) {
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

	res = Stream_Read(&s, tmp, MACHINEID_LEN);
	if (!res) DecodeMachineID(tmp, MACHINEID_LEN, key);

	(void)s.Close(&s);
	return res;
}
#elif defined CC_BUILD_MACOS
/* Read kIOPlatformUUIDKey from I/O registry for the key */
static cc_result GetMachineID(cc_uint32* key) {
	io_registry_entry_t registry;
	CFStringRef uuid = NULL;
	char tmp[256] = { 0 };

	registry = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
	if (!registry) return ERR_NOT_SUPPORTED;

#ifdef kIOPlatformUUIDKey
	uuid = IORegistryEntryCreateCFProperty(registry, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
	if (uuid && CFStringGetCString(uuid, tmp, sizeof(tmp), kCFStringEncodingUTF8)) {
		DecodeMachineID(tmp, String_Length(tmp), key);	
	}
#endif
	if (uuid) CFRelease(uuid);
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

	for (; len > 0; len -= ENC_SIZE, src += ENC_SIZE) {
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

	for (; dataLen > 0; len -= ENC_SIZE, src += ENC_SIZE, dataLen -= ENC_SIZE) {
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));

		DecipherBlock(header, key);
		String_AppendAll(dst, header, min(dataLen, ENC_SIZE));
	}
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_ANDROID
/* implemented in Platform_Android.c */
#elif defined CC_BUILD_IOS
/* implemented in interop_ios.m */
#else
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */
	

	#if defined CC_BUILD_MACOS
	/* Sometimes a "-psn_0_[number]" argument is added before actual args */
	if (argc) {
		static const cc_string psn = String_FromConst("-psn_0_");
		cc_string arg0 = String_FromReadonly(argv[0]);
		if (String_CaselessStarts(&arg0, &psn)) { argc--; argv++; }
	}
	#endif

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) {
		/* -d[directory] argument used to change directory data is stored in */
		if (argv[i][0] == '-' && argv[i][1] == 'd' && argv[i][2]) {
			Logger_Abort("-d argument no longer supported - cd to desired working directory instead");
			continue;
		}
		args[i] = String_FromReadonly(argv[i]);
	}
	return count;
}

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

	getcwd(path, 2048);
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
