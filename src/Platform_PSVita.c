#include "Core.h"
#if defined CC_BUILD_PSVITA
#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <vitasdk.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = SCE_NET_ERROR_EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = SCE_NET_ERROR_EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const char* Platform_AppNameSuffix = " PS Vita";
static int epoll_id;
static int stdout_fd;


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}

void Platform_Log(const char* msg, int len) {
	if (!stdout_fd) stdout_fd = sceKernelGetStdout();
	sceIoWrite(stdout_fd, msg, len);
}

#define UnixTime_TotalMS(time) ((cc_uint64)time.sec * 1000 + UNIX_EPOCH + (time.usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct SceKernelTimeval cur;
	sceKernelLibcGettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

void DateTime_CurrentLocal(struct DateTime* t) {
	SceDateTime curTime;
	sceRtcGetCurrentClockLocalTime(&curTime);

	t->year   = curTime.year;
	t->month  = curTime.month;
	t->day    = curTime.day;
	t->hour   = curTime.hour;
	t->minute = curTime.minute;
	t->second = curTime.second;
}

#define US_PER_SEC 1000000ULL
cc_uint64 Stopwatch_Measure(void) {
	// TODO: sceKernelGetSystemTimeWide
	struct SceKernelTimeval cur;
	sceKernelLibcGettimeofday(&cur, NULL);
	return (cc_uint64)cur.sec * US_PER_SEC + cur.usec;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static const cc_string root_path = String_FromConst("ux0:data/ClassiCube/");

static void GetNativePath(char* str, const cc_string* path) {
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, path);
}

#define GetSCEResult(result) (result >= 0 ? 0 : result & 0xFFFF)

cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	int result = sceIoMkdir(str, 0777);
	return GetSCEResult(result);
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	SceIoStat sb;
	GetNativePath(str, path);
	return sceIoGetstat(str, &sb) == 0 && (sb.st_attr & SCE_SO_IFREG) != 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	int res;

	GetNativePath(str, dirPath);
	SceUID uid = sceIoDopen(str);
	if (uid < 0) return GetSCEResult(uid); // error

	String_InitArray(path, pathBuffer);
	SceIoDirent entry;

	while ((res = sceIoDread(uid, &entry)) > 0) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry (PSP does return them)
		char* src = entry.d_name;
		if (src[0] == '.' && src[1] == '\0')                  continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;
		
		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		if (entry.d_stat.st_attr & SCE_SO_IFDIR) {
			res = Directory_Enum(&path, obj, callback);
			if (res) break;
		} else {
			callback(&path, obj);
		}
	}

	sceIoDclose(uid);
	return GetSCEResult(res);
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	int result = sceIoOpen(str, mode, 0777);
	*file      = result;
	return GetSCEResult(result);
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return File_Do(file, path, SCE_O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	return File_Do(file, path, SCE_O_RDWR | SCE_O_CREAT | SCE_O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return File_Do(file, path, SCE_O_RDWR | SCE_O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	int result = sceIoRead(file, data, count);
	*bytesRead = result;
	return GetSCEResult(result);
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	int result  = sceIoWrite(file, data, count);
	*bytesWrote = result;
	return GetSCEResult(result);
}

cc_result File_Close(cc_file file) {
	int result = sceIoDclose(file);
	return GetSCEResult(result);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SCE_SEEK_SET, SCE_SEEK_CUR, SCE_SEEK_END };
	
	int result = sceIoLseek32(file, offset, modes[seekType]);
	return GetSCEResult(result);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int result = sceIoLseek32(file, 0, SCE_SEEK_CUR);
	*pos       = result;
	return GetSCEResult(result);
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int curPos = sceIoLseek32(file, 0, SCE_SEEK_CUR);
	if (curPos < 0) { *len = -1; return GetSCEResult(curPos); }
	
	*len = sceIoLseek32(file, 0, SCE_SEEK_END);
	sceIoLseek32(file, curPos, SCE_SEEK_SET); // restore position
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	sceKernelDelayThread(milliseconds * 1000); 
}

static int ExecThread(unsigned int argc, void *argv) {
	Thread_StartFunc* func = (Thread_StartFunc*)argv;
	(*func)();
	return 0;
}

void* Thread_Create(Thread_StartFunc func) {
	#define CC_THREAD_PRIORITY 0x10000100
	#define CC_THREAD_STACKSIZE 128 * 1024
	#define CC_THREAD_ATTRS 0 // TODO PSP_THREAD_ATTR_VFPU?
	
	return (void*)sceKernelCreateThread("CC thread", ExecThread, CC_THREAD_PRIORITY, 
						CC_THREAD_STACKSIZE, CC_THREAD_ATTRS, 0, NULL);
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	Thread_StartFunc func_ = func;
	sceKernelStartThread((int)handle, sizeof(func_), (void*)&func_);
}

void Thread_Detach(void* handle) {
	sceKernelDeleteThread((int)handle); // TODO don't call this??
}

void Thread_Join(void* handle) {
	sceKernelWaitThreadEnd((int)handle, NULL, NULL);
	sceKernelDeleteThread((int)handle);
}

void* Mutex_Create(void) {
	SceKernelLwMutexWork* ptr = (SceKernelLwMutexWork*)Mem_Alloc(1, sizeof(SceKernelLwMutexWork), "mutex");
	int res = sceKernelCreateLwMutex(ptr, "CC mutex", 0, 0, NULL);
	if (res) Logger_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = sceKernelDeleteLwMutex((SceKernelLwMutexWork*)handle);
	if (res) Logger_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = sceKernelLockLwMutex((SceKernelLwMutexWork*)handle, 1, NULL);
	if (res) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = sceKernelUnlockLwMutex((SceKernelLwMutexWork*)handle, 1);
	if (res) Logger_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(void) {
	int evid = sceKernelCreateEventFlag("CC event", SCE_EVENT_WAITMULTIPLE, 0, NULL);
	if (evid < 0) Logger_Abort2(evid, "Creating waitable");
	return (void*)evid;
}

void Waitable_Free(void* handle) {
	sceKernelDeleteEventFlag((int)handle);
}

void Waitable_Signal(void* handle) {
	int res = sceKernelSetEventFlag((int)handle, 0x1);
	if (res < 0) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	unsigned int match;
	int res = sceKernelWaitEventFlag((int)handle, 0x1, SCE_EVENT_WAITAND | SCE_EVENT_WAITCLEAR, &match, NULL);
	if (res < 0) Logger_Abort2(res, "Event wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	SceUInt timeout = milliseconds * 1000;
	unsigned int match;
	int res = sceKernelWaitEventFlag((int)handle, 0x1, SCE_EVENT_WAITAND | SCE_EVENT_WAITCLEAR, &match, &timeout);
	if (res < 0) Logger_Abort2(res, "Event timed wait");
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct SceNetSockaddr raw;
	struct SceNetSockaddrIn v4;
};

static int ParseHost(union SocketAddress* addr, const char* host) {
	int rid = sceNetResolverCreate("CC resolver", NULL, 0);
	if (rid < 0) return ERR_INVALID_ARGUMENT;

	int ret = sceNetResolverStartNtoa(rid, host, &addr->v4.sin_addr, 1 /* timeout */, 5 /* retries */, 0 /* flags */);
	sceNetResolverDestroy(rid);
	return ret;
}

static int ParseAddress(union SocketAddress* addr, const cc_string* address) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, address);

	if (sceNetInetPton(SCE_NET_AF_INET, str, &addr->v4.sin_addr) > 0) return 0;
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

	*s = sceNetSocket("CC socket", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, SCE_NET_IPPROTO_TCP);
	if (*s < 0) return *s;
	
	if (nonblocking) {
		int on = 1;
		sceNetSetsockopt(*s, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &on, sizeof(int));
	}
	
	addr.v4.sin_family = SCE_NET_AF_INET;
	addr.v4.sin_port   = sceNetHtons(port);

	res = sceNetConnect(*s, &addr.raw, sizeof(addr.v4));
	return res;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = sceNetRecv(s, data, count, 0);
	if (recvCount >= 0) { *modified = recvCount; return 0; }
	
	*modified = 0; 
	return recvCount;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = sceNetSend(s, data, count, 0);
	if (sentCount >= 0) { *modified = sentCount; return 0; }
	
	*modified = 0; 
	return sentCount;
}

void Socket_Close(cc_socket s) {
	sceNetShutdown(s, SCE_NET_SHUT_RDWR);
	sceNetSocketClose(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	SceNetEpollEvent ev = { 0 };
	// to match select, closed socket still counts as readable
	int flags = mode == SOCKET_POLL_READ ? (SCE_NET_EPOLLIN | SCE_NET_EPOLLHUP) : SCE_NET_EPOLLOUT;
	int res, num_events;
	ev.data.fd = s;
	ev.events  = flags;
	
	if ((res = sceNetEpollControl(epoll_id, SCE_NET_EPOLL_CTL_ADD, s, &ev))) return res;	
	num_events = sceNetEpollWait(epoll_id, &ev, 1, 0);
	sceNetEpollControl(epoll_id, SCE_NET_EPOLL_CTL_DEL, s, NULL);
	
	if (num_events < 0)  return num_events;
	if (num_events == 0) return ERR_NOT_SUPPORTED; // TODO when can this ever happen?
	
	*success = (ev.events & flags) != 0;
	return 0;
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	uint32_t resultSize = sizeof(uint32_t);
	cc_result res = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	if (res || *writable) return res;

	// https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation
	sceNetGetsockopt(s, SCE_NET_SOL_SOCKET, SCE_NET_SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

void Platform_Init(void) {
	/*pspDebugSioInit();*/ 
	// TODO: sceNetInit();
	epoll_id = sceNetEpollCreate("CC poll", 0);
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
