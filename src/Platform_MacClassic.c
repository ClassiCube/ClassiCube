#include "Core.h"
#if defined CC_BUILD_MACCLASSIC
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING
#define OVERRIDE_MEM_FUNCTIONS

#include "Stream.h"
#include "ExtMath.h"
#include "SystemFonts.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "PackedCol.h"
#include <string.h>
#include <sys/time.h>

#undef true
#undef false
#include <MacMemory.h>
#include <Processes.h>
#include <Devices.h>
#include <Files.h>
#include <Gestalt.h>

const cc_result ReturnCode_FileShareViolation = 1000000000;
const cc_result ReturnCode_FileNotFound     = fnfErr;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = dupFNErr;
const cc_result ReturnCode_SocketInProgess  = 1000000;
const cc_result ReturnCode_SocketWouldBlock = 1000000;
const cc_result ReturnCode_SocketDropped    = 1000000;
static long sysVersion;

#if TARGET_CPU_68K
const char* Platform_AppNameSuffix = " MAC 68k";
#else
const char* Platform_AppNameSuffix = " MAC PPC";
#endif

cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS;
cc_bool  Platform_ReadonlyFilesystem;
#include "_PlatformBase.h"

/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "main_impl.h"

int main(int argc, char** argv) {
	cc_result res;
	SetupProgram(argc, argv);

	do {
		res = RunProgram(argc, argv);
	} while (Window_Main.Exists);

	Window_Free();
	Process_Exit(res);
	return res;
}


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
static const int MAC_smSystemScript = -1;

// ==================================== IMPORTS FROM TIMER.H ====================================
// Availability: in InterfaceLib 7.1 and later
MAC_SYSAPI(void) Microseconds(cc_uint64* microTickCount) MAC_FOURWORDINLINE(0xA193, 0x225F, 0x22C8, 0x2280);

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
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
void Console_Write(const char* msg, int len);

void Platform_Log(const char* msg, int len) {
	Console_Write(msg, len);
}

// classic macOS uses an epoch of 1904
#define EPOCH_ADJUSTMENT 2082866400UL

static time_t gettod(void) {
    unsigned long secs;
    GetDateTime(&secs);
	return secs - EPOCH_ADJUSTMENT;
}

TimeMS DateTime_CurrentUTC(void) {
	time_t secs = gettod();
	return (cc_uint64)secs + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	struct tm loc_time;
	time_t secs = gettod();
	localtime_r(&secs, &loc_time);

	t->year   = loc_time.tm_year + 1900;
	t->month  = loc_time.tm_mon  + 1;
	t->day    = loc_time.tm_mday;
	t->hour   = loc_time.tm_hour;
	t->minute = loc_time.tm_min;
	t->second = loc_time.tm_sec;
}


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) { }

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#define US_PER_SEC 1000000ULL

cc_uint64 Stopwatch_Measure(void) {
	cc_uint64 count;
	if (sysVersion < 0x7000) {
		// 60 ticks a second
		count = TickCount();
		return count * US_PER_SEC / 60;
	}

	Microseconds(&count);
	return count;
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static int retrievedWD, wd_refNum, wd_dirID;

static void UpdateWorkingDirectory(void) {
	retrievedWD = true;

	WDPBRec r = { 0 };
	PBHGetVolSync(&r);
	wd_refNum = r.ioWDVRefNum;
	wd_dirID  = r.ioWDDirID;

	int V = r.ioWDVRefNum, D = r.ioWDDirID;
	Platform_Log2("Working directory: %i, %i", &V, &D);
}

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* buf = dst->buffer;
	char* str = dst->buffer;
	str++; // placeholder for length later
	*str++ = ':';
	
	// Classic Mac OS uses : to separate directories
	for (int i = 0; i < path->length; i++) 
	{
		char c = (char)path->buffer[i];
		if (c == '/') c = ':';
		*str++ = c;
	}
	*str   = '\0';
	buf[0] = String_Length(buf + 1); // pascal strings

	// TODO not call here? maybe in Platform_Init instead?
	if (retrievedWD) return;
	UpdateWorkingDirectory();
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	String_AppendAll(dst, &path->buffer[1], path->buffer[0]);
}

