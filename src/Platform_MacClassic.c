#include "Core.h"
#if defined CC_BUILD_MACCLASSIC

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
#include <string.h>
#include <sys/time.h>

#undef true
#undef false
#include <MacMemory.h>
#include <Processes.h>
#include <Files.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;

const char* Platform_AppNameSuffix = " MAC68k";
cc_bool Platform_SingleProcess = true;


/*########################################################################################################################*
*---------------------------------------------------Imported headers------------------------------------------------------*
*#########################################################################################################################*/
// On 68k these are implemented using direct 68k opcodes
// On PPC these are implemented using function calls
#if TARGET_CPU_68K
	#define MAC_SYSAPI(_type) static _type
    #define MAC_ONEWORDINLINE(w1)           = w1
    #define MAC_TWOWORDINLINE(w1,w2)        = {w1, w2}
    #define MAC_THREEWORDINLINE(w1,w2,w3)   = {w1, w2, w3}
    #define MAC_FOURWORDINLINE(w1,w2,w3,w4) = {w1, w2, w3, w4}
#else
	#define MAC_SYSAPI(_type) extern pascal _type
    #define MAC_ONEWORDINLINE(w1)
    #define MAC_TWOWORDINLINE(w1,w2)
    #define MAC_THREEWORDINLINE(w1,w2,w3)
    #define MAC_FOURWORDINLINE(w1,w2,w3,w4)
#endif
typedef unsigned long MAC_FourCharCode;

// ==================================== IMPORTS FROM TIMER.H ====================================
// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(void) Microseconds(UnsignedWide* microTickCount) MAC_FOURWORDINLINE(0xA193, 0x225F, 0x22C8, 0x2280);

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_Set(void*  dst, cc_uint8 value,  unsigned numBytes) { return memset( dst, value, numBytes); }
void* Mem_Copy(void* dst, const void* src, unsigned numBytes) { return memcpy( dst, src,   numBytes); }
void* Mem_Move(void* dst, const void* src, unsigned numBytes) { return memmove(dst, src,   numBytes); }

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? NewPtr(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? NewPtrClear(size) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	if (!size) return NULL;
	if (!mem)  return NewPtr(size);

	// Try to resize in place
	MemError();
	SetPtrSize(mem, size);
	if (!MemError()) return mem;

	void* newMem = NewPtr(size);
	if (!newMem) return NULL;

	Mem_Copy(newMem, mem, GetPtrSize(mem));
	return newMem;
}

void Mem_Free(void* mem) {
	if (mem) DisposePtr(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
ssize_t _consolewrite(int fd, const void *buf, size_t count);

void Platform_Log(const char* msg, int len) {
	_consolewrite(0, msg,  len);
	_consolewrite(0, "\n",   1);
}

// classic macOS uses an epoch of 1904
#define EPOCH_ADJUSTMENT 2082866400UL

static void gettod(struct timeval *tp) {
    unsigned long secs;
    GetDateTime(&secs);

	tp->tv_sec  = secs - EPOCH_ADJUSTMENT;
	tp->tv_usec = 0;
}

TimeMS DateTime_CurrentUTC(void) {
	struct timeval cur;
	gettod(&cur);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct DateTime* t) {
	struct timeval cur;
	struct tm loc_time;
	gettod(&cur);
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
	//return TickCount();
	UnsignedWide count;
	Microseconds(&count);
	return (cc_uint64)count.lo | ((cc_uint64)count.hi << 32);
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create(const cc_string* path) {
	return ERR_NOT_SUPPORTED;
}

int File_Exists(const cc_string* path) {
	return 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Create(cc_file* file, const cc_string* path) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	long cnt = count;
    int res  = FSRead(file, &cnt, data);

	*bytesRead = cnt;
	return res;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	long cnt = count;
    int res  = FSWrite(file, &cnt, data);

	*bytesWrote = cnt;
	return res;
}

cc_result File_Close(cc_file file) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[] = { fsFromStart, fsFromMark, fsFromLEOF };
	SetFPos(file, modes[seekType], offset);
	return 0;
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
void Thread_Sleep(cc_uint32 milliseconds) { 
	// TODO Delay API
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	*handle = NULL;
	// TODO
}

void Thread_Detach(void* handle) {
	// TODO
}

void Thread_Join(void* handle) {
	// TODO
}

void* Mutex_Create(void) {
	// TODO
	return 1;
}

void Mutex_Free(void* handle) {
	// TODO
}

void Mutex_Lock(void* handle) {
	// TODO
}

void Mutex_Unlock(void* handle) {
	// TODO
}

void* Waitable_Create(void) {
	return 1;
	// TODO
}

void Waitable_Free(void* handle) {
	// TODO
}

void Waitable_Signal(void* handle) {
	// TODO
}

void Waitable_Wait(void* handle) {
	// TODO
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	// TODO
}


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj) {
	SysFonts_Register(path, NULL);
}

void Platform_LoadSysFonts(void) {
	int i;
	static const cc_string dirs[] = {
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};

	for (i = 0; i < Array_Elems(dirs); i++) {
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
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

void Socket_Close(cc_socket s) {

}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = false;
static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int count = gameNumArgs;
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromRawArray(gameArgs[i]);
	}
	
	// clear arguments so after game is closed, launcher is started
	gameNumArgs = 0;
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	for (int i = 0; i < numArgs; i++) 
	{
		String_CopyToRawArray(gameArgs[i], &args[i]);
	}

	gameNumArgs = numArgs;
	return 0;
}

void Process_Exit(cc_result code) { 
	ExitToShell();
    for(;;) { }
}

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Updater_Supported = false;
cc_bool Updater_Clean(void) { return true; }

const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };

cc_result Updater_Start(const char** action) {
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
const cc_string DynamicLib_Ext = String_FromConst(".dylib");

void* DynamicLib_Load2(const cc_string* path) {
	return NULL;
}

void* DynamicLib_Get2(void* lib, const char* name) {
	return NULL;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	return false;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	// TODO
	return false;
}

void Platform_Init(void) {
	Platform_LoadSysFonts();
}

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}
#endif
