#include "Core.h"
#if defined CC_BUILD_GCWII
#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "PackedCol.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <network.h>
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <ogc/cond.h>
#include <ogc/lwp_watchdog.h>
#include <fat.h>
#ifdef HW_RVL
#include <ogc/wiilaunch.h>
#endif
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = -EINPROGRESS; // net_XYZ error results are negative
const cc_result ReturnCode_SocketWouldBlock = -EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
#ifdef HW_RVL
const char* Platform_AppNameSuffix = " Wii";
#else
const char* Platform_AppNameSuffix = " GameCube";
#endif


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
// dolphin recognises this function name (if loaded as .elf), and will patch it
//  to also log the message to dolphin's console at OSREPORT-HLE log level
void CC_NOINLINE __write_console(int fd, const char* msg, const u32* size) {
	write(STDOUT_FILENO, msg, *size); // this can be intercepted by libogc debug console
}
void Platform_Log(const char* msg, int len) {
	char buffer[256];
	cc_string str = String_Init(buffer, 0, 254); // 2 characters (\n and \0)
	u32 size;
	
	String_AppendAll(&str, msg, len);
	buffer[str.length + 0] = '\n';
	buffer[str.length + 1] = '\0'; // needed to make Dolphin logger happy
	
	size = str.length + 1; // +1 for '\n'
	__write_console(0, buffer, &size); 
	// TODO: Just use printf("%s", somehow ???
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
	return gettime();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ticks_to_microsecs(end - beg);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static char root_buffer[NATIVE_STR_LEN];
static cc_string root_path = String_FromArray(root_buffer);

static bool fat_available; 
// trying to call mkdir etc with no FAT device loaded seems to be broken (dolphin crashes due to trying to execute invalid instruction)
//   https://github.com/Patater/newlib/blob/8a9e3aaad59732842b08ad5fc19e0acf550a418a/libgloss/libsysbase/mkdir.c and
//   https://github.com/Patater/newlib/blob/8a9e3aaad59732842b08ad5fc19e0acf550a418a/newlib/libc/include/sys/iosupport.h
// FindDevice() returns -1 when no matching device, however the code still unconditionally does "if (devoptab_list[dev]->mkdir_r) {"
// - so will either attempt to access or execute invalid memory

static void GetNativePath(char* str, const cc_string* path) {
	Mem_Copy(str, root_path.buffer, root_path.length);
	str   += root_path.length;
	*str++ = '/';
	String_EncodeUtf8(str, path);
}

cc_result Directory_Create(const cc_string* path) {
	if (!fat_available) return ENOSYS;
	
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	return mkdir(str, 0) == -1 ? errno : 0;
}

int File_Exists(const cc_string* path) {
	if (!fat_available) return false;
	
	char str[NATIVE_STR_LEN];
	struct stat sb;
	GetNativePath(str, path);
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	if (!fat_available) return ENOSYS;

	cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	struct dirent* entry;
	int res;

	GetNativePath(str, dirPath);
	DIR* dirPtr = opendir(str);
	if (!dirPtr) return errno;

	// POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed."
	// errno is sometimes leftover from previous calls, so always reset it before readdir gets called
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry
		char* src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);
		int is_dir = entry->d_type == DT_DIR;
		// TODO: fallback to stat when this fails

		if (is_dir) {
			res = Directory_Enum(&path, obj, callback);
			if (res) { closedir(dirPtr); return res; }
		} else {
			callback(&path, obj);
		}
		errno = 0;
	}

	res = errno; // return code from readdir
	closedir(dirPtr);
	return res;
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	if (!fat_available) return ENOSYS;
	
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	*file = open(str, mode, 0);
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

static void* ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
	return NULL;
}

void* Thread_Create(Thread_StartFunc func) {
	return Mem_Alloc(1, sizeof(lwp_t), "thread");
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	lwp_t* ptr = (lwp_t*)handle;
	int res = LWP_CreateThread(ptr, ExecThread, (void*)func, NULL, 256 * 1024, 80);
	if (res) Logger_Abort2(res, "Creating thread");
}

