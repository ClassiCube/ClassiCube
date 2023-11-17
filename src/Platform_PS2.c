#include "Core.h"
#if defined PLAT_PS2

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
#include <sys/time.h>
#include <kernel.h>
#include <timer_alarm.h>
#include <debug.h>
#include <sifrpc.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = 0x80010006; // ENOENT;
//const cc_result ReturnCode_SocketInProgess  = 0x80010032; // EINPROGRESS
//const cc_result ReturnCode_SocketWouldBlock = 0x80010001; // EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = 0x80010014; // EEXIST

const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const char* Platform_AppNameSuffix = " PS2";


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	
	_print("%s\n", tmp);
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
#define US_PER_SEC 1000000ULL

cc_uint64 Stopwatch_Measure(void) { 
	return GetTimerSystemTime();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) * US_PER_SEC / kBUSCLK;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static const cc_string root_path = String_FromConst("/dev_hdd0/ClassiCube/");

static void GetNativePath(char* str, const cc_string* path) {
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, path);
}

cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	return mkdir(str, 0) == -1 ? errno : 0;
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	struct stat sb;
	GetNativePath(str, path);
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
	/*cc_string path; char pathBuffer[FILENAME_SIZE];
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
	return res;*/
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
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
#define STACK_SIZE (128 * 1024)

void Thread_Sleep(cc_uint32 milliseconds) {
	uint64_t cycles = MSec2TimerBusClock(milliseconds);
	ThreadWaitClock(cycles);
}

static int ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
	
	int thdID = GetThreadId();
	ee_thread_status_t info;
	
	int res = ReferThreadStatus(thdID, &info);
	if (res > 0 && info.stack) Mem_Free(info.stack);
	
	return 0; // TODO detach ?
}

void* Thread_Create(Thread_StartFunc func) {
	ee_thread_t thread = { 0 };
	thread.func        = ExecThread;
	thread.stack       = Mem_Alloc(STACK_SIZE, 1, "Thread stack");
	thread.stack_size  = STACK_SIZE;
	thread.gp_reg      = &_gp;
	thread.initial_priority = 18;
	
	int thdID = CreateThread(&thread);
	if (thdID < 0) Logger_Abort2(thdID, "Creating thread");
	return (void*)thdID;
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	int thdID = (int)handle;
	int res   = StartThread(thdID, (void*)func);
	
	if (res < 0) Logger_Abort2(res, "Running thread");
}

void Thread_Detach(void* handle) {
	// TODO do something
}

void Thread_Join(void* handle) {
	int thdID = (int)handle;
	ee_thread_status_t info;
	
	for (;;)
	{
		int res = ReferThreadStatus(thdID, &info);
		if (res) Logger_Abort("Checking thread status");
		
		if (info.status == THS_DORMANT) break;
		Thread_Sleep(10); // TODO better solution
	}
}

void* Mutex_Create(void) {
	ee_sema_t sema  = { 0 };
	sema.init_count = 1;
	sema.max_count  = 1;
	
	int semID = CreateSema(&sema);
	if (semID < 0) Logger_Abort2(semID, "Creating mutex");
	return (void*)semID;
}

void Mutex_Free(void* handle) {
	int semID = (int)handle;
	int res   = DeleteSema(semID);
	
	if (res) Logger_Abort2(res, "Destroying mutex");
}

void Mutex_Lock(void* handle) {
	int semID = (int)handle;
	int res   = WaitSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int semID = (int)handle;
	int res   = SignalSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(void) {
	ee_sema_t sema  = { 0 };
	sema.init_count = 0;
	sema.max_count  = 1;
	
	int semID = CreateSema(&sema);
	if (semID < 0) Logger_Abort2(semID, "Creating waitable");
	return (void*)semID;
}

void Waitable_Free(void* handle) {
	int semID = (int)handle;
	int res   = DeleteSema(semID);
	
	if (res) Logger_Abort2(res, "Destroying waitable");
}

void Waitable_Signal(void* handle) {
	int semID = (int)handle;
	int res   = SignalSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int semID = (int)handle;
	int res   = WaitSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Signalling event");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	Logger_Abort("Can't wait for");
	// TODO implement support
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
int Socket_ValidAddress(const cc_string* address) {
	return false;
}

cc_result Socket_Connect(cc_socket* s, const cc_string* address, int port, cc_bool nonblocking) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

void Socket_Close(cc_socket s) {
}

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
	//InitDebug();
	SifInitRpc(0);
	//netInitialize();
	// Create root directory
	Directory_Create(&String_Empty);
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
