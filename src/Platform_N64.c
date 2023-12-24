#include "Core.h"
#if defined CC_BUILD_N64
#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "Options.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libdragon.h>
//#include <dragonfs.h>
//#include <rtc.h>
//#include <timer.h>
#include <unistd.h>
#include <sys/time.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const char* Platform_AppNameSuffix = " N64";


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	
	cc_uint64 delta = end - beg;
	return TIMER_MICROS_LL(delta);
}

cc_uint64 Stopwatch_Measure(void) {
	return timer_ticks();
}

void Platform_Log(const char* msg, int len) {
	write(STDERR_FILENO, msg,  len);
	write(STDERR_FILENO, "\n",   1);
}

#define UnixTime_TotalMS(time) ((cc_uint64)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

void DateTime_CurrentLocal(struct DateTime* t) {
	rtc_time_t curTime = { 0 };
	rtc_get(&curTime);

	t->year   = curTime.year;
	t->month  = curTime.month;
	t->day    = curTime.day;
	t->hour   = curTime.hour;
	t->minute = curTime.min;
	t->second = curTime.sec;
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
	return ERR_NOT_SUPPORTED; // TODO add support
}

static cc_result File_Do(cc_file* file, const cc_string* path) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	//*file = -1;
	//return ReturnCode_FileNotFound;
	// TODO: Why does trying this code break everything
	
	int ret = dfs_open(str);
	Platform_Log2("Opened %c = %i", str, &ret);
	if (ret < 0) { *file = -1; return ret; }
	
	*file = ret;
	return 0;
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return File_Do(file, path);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	*file = -1;
	return ERR_NOT_SUPPORTED;
	//return File_Do(file, path);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	*file = -1;
	return ERR_NOT_SUPPORTED;
	//return File_Do(file, path);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	int ret = dfs_read(data, 1, count, file);
	if (ret < 0) { *bytesRead = -1; return ret; }
	
	*bytesRead = ret;
	return 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Close(cc_file file) {
	if (file < 0) return 0;
	return dfs_close(file);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return dfs_seek(file, offset, modes[seekType]);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int ret = dfs_tell(file);
	if (ret < 0) { *pos = -1; return ret; }
	
	*pos = ret;
	return 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int ret = dfs_size(file);
	if (ret < 0) { *len = -1; return ret; }
	
	*len = ret;
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	wait_ms(milliseconds); 
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
	debug_init_isviewer();
	debug_init_usblog();
	
	// TODO: Redesign Drawer2D to better handle this
	Options_SetBool(OPT_USE_CHAT_FONT, true);
	
    //console_init();
    //console_set_render_mode(RENDER_AUTOMATIC);
    //console_set_debug(true);
	
	dfs_init(DFS_DEFAULT_LOCATION);
	timer_init();
	rtc_init();
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