void Thread_Detach(void* handle) {
	// TODO: Leaks return value of thread ???
	lwp_t* ptr = (lwp_t*)handle;
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	lwp_t* ptr = (lwp_t*)handle;
	int res = LWP_JoinThread(*ptr, NULL);
	if (res) Logger_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(void) {
	mutex_t* ptr = (mutex_t*)Mem_Alloc(1, sizeof(mutex_t), "mutex");
	int res = LWP_MutexInit(ptr, false);
	if (res) Logger_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	mutex_t* mutex = (mutex_t*)handle;
	int res = LWP_MutexDestroy(*mutex);
	if (res) Logger_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	mutex_t* mutex = (mutex_t*)handle;
	int res = LWP_MutexLock(*mutex);
	if (res) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	mutex_t* mutex = (mutex_t*)handle;
	int res = LWP_MutexUnlock(*mutex);
	if (res) Logger_Abort2(res, "Unlocking mutex");
}

// should really use a semaphore with max 1.. too bad no 'TimedWait' though
struct WaitData {
	cond_t  cond;
	mutex_t mutex;
	int signalled; // For when Waitable_Signal is called before Waitable_Wait
};

void* Waitable_Create(void) {
	struct WaitData* ptr = (struct WaitData*)Mem_Alloc(1, sizeof(struct WaitData), "waitable");
	int res;
	
	res = LWP_CondInit(&ptr->cond);
	if (res) Logger_Abort2(res, "Creating waitable");
	res = LWP_MutexInit(&ptr->mutex, false);
	if (res) Logger_Abort2(res, "Creating waitable mutex");

	ptr->signalled = false;
	return ptr;
}

void Waitable_Free(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;
	
	res = LWP_CondDestroy(ptr->cond);
	if (res) Logger_Abort2(res, "Destroying waitable");
	res = LWP_MutexDestroy(ptr->mutex);
	if (res) Logger_Abort2(res, "Destroying waitable mutex");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	ptr->signalled = true;
	Mutex_Unlock(&ptr->mutex);

	res = LWP_CondSignal(ptr->cond);
	if (res) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = LWP_CondWait(ptr->cond, ptr->mutex);
		if (res) Logger_Abort2(res, "Waitable wait");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	struct WaitData* ptr = (struct WaitData*)handle;
	struct timespec ts;
	int res;

	ts.tv_sec  = milliseconds / 1000;
	ts.tv_nsec = milliseconds % 1000;

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = LWP_CondTimedWait(ptr->cond, ptr->mutex, &ts);
		if (res && res != ETIMEDOUT) Logger_Abort2(res, "Waitable wait for");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in v4;
};

static int ParseHost(union SocketAddress* addr, const char* host) {
#ifdef HW_RVL
	struct hostent* res = net_gethostbyname(host);
	// avoid confusion with SSL error codes
	// e.g. FFFF FFF7 > FF00 FFF7
	if (!res) return -0xFF0000 + errno;
	
	// Must have at least one IPv4 address
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list[0])       return ERR_INVALID_ARGUMENT;

	addr->v4.sin_addr = *(struct in_addr*)res->h_addr_list[0];
	return 0;
#else
	// DNS resolution not implemented in gamecube libbba
	return ERR_NOT_SUPPORTED;
#endif
}

static int ParseAddress(union SocketAddress* addr, const cc_string* address) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, address);

	if (inet_aton(str, &addr->v4.sin_addr) > 0) return 0;
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

	*s = net_socket(AF_INET, SOCK_STREAM, 0);
	if (*s < 0) return *s;

	if (nonblocking) {
		int blocking_raw = -1; /* non-blocking mode */
		net_ioctl(*s, FIONBIO, &blocking_raw);
	}

	addr.v4.sin_family = AF_INET;
	addr.v4.sin_port   = htons(port);

	res = net_connect(*s, &addr.raw, sizeof(addr.v4));
	return res < 0 ? res : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = net_recv(s, data, count, 0);
	if (res < 0) { *modified = 0; return res; }
	
	*modified = res; return 0;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = net_send(s, data, count, 0);
	if (res < 0) { *modified = 0; return res; }
	
	*modified = res; return 0;
}

void Socket_Close(cc_socket s) {
	net_shutdown(s, 2); // SHUT_RDWR = 2
	net_close(s);
}

#ifdef HW_RVL
// libogc only implements net_poll for wii currently
static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollsd pfd;
	pfd.socket = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	
	int res = net_poll(&pfd, 1, 0);
	if (res < 0) { *success = false; return res; }
	
	// to match select, closed socket still counts as readable
	int flags = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success  = (pfd.revents & flags) != 0;
	return 0;
}
#else
// libogc only implements net_select for gamecube currently
static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int res; // number of 'ready' sockets
	FD_ZERO(&set);
	FD_SET(s, &set);
	if (mode == SOCKET_POLL_READ) {
		res = net_select(s + 1, &set, NULL, NULL, &time);
	} else {
		res = net_select(s + 1, NULL, &set, NULL, &time);
	}
	if (res < 0) { *success = false; return res; }
	*success = FD_ISSET(s, &set) != 0; return 0;
}
#endif

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	u32 resultSize = sizeof(u32);
	cc_result res  = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	if (res || *writable) return res;

	return 0;
	// TODO FIX with updated devkitpro ???
	
	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	net_getsockopt(s, SOL_SOCKET, SO_ERROR, &res, resultSize);
	return res;
}
static void InitSockets(void) {
#ifdef HW_RVL
	int ret = net_init();
	Platform_Log1("Network setup result: %i", &ret);
#else
	// https://github.com/devkitPro/wii-examples/blob/master/devices/network/sockettest/source/sockettest.c
	char localip[16] = {0};
	char netmask[16] = {0};
	char gateway[16] = {0};
	
	int ret = if_config(localip, netmask, gateway, TRUE, 20);
	if (ret >= 0) {
		Platform_Log3("Network ip: %c, gw: %c, mask %c", localip, gateway, netmask);
	} else {
		Platform_Log1("Network setup failed: %i", &ret);
	}
#endif
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#ifdef HW_RVL
cc_result Process_StartOpen(const cc_string* args) {
	char url[NATIVE_STR_LEN];
	String_EncodeUtf8(url, args);
	
	// TODO: Not sure if this works or not
	return WII_OpenURL(url);
}
#else
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}
#endif

static void AppendDevice(cc_string* path, char* cwd) {
	// try to find device FAT mounted on, otherwise default to SD card
	if (!cwd) { String_AppendConst(path, "sd"); return;	}
	
	Platform_Log1("CWD: %c", cwd);
	cc_string cwd_ = String_FromReadonly(cwd);
	int deviceEnd  = String_IndexOf(&cwd_, ':');
		
	if (deviceEnd >= 0) {
		// e.g. "card0:/" becomes "card0"
		String_AppendAll(path, cwd, deviceEnd);
	} else {
		String_AppendConst(path, "sd");
	}
}

static void FindRootDirectory(void) {
	char cwdBuffer[NATIVE_STR_LEN] = { 0 };
	char* cwd = getcwd(cwdBuffer, NATIVE_STR_LEN);
	
	root_path.length = 0;
	AppendDevice(&root_path, cwd);
	String_AppendConst(&root_path, ":/ClassiCube");
}

static void CreateRootDirectory(void) {
	if (!fat_available) return;
	root_buffer[root_path.length] = '\0';
	
	// irectory_Create(&String_Empty); just returns error 20
	int res = mkdir(root_buffer, 0);
	int err = res == -1 ? errno : 0;
	Platform_Log1("Created root directory: %i", &err);
}

void Platform_Init(void) {
	fat_available = fatInitDefault();
	FindRootDirectory();
	CreateRootDirectory();
	
	InitSockets();
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
#if defined HW_RVL
#include <ogc/es.h>

static cc_result GetMachineID(cc_uint32* key) {
	return ES_GetDeviceID(key);
}
#else
static cc_result GetMachineID(cc_uint32* key) {
	return ERR_NOT_SUPPORTED;
}
#endif

#endif