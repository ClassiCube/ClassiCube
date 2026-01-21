#include "Core.h"
#if defined CC_BUILD_N64
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING

#define CC_XTEA_ENCRYPTION
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
#include <cop1.h>
//#include <dragonfs.h>
//#include <rtc.h>
//#include <timer.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/fcntl.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

const char* Platform_AppNameSuffix  = " N64";
cc_bool Platform_ReadonlyFilesystem = false;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "main_impl.h"

int main(int argc, char** argv) {
	SetupProgram(argc, argv);
	while (Window_Main.Exists) { 
		RunProgram(argc, argv);
	}
	
	Window_Free();
	return 0;
}


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

TimeMS DateTime_CurrentUTC(void) {
	return 0;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
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
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) { }

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static cc_string root_path = String_FromConst("sd:/ClassiCube/");
static cc_string root_path_rom = String_FromConst("rom:/");

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	cc_string path_ = *path;

	if (Platform_ReadonlyFilesystem) { // read from rom root
		int idx = String_IndexOf(path, '/');
		if (idx >= 0) path_ = String_UNSAFE_SubstringAt(&path_, idx + 1);
	}
	
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, &path_);
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	const char* str = path->buffer;
	String_AppendUtf8(dst, str, String_Length(str));
}

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	return mkdir(path->buffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

int File_Exists(const cc_filepath* path) {
	struct stat sb;
	return stat(path->buffer, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	dir_t entry;

	Platform_EncodePath(&str, dirPath);
	int res = dir_findfirst(str.buffer, &entry);
	if (res != 0) return errno;

	errno = 0;
	String_InitArray(path, pathBuffer);

	while (res == 0) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		char* src = entry.d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		int is_dir = entry.d_type == DT_DIR;
		callback(&path, obj, is_dir);
		errno = 0;
		res = dir_findnext(str.buffer, &entry);
	}

	return errno; // no clue if libdragon actually sets errno...
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	*file = open(path, mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	return *file == -1 ? errno : 0;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, O_RDWR | O_CREAT | O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, O_RDWR | O_CREAT);
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
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	wait_ms(milliseconds); 
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
// See src/n64sys.c
static void DisableFpuExceptions(void) {
    uint32_t fcr31 = C1_FCR31();
    
    fcr31 &= ~(C1_ENABLE_OVERFLOW | C1_ENABLE_UNDERFLOW | C1_ENABLE_INEXACT_OP);
    fcr31 |= C1_ENABLE_DIV_BY_0 | C1_ENABLE_INVALID_OP;
    fcr31 |= C1_FCR31_FS; // flush denormals to zero

    C1_WRITE_FCR31(fcr31);	
}

void Platform_Init(void) {
	debug_init_isviewer();
	debug_init_usblog();
	DisableFpuExceptions();
	
	dfs_init(DFS_DEFAULT_LOCATION);
	timer_init();
	rtc_init();

	if (!debug_init_sdfs("sd:/", -1)) {
		String_Copy(&root_path, &root_path_rom);
		Platform_ReadonlyFilesystem = true;
		Platform_LogConst("Couldn't access SD card.");
	}
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

cc_bool Process_OpenSupported = false;
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

void CPU_FlushDataCache(void* start, int length) {
	data_cache_hit_writeback(start, length);
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "N64_N64_N64_N64_"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
#endif