static int DoOpenDF(const char* name, char perm, cc_file* file) {
    HParamBlockRec pb;
	Mem_Set(&pb, 0, sizeof(pb));

	pb.fileParam.ioVRefNum = wd_refNum;
	pb.fileParam.ioDirID   = wd_dirID;
	pb.fileParam.ioNamePtr = name;
	pb.ioParam.ioPermssn   = perm;

	int err = PBHOpenDFSync(&pb);
	*file   = pb.ioParam.ioRefNum;
	return err;
}

static int DoCreateFile(const char* name) {
    HParamBlockRec pb;
	Mem_Set(&pb, 0, sizeof(pb));

	pb.fileParam.ioVRefNum = wd_refNum;
	pb.fileParam.ioDirID   = wd_dirID;
	pb.fileParam.ioNamePtr = name;

    return PBHCreateSync(&pb);
}

static int DoCreateFolder(const char* name) {
    HParamBlockRec pb;
	Mem_Set(&pb, 0, sizeof(pb));

	pb.fileParam.ioVRefNum = wd_refNum;
	pb.fileParam.ioDirID   = wd_dirID;
	pb.fileParam.ioNamePtr = name;

    return PBDirCreateSync(&pb);
}


void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	return DoCreateFolder(path->buffer);
}

int File_Exists(const cc_filepath* path) {
	return false; // TODO
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return DoOpenDF(path->buffer, fsRdPerm, file);
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	int res = DoCreateFile(path->buffer);
	if (res && res != dupFNErr) return res;

	return DoOpenDF(path->buffer, fsWrPerm, file);
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	int res = DoCreateFile(path->buffer);
	if (res && res != dupFNErr) return res;

	return DoOpenDF(path->buffer, fsRdWrPerm, file);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	ParamBlockRec pb;
	pb.ioParam.ioRefNum   = file;
	pb.ioParam.ioBuffer   = data;
	pb.ioParam.ioReqCount = count;
	pb.ioParam.ioPosMode  = fsAtMark;

	int err = PBReadSync(&pb);
	*bytesRead = pb.ioParam.ioActCount;
	return err;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	ParamBlockRec pb;
	pb.ioParam.ioRefNum   = file;
	pb.ioParam.ioBuffer   = data;
	pb.ioParam.ioReqCount = count;
	pb.ioParam.ioPosMode  = fsAtMark;

	int err = PBWriteSync(&pb);
	*bytesWrote = pb.ioParam.ioActCount;
	return err;
}

cc_result File_Close(cc_file file) {
	ParamBlockRec pb;
	pb.ioParam.ioRefNum = file;

	return PBCloseSync(&pb);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[] = { fsFromStart, fsFromMark, fsFromLEOF };
	ParamBlockRec pb;
	pb.ioParam.ioRefNum    = file;
	pb.ioParam.ioPosMode   = modes[seekType];
	pb.ioParam.ioPosOffset = offset;

	return PBSetFPosSync(&pb);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	ParamBlockRec pb;
	pb.ioParam.ioRefNum = file;

	int err = PBGetFPosSync(&pb);
	*pos    = pb.ioParam.ioPosOffset;
	return err;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	ParamBlockRec pb;
	pb.ioParam.ioRefNum = file;

	int err = PBGetEOFSync(&pb);
	*len    = (cc_uint32)pb.ioParam.ioMisc;
	return err;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) {
	long delay = milliseconds * 1000 / 60;
	long final;
	Delay(delay, &final);
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = false;

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return SetGameArgs(args, numArgs);
}

void Process_Exit(cc_result code) { 
	ExitToShell();
    for(;;) { }
}

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
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
	Gestalt(gestaltSystemVersion, &sysVersion);
	Platform_Log1("Running on Mac OS %h", &sysVersion);

	cc_string path = String_FromConst("aB.txt");
	cc_filepath str;
	Platform_EncodePath(&str, &path);

	int ERR2 = DoCreateFile(&str);
	Platform_Log1("TEST FILE: %i", &ERR2);
}

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
#endif
