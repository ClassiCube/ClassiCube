#include "Core.h"
#if defined CC_BUILD_NDS
#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "Options.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <nds/bios.h>
#include <nds/timers.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const char* Platform_AppNameSuffix = " NDS";


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
static u32 rtcSecs;
static void syncRTC(void) { rtcSecs++; }

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	
	return end - beg;
}

cc_uint64 Stopwatch_Measure(void) {
	u16 raw   = timerTick(1);
	u32 usecs = timerTicks2usec(raw);
	return rtcSecs * 1000000ULL + usecs;
}

void Platform_Log(const char* msg, int len) {
	char buffer[256];
	cc_string str;
	String_InitArray(str, buffer);
	
	String_AppendAll(&str, msg, len);
	buffer[str.length] = '\0';
	
	printf("%s\n", buffer);
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
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static const cc_string root_path = String_FromConst("/");

static void GetNativePath(char* str, const cc_string* path) {
	// TODO temp hack
	cc_string path_ = *path;
	int idx = String_IndexOf(path, '/');
	if (idx >= 0) path_ = String_UNSAFE_SubstringAt(&path_, idx + 1);
	
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, &path_);
}

cc_result Directory_Create(const cc_string* path) {
	return ERR_NOT_SUPPORTED;
}

int File_Exists(const cc_string* path) {
	return false;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	*file = -1;
	return ERR_NOT_SUPPORTED;
}

cc_result File_Create(cc_file* file, const cc_string* path) {
	*file = -1;
	return ERR_NOT_SUPPORTED;
}

cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	*file = -1;
	return ERR_NOT_SUPPORTED;
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Close(cc_file file) {
	if (file < 0) return 0;
	return ERR_NOT_SUPPORTED;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	swiDelay(8378 * milliseconds); // TODO probably wrong
}

void* Thread_Create(Thread_StartFunc func) {
	return NULL;
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	// TODO: actual multithreading ???
}

void Thread_Detach(void* handle) {
}

void Thread_Join(void* handle) {
}

void* Mutex_Create(void) {
	return NULL;
}

void Mutex_Free(void* handle) {
}

void Mutex_Lock(void* handle) {
}

void Mutex_Unlock(void* handle) {
}

void* Waitable_Create(void) {
	return NULL;
}

void Waitable_Free(void* handle) {
}

void Waitable_Signal(void* handle) {
}

void Waitable_Wait(void* handle) {
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Connect(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

void Socket_Close(cc_socket s) { }

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Init(void) {
	// Setup a timer that triggers an interrupt once per second
    timerStart(1, ClockDivider_1024, TIMER_FREQ_1024(1), syncRTC);
	
	// TODO: Redesign Drawer2D to better handle this
	Options_SetBool(OPT_USE_CHAT_FONT, true);
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
