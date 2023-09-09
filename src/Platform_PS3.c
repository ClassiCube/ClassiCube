#include "Core.h"
#if defined PLAT_PS3

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
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <signal.h>
#include <net/net.h>
#include <net/poll.h>
#include <ppu-lv2.h>
#include <sys/lv2errno.h>
#include <sys/mutex.h>
#include <sys/sem.h>
#include <sys/thread.h>
#include <sys/systime.h>
#include <sys/tty.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = 0x80010006; // ENOENT;
//const cc_result ReturnCode_SocketInProgess  = 0x80010032; // EINPROGRESS
//const cc_result ReturnCode_SocketWouldBlock = 0x80010001; // EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = 0x80010014; // EEXIST

const cc_result ReturnCode_SocketInProgess  = NET_EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = NET_EWOULDBLOCK;
const char* Platform_AppNameSuffix = " PS3";


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	u32 done = 0;
	sysTtyWrite(STDOUT_FILENO, msg,  len, &done);
	sysTtyWrite(STDOUT_FILENO, "\n", 1,   &done);
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


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#define NS_PER_SEC 1000000000ULL

cc_uint64 Stopwatch_Measure(void) { 
	u64 sec, nsec;
	sysGetCurrentTime(&sec, &nsec);
	return sec * NS_PER_SEC + nsec;
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / 1000;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
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

#if defined CC_BUILD_HAIKU || defined CC_BUILD_SOLARIS || defined CC_BUILD_IRIX || defined CC_BUILD_BEOS
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
void Thread_Sleep(cc_uint32 milliseconds) { 
	sysUsleep(milliseconds * 1000); 
}

static void ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
}
#define STACK_SIZE (128 * 1024)

void* Thread_Create(Thread_StartFunc func) {
	return Mem_Alloc(1, sizeof(sys_ppu_thread_t), "thread");
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	sys_ppu_thread_t* thread = (sys_ppu_thread_t*)handle;
	int res = sysThreadCreate(thread, ExecThread, (void*)func,
			0, STACK_SIZE, THREAD_JOINABLE, "CC thread");
	if (res) Logger_Abort2(res, "Creating thread");
}

void Thread_Detach(void* handle) {
	sys_ppu_thread_t* thread = (sys_ppu_thread_t*)handle;
	int res = sysThreadDetach(*thread);
	if (res) Logger_Abort2(res, "Detaching thread");
	Mem_Free(thread);
}

void Thread_Join(void* handle) {
	u64 retVal;
	sys_ppu_thread_t* thread = (sys_ppu_thread_t*)handle;
	int res = sysThreadJoin(*thread, &retVal);
	if (res) Logger_Abort2(res, "Joining thread");
	Mem_Free(thread);
}

void* Mutex_Create(void) {
	sys_mutex_attr_t attr;	
	sysMutexAttrInitialize(attr);
	
	sys_mutex_t* mutex = (sys_mutex_t*)Mem_Alloc(1, sizeof(sys_mutex_t), "mutex");
	int res = sysMutexCreate(mutex, &attr);
	if (res) Logger_Abort2(res, "Creating mutex");
	return mutex;
}

void Mutex_Free(void* handle) {
	sys_mutex_t* mutex = (sys_mutex_t*)handle;
	int res = sysMutexDestroy(*mutex);
	if (res) Logger_Abort2(res, "Destroying mutex");
	Mem_Free(mutex);
}

void Mutex_Lock(void* handle) {
	sys_mutex_t* mutex = (sys_mutex_t*)handle;
	int res = sysMutexLock(*mutex, 0);
	if (res) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	sys_mutex_t* mutex = (sys_mutex_t*)handle;
	int res = sysMutexUnlock(*mutex);
	if (res) Logger_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(void) {
	sys_sem_attr_t attr = { 0 };
	attr.attr_protocol  = SYS_SEM_ATTR_PROTOCOL;
	attr.attr_pshared   = SYS_SEM_ATTR_PSHARED; 
	
	sys_sem_t* sem = (sys_sem_t*)Mem_Alloc(1, sizeof(sys_sem_t), "waitable");
	int res = sysSemCreate(sem, &attr, 0, 1000000);
	if (res) Logger_Abort2(res, "Creating waitable");
	
	return sem;
}

void Waitable_Free(void* handle) {
	sys_sem_t* sem = (sys_sem_t*)handle;

	int res = sysSemDestroy(*sem);
	if (res) Logger_Abort2(res, "Destroying waitable");
	Mem_Free(sem);
}

void Waitable_Signal(void* handle) {
	sys_sem_t* sem = (sys_sem_t*)handle;
	int res = sysSemPost(*sem, 1);
	if (res) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	sys_sem_t* sem = (sys_sem_t*)handle;
	int res = sysSemWait(*sem, 0);
	if (res) Logger_Abort2(res, "Waitable wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	sys_sem_t* sem = (sys_sem_t*)handle;
	int res = sysSemWait(*sem, milliseconds * 1000);
	if (res) Logger_Abort2(res, "Waitable wait for");
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in v4;
};

static int ParseHost(union SocketAddress* addr, const char* host) {
	struct net_hostent* res = netGetHostByName(host);
	if (!res) return net_h_errno;
	
	// Must have at least one IPv4 address
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)       return ERR_INVALID_ARGUMENT;

	// TODO probably wrong....
	addr->v4.sin_addr = *(struct in_addr*)&res->h_addr_list;
	return 0;
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

	*s  = -1;
	res = ParseAddress(&addr, address);
	if (res) return res;

	res = sysNetSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (res < 0) return res;
	*s  = res;

	// TODO: RPCS3 makes sockets non blocking by default anyways ?
	/*if (nonblocking) {
		int blocking_raw = -1;
		ioctl(*s, FIONBIO, &blocking_raw);
	}*/

	addr.v4.sin_family = AF_INET;
	addr.v4.sin_port   = htons(port);

	return sysNetConnect(*s, &addr.raw, sizeof(addr.v4));
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = sysNetRecvfrom(s, data, count, 0, NULL, NULL);
	if (res < 0) return res;
	
	*modified = res; return 0;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = sysNetSendto(s, data, count, 0, NULL, 0);
	if (res < 0) return res;
	
	*modified = res; return 0;
}

void Socket_Close(cc_socket s) {
	sysNetShutdown(s, SHUT_RDWR);
	sysNetClose(s);
}

LV2_SYSCALL CC_sysNetPoll(struct pollfd* fds, s32 nfds, s32 ms)
{
	lv2syscall3(715, (u64)fds, nfds, ms);
	return_to_user_prog(s32);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags, res;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	
	res = CC_sysNetPoll(&pfd, 1, 0);
	if (res) return res;
	
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
	netGetSockOpt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

void Platform_Init(void) {
	netInitialize();
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
static cc_result GetMachineID(cc_uint32* key) {
	return ERR_NOT_SUPPORTED;
}
#endif