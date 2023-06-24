#include "Core.h"
#if defined CC_BUILD_PSP

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
#include <arpa/inet.h>
#include <pspkernel.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <psprtc.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;

PSP_MODULE_INFO("ClassiCube", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

PSP_DISABLE_AUTOSTART_PTHREAD() // reduces .elf size by 140 kb


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
	return end - beg;
}

void Platform_Log(const char* msg, int len) {
	int fd = sceKernelStdout();
	sceIoWrite(fd, msg, len);
	sceIoWrite(fd, "\n",  1);
	
	//sceIoDevctl("emulator:", 2, msg, len, NULL, 0);
	//cc_string str = String_Init(msg, len, len);
	//cc_file file = 0;
	//File_Open(&file, &str);
	//File_Close(file);	
}

#define UnixTime_TotalMS(time) ((cc_uint64)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct SceKernelTimeval cur;
	sceKernelLibcGettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

void DateTime_CurrentLocal(struct DateTime* t) {
	pspTime curTime;
	sceRtcGetCurrentClockLocalTime(&curTime);

	t->year   = curTime.year;
	t->month  = curTime.month;
	t->day    = curTime.day;
	t->hour   = curTime.hour;
	t->minute = curTime.minutes;
	t->second = curTime.seconds;
}

#define US_PER_SEC 1000000ULL
cc_uint64 Stopwatch_Measure(void) {
	// TODO: sceKernelGetSystemTimeWide
	struct SceKernelTimeval cur;
	sceKernelLibcGettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec * US_PER_SEC + cur.tv_usec;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }

extern int __path_absolute(const char *in, char *out, int len);
static void GetNativePath(char* str, const cc_string* path) {
	char tmp[NATIVE_STR_LEN + 1];
	String_EncodeUtf8(tmp, path);
	__path_absolute(tmp, str, NATIVE_STR_LEN);
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
	return sceIoGetstat(str, &sb) == 0 && (sb.st_attr & FIO_SO_IFREG) != 0;
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

		if (entry.d_stat.st_attr & FIO_SO_IFDIR) {
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
	return File_Do(file, path, PSP_O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	return File_Do(file, path, PSP_O_RDWR | PSP_O_CREAT | PSP_O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return File_Do(file, path, PSP_O_RDWR | PSP_O_CREAT);
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
	static cc_uint8 modes[3] = { PSP_SEEK_SET, PSP_SEEK_CUR, PSP_SEEK_END };
	
	int result = sceIoLseek32(file, offset, modes[seekType]);
	return GetSCEResult(result);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int result = sceIoLseek32(file, 0, PSP_SEEK_CUR);
	*pos       = result;
	return GetSCEResult(result);
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int curPos = sceIoLseek32(file, 0, PSP_SEEK_CUR);
	if (curPos < 0) { *len = -1; return GetSCEResult(curPos); }
	
	*len = sceIoLseek32(file, 0, PSP_SEEK_END);
	sceIoLseek32(file, curPos, PSP_SEEK_SET); // restore position
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
	#define CC_THREAD_PRIORITY 17 // TODO: 18?
	#define CC_THREAD_STACKSIZE 128 * 1024
	#define CC_THREAD_ATTRS 0 // TODO PSP_THREAD_ATTR_VFPU?
	
	return (void*)sceKernelCreateThread("CC thread", ExecThread, CC_THREAD_PRIORITY, 
						CC_THREAD_STACKSIZE, CC_THREAD_ATTRS, NULL);
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	Thread_StartFunc func_ = func;
	sceKernelStartThread((int)handle, sizeof(func_), (void*)&func_);
}

void Thread_Detach(void* handle) {
	sceKernelDeleteThread((int)handle); // TODO don't call this??
}

void Thread_Join(void* handle) {
	sceKernelWaitThreadEnd((int)handle, NULL);
	sceKernelDeleteThread((int)handle);
}

void* Mutex_Create(void) {
	SceLwMutexWorkarea* ptr = (SceLwMutexWorkarea*)Mem_Alloc(1, sizeof(SceLwMutexWorkarea), "mutex");
	int res = sceKernelCreateLwMutex(ptr, "CC mutex", 0, 0, NULL);
	if (res) Logger_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = sceKernelDeleteLwMutex((SceLwMutexWorkarea*)handle);
	if (res) Logger_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = sceKernelLockLwMutex((SceLwMutexWorkarea*)handle, 1, NULL);
	if (res) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = sceKernelUnlockLwMutex((SceLwMutexWorkarea*)handle, 1);
	if (res) Logger_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(void) {
	int evid = sceKernelCreateEventFlag("CC event", PSP_EVENT_WAITMULTIPLE, 0, NULL);
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
	u32 match;
	int res = sceKernelWaitEventFlag((int)handle, 0x1, PSP_EVENT_WAITAND | PSP_EVENT_WAITCLEAR, &match, NULL);
	if (res < 0) Logger_Abort2(res, "Event wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	SceUInt timeout = milliseconds * 1000;
	u32 match;
	int res = sceKernelWaitEventFlag((int)handle, 0x1, PSP_EVENT_WAITAND | PSP_EVENT_WAITCLEAR, &match, &timeout);
	if (res < 0) Logger_Abort2(res, "Event timed wait");
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in v4;
};

static int ParseHost(union SocketAddress* addr, const char* host) {
	char buf[1024];
	int rid, ret;

	if (sceNetResolverCreate(&rid, buf, sizeof(buf)) < 0) return 0;

	ret = sceNetResolverStartNtoA(rid, host, &addr->v4.sin_addr, 1 /* timeout */, 5 /* retries */);
	sceNetResolverDelete(rid);
	return ret >= 0;
}

static int ParseAddress(union SocketAddress* addr, const cc_string* address) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, address);

	if (sceNetInetInetPton(AF_INET,str, &addr->v4.sin_addr) > 0) return true;
	return ParseHost(addr, str);
}

int Socket_ValidAddress(const cc_string* address) {
	union SocketAddress addr;
	return ParseAddress(&addr, address);
}

cc_result Socket_Connect(cc_socket* s, const cc_string* address, int port, cc_bool nonblocking) {
	union SocketAddress addr;
	int res;

	*s = -1;
	if (!ParseAddress(&addr, address)) return ERR_INVALID_ARGUMENT;

	*s = sceNetInetSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*s < 0) return sceNetInetGetErrno();
	
	if (nonblocking) {
		int on = 1;
		sceNetInetSetsockopt(*s, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int));
	}
	
	addr.v4.sin_family = AF_INET;
	addr.v4.sin_port   = htons(port);

	res = sceNetInetConnect(*s, &addr.raw, sizeof(addr.v4));
	return res < 0 ? sceNetInetGetErrno() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = sceNetInetRecv(s, data, count, 0);
	if (recvCount < 0) { *modified = recvCount; return 0; }
	
	*modified = 0; 
	return sceNetInetGetErrno();
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = sceNetInetSend(s, data, count, 0);
	if (sentCount < 0) { *modified = sentCount; return 0; }
	
	*modified = 0; 
	return sceNetInetGetErrno();
}

void Socket_Close(cc_socket s) {
	sceNetInetShutdown(s, SHUT_RDWR);
	sceNetInetClose(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct SceNetInetTimeval time = { 0 };
	int selectCount;

	FD_ZERO(&set);
	FD_SET(s, &set);

	if (mode == SOCKET_POLL_READ) {
		selectCount = sceNetInetSelect(s + 1, &set, NULL, NULL, &time);
	} else {
		selectCount = sceNetInetSelect(s + 1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) {
		*success = false; 
		return errno;
	}
	
	*success = FD_ISSET(s, &set) != 0; 
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
	sceNetInetGetsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return ERR_NOT_SUPPORTED;
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


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i]);
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0; /* TODO switch to RomFS ?? */
}
#endif