#include "Core.h"
#if defined CC_BUILD_DREAMCAST
#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "SystemFonts.h"

#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>
#include <ppp/ppp.h>
#include <kos.h>
#include <dc/sd.h>
#include <fat/fs_fat.h>
#include <kos/dbgio.h>
#include "_PlatformConsole.h"

KOS_INIT_FLAGS(INIT_CONTROLLER | INIT_KEYBOARD | INIT_MOUSE |
               INIT_VMU        | INIT_CDROM    | INIT_NET);

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

const char* Platform_AppNameSuffix = " Dreamcast";
cc_bool Platform_ReadonlyFilesystem;
static cc_bool usingSD;


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}

cc_uint64 Stopwatch_Measure(void) {
	return timer_us_gettime64();
}

static uint32 str_offset;
static cc_bool log_debugger  = true;
static cc_bool log_timestamp = true;
extern cc_bool window_inited;

#define MAX_ONSCREEN_LINES 17
#define ONSCREEN_LINE_HEIGHT (24 + 2) // 8 * 3 text, plus 2 pixel padding
#define Onscreen_LineOffset(y) ((10 + y + (str_offset * ONSCREEN_LINE_HEIGHT)) * vid_mode->width)

static void PlotOnscreen(int x, int y, void* ctx) {
	if (x >= vid_mode->width) return;
	
	uint32 line_offset = Onscreen_LineOffset(y);
	vram_s[line_offset + x] = 0xFFFF;
}

static void LogOnscreen(const char* msg, int len) {
	char buffer[50];
	cc_string str;
	uint32 secs, ms;
	timer_ms_gettime(&secs, &ms);
	
	String_InitArray(str, buffer);
	if (log_timestamp) String_Format2(&str, "[%p2.%p3] ", &secs, &ms);
	String_AppendAll(&str, msg, len);

	short* dst     = vram_s + Onscreen_LineOffset(0);
	int num_pixels = ONSCREEN_LINE_HEIGHT * 2 * vid_mode->width;
	for (int i = 0; i < num_pixels; i++) dst[i] = 0;
	//sq_set16(vram_s + Onscreen_LineOffset(0), 0, ONSCREEN_LINE_HEIGHT * 2 * vid_mode->width);
	
	FallbackFont_Plot(&str, PlotOnscreen, 3, NULL);
	str_offset = (str_offset + 1) % MAX_ONSCREEN_LINES;
}

void Platform_Log(const char* msg, int len) {
	if (log_debugger) {
		dbgio_write_buffer_xlat(msg,  len);
		dbgio_write_buffer_xlat("\n",   1);
	}
	
	if (window_inited) return;
	// Log details on-screen for initial modem initing etc
	//  (this can take around 40 seconds on average)
	LogOnscreen(msg, len);
}

TimeMS DateTime_CurrentUTC(void) {
	uint32 secs, ms;
	timer_ms_gettime(&secs, &ms);
	
	time_t boot_time  = rtc_boot_time();
	cc_uint64 curSecs = boot_time + secs;
	return curSecs + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	uint32 secs, ms;
	time_t total_secs;
	struct tm loc_time;
	
	timer_ms_gettime(&secs, &ms);
	total_secs = rtc_boot_time() + secs;
	localtime_r(&total_secs, &loc_time);

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
static void HandleCrash(irq_t evt, irq_context_t* ctx, void* data) {
	uint32_t code = evt;
	log_timestamp = false;
	window_inited = false;
	str_offset    = 0;

	for (;;)
	{
		Platform_LogConst("** CLASSICUBE FATALLY CRASHED **");
		Platform_Log2("PC: %h, error: %h",
						&ctx->pc, &code);
		Platform_LogConst("");
	
		static const char* const regNames[] = {
			"R0 ", "R1 ", "R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ",
			"R8 ", "R9 ", "R10", "R11", "R12", "R13", "R14", "R15"
		};
	
		for (int i = 0; i < 8; i++) {
			Platform_Log4("    %c: %h    %c: %h",
						regNames[i],     &ctx->r[i],
						regNames[i + 8], &ctx->r[i + 8]);
		}
	
		Platform_Log4("    %c : %h    %c : %h",
					"SR", &ctx->sr,
					"PR", &ctx->pr);
	
		Platform_LogConst("");
		Platform_LogConst("Please report on ClassiCube Discord or forums");
		Platform_LogConst("");
		Platform_LogConst("You will need to restart your Dreamcast");
		Platform_LogConst("");
		
		// Only log to serial/emu console first time
		log_debugger = false;
	}
}

void CrashHandler_Install(void) {
	irq_set_handler(EXC_UNHANDLED_EXC, HandleCrash, NULL);
}

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*----------------------------------------------------VMU options file-----------------------------------------------------*
*#########################################################################################################################*/
static const cc_uint8 icon_pal[] = {
0xff,0xff, 0xdd,0xfd, 0x00,0x50, 0x33,0xf3, 0xee,0xfe, 0xcc,0xfc, 0xbb,0xfb, 0x00,0x40, 
0x88,0xf8, 0x00,0xb0, 0x22,0xf2, 0x00,0xb0, 0x00,0xf0, 0x00,0x30, 0x00,0x00, 0x00,0xf0,
};
static const cc_uint8 icon_data[] = {
0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xd9, 0x97, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xd9, 0xc3, 0x3c, 0x97, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xee, 0xd9, 0xc3, 0x60, 0x06, 0x3c, 0x97, 0xee, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xd9, 0xc3, 0x60, 0x00, 0x00, 0x06, 0x3c, 0x97, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xd9, 0xc3, 0x60, 0x00, 0x00, 0x00, 0x00, 0x06, 0x3c, 0x97, 0xee, 0xee, 0xee,
0xee, 0xee, 0xd9, 0xc3, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x3c, 0x97, 0xee, 0xee,
0xee, 0xd9, 0xc3, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x3c, 0x97, 0xee,
0xe7, 0xc3, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x3c, 0x7e,
0xe2, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x2e,
0xe2, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x11, 0x56, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x10, 0x00, 0x16, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x56, 0x66, 0x64, 0x00, 0x00, 0x00, 0x16, 0x64, 0x00, 0x00, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x04, 0x66, 0x66, 0x66, 0x40, 0x00, 0x16, 0x66, 0x40, 0x04, 0x15, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x01, 0x66, 0x14, 0x16, 0x60, 0x00, 0x66, 0x65, 0x00, 0x56, 0x66, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x05, 0x65, 0x00, 0x04, 0x64, 0x00, 0x66, 0x64, 0x04, 0x66, 0x66, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x05, 0x65, 0x00, 0x00, 0x00, 0x00, 0x66, 0x50, 0x05, 0x66, 0x66, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x05, 0x65, 0x00, 0x00, 0x00, 0x00, 0x66, 0x10, 0x06, 0x66, 0x66, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x01, 0x65, 0x00, 0x00, 0x00, 0x00, 0x66, 0x40, 0x46, 0x66, 0x66, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x04, 0x65, 0x00, 0x00, 0x00, 0x00, 0x66, 0x40, 0x46, 0x66, 0x65, 0x66, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x56, 0x40, 0x00, 0x00, 0x00, 0x66, 0x00, 0x06, 0x66, 0x50, 0x56, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x16, 0x50, 0x00, 0x00, 0x00, 0x66, 0x40, 0x05, 0x65, 0x40, 0x16, 0x6c, 0x2e,
0xe2, 0xc0, 0x00, 0x05, 0x61, 0x00, 0x00, 0x00, 0x66, 0x40, 0x00, 0x40, 0x00, 0x66, 0x6c, 0x2e,
0xe7, 0xc3, 0x60, 0x04, 0x66, 0x55, 0x50, 0x00, 0x66, 0x50, 0x00, 0x00, 0x05, 0x68, 0xac, 0x7e,
0xee, 0xd9, 0xc3, 0x60, 0x45, 0x66, 0x64, 0x00, 0x66, 0x64, 0x00, 0x04, 0x58, 0xac, 0x97, 0xee,
0xee, 0xee, 0xd9, 0xc3, 0x60, 0x15, 0x54, 0x00, 0x66, 0x66, 0x55, 0x58, 0xac, 0x97, 0xee, 0xee,
0xee, 0xee, 0xee, 0xd9, 0xc3, 0x60, 0x00, 0x00, 0x66, 0x66, 0x68, 0xac, 0x97, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xd9, 0xc3, 0x60, 0x00, 0x66, 0x68, 0xac, 0x97, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xee, 0xd9, 0xc3, 0x60, 0x68, 0xac, 0x97, 0xee, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xd9, 0xc3, 0xac, 0x97, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xd9, 0x97, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
};

static volatile int vmu_write_FD = -10000;
static int VMUFile_Do(cc_file* file, int mode) {
	void* data = NULL;
	int fd, err = -1, len;
	vmu_pkg_t pkg;
	
	errno = 0;
	fd    = fs_open("/vmu/a1/CCOPT.txt", O_RDONLY);
	
	// Try to extract stored data from the VMU
	if (fd >= 0) {
		len  = fs_total(fd);
		data = Mem_Alloc(len, 1, "VMU data");
		fs_read(fd, data, len);
		
		err = vmu_pkg_parse(data, &pkg);
		fs_close(fd);
	}
	
	// Copy VMU data into a RAM temp file
	errno = 0;
	fd    = fs_open("/ram/ccopt", O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0) return errno;
	
	if (err >= 0) {
		fs_write(fd, pkg.data, pkg.data_len);
		fs_seek(fd,  0, SEEK_SET);
	}
	Mem_Free(data);

	if (mode != O_RDONLY) vmu_write_FD = fd;
	*file = fd;
	return 0;
}

static cc_result VMUFile_Close(cc_file file) {
	void* data;
	uint8* pkg_data;
	int fd, err = -1, len, pkg_len;
	vmu_pkg_t pkg = { 0 };
	vmu_write_FD  = -10000;
	
	len  = fs_total(file);
	data = Mem_Alloc(len, 1, "VMU data");
	fs_seek(file, 0, SEEK_SET);
	fs_read(file, data, len);
	
	fs_close(file);
	fs_unlink("/ram/ccopt");
	
	strcpy(pkg.desc_short, "CC options file");
	strcpy(pkg.desc_long,  "ClassiCube config/settings");
	strcpy(pkg.app_id,     "ClassiCube");
	pkg.eyecatch_type = VMUPKG_EC_NONE;
	pkg.data_len      = len;
	pkg.data          = data;

	pkg.icon_cnt  = 1;
	pkg.icon_data = icon_data;
	Mem_Copy(pkg.icon_pal, icon_pal, sizeof(icon_pal));
		
	err = vmu_pkg_build(&pkg, &pkg_data, &pkg_len);
	if (err) { Mem_Free(data); return ERR_OUT_OF_MEMORY; }
	
	// Copy into VMU file
	errno = 0;
	fd    = fs_open("/vmu/a1/CCOPT.txt", O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0) return errno;
	
	fs_write(fd, pkg_data, pkg_len);
	fs_close(fd);
	free(pkg_data);
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static cc_string root_path = String_FromConst("/cd/");

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, path);
}

cc_result Directory_Create(const cc_filepath* path) {
	int res = fs_mkdir(path->buffer);
	int err = res == -1 ? errno : 0;
	
	// Filesystem returns EINVAL when operation unsupported (e.g. CD system)
	//  so rather than logging an error, just pretend it already exists
	if (err == EINVAL) err = EEXIST;

	// Changes are cached in memory, sync to SD card
	// TODO maybe use fs_shutdown/fs_unmount and only sync once??
	if (!err) fs_fat_sync("/sd");
	return err;
}

int File_Exists(const cc_filepath* path) {
	struct stat sb;
	return fs_stat(path->buffer, &sb, 0) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	// CD filesystem loader doesn't usually set errno
	//  when it can't find the requested file
	errno = 0;

	Platform_EncodePath(&str, dirPath);
	int fd = fs_open(str.buffer, O_DIR | O_RDONLY);
	if (fd < 0) return errno;

	String_InitArray(path, pathBuffer);
	dirent_t* entry;
	errno = 0;
	
	while ((entry = fs_readdir(fd))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry (PSP does return them)
		// TODO: Does Dreamcast?
		char* src = entry->name;
		if (src[0] == '.' && src[1] == '\0')                  continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;
		
		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		// negative size indicates a directory entry
		int is_dir = entry->size < 0;
		callback(&path, obj, is_dir);
	}

	int err = errno; // save error from fs_readdir
	fs_close(fd);
	return err;
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	// CD filesystem loader doesn't usually set errno
	//  when it can't find the requested file
	errno = 0;

	int res = fs_open(path, mode);
	*file   = res;
	
	int err = res == -1 ? errno : 0;
	if (res == -1 && err == 0) err = ENOENT;

	// Read/Write VMU for options.txt if no SD card, since that file is critical
	cc_string raw = String_FromReadonly(path);
	if (err && String_CaselessEqualsConst(&raw, "/cd/options.txt")) {
		return VMUFile_Do(file, mode);
	}
	return err;
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
	int res    = fs_read(file, data, count);
	*bytesRead = res;
	return res == -1 ? errno : 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	int res     = fs_write(file, data, count);
	*bytesWrote = res;
	return res == -1 ? errno : 0;
}

cc_result File_Close(cc_file file) {
	if (file == vmu_write_FD) 
		return VMUFile_Close(file);
	
	// Changes are cached in memory, sync to SD card
	// TODO maybe use fs_shutdown/fs_unmount and only sync once??
	if (usingSD) fs_fat_sync("/sd");

	int res = fs_close(file);
	return res == -1 ? errno : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	
	int res = fs_seek(file, offset, modes[seekType]);
	return res == -1 ? errno : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int res = fs_seek(file, 0, SEEK_CUR);
	*pos    = res;
	return res == -1 ? errno : 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int res = fs_total(file);
	*len    = res;
	return res == -1 ? errno : 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: Dreamcast is configured to use preemptive multithreading !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	thd_sleep(milliseconds); 
}

static void* ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	(func)();
	return NULL;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	kthread_attr_t attrs = { 0 };
	attrs.stack_size     = stackSize;
	attrs.label          = name;
	*handle = thd_create_ex(&attrs, ExecThread, func);
}

void Thread_Detach(void* handle) {
	thd_detach((kthread_t*)handle);
}

void Thread_Join(void* handle) {
	thd_join((kthread_t*)handle, NULL);
}

void* Mutex_Create(const char* name) {
	mutex_t* ptr = (mutex_t*)Mem_Alloc(1, sizeof(mutex_t), "mutex");
	int res = mutex_init(ptr, MUTEX_TYPE_NORMAL);
	if (res) Process_Abort2(errno, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = mutex_destroy((mutex_t*)handle);
	if (res) Process_Abort2(errno, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = mutex_lock((mutex_t*)handle);
	if (res) Process_Abort2(errno, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = mutex_unlock((mutex_t*)handle);
	if (res) Process_Abort2(errno, "Unlocking mutex");
}

void* Waitable_Create(const char* name) {
	semaphore_t* ptr = (semaphore_t*)Mem_Alloc(1, sizeof(semaphore_t), "waitable");
	int res = sem_init(ptr, 0);
	if (res) Process_Abort2(errno, "Creating waitable");
	return ptr;
}

void Waitable_Free(void* handle) {
	int res = sem_destroy((semaphore_t*)handle);
	if (res) Process_Abort2(errno, "Destroying waitable");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	int res = sem_signal((semaphore_t*)handle);
	if (res < 0) Process_Abort2(errno, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int res = sem_wait((semaphore_t*)handle);
	if (res < 0) Process_Abort2(errno, "Event wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	int res = sem_wait_timed((semaphore_t*)handle, milliseconds);
	if (res >= 0) return;
	
	int err = errno;
	if (err != ETIMEDOUT) Process_Abort2(err, "Event timed wait");
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	char str[NATIVE_STR_LEN];

	char portRaw[32]; cc_string portStr;
	struct addrinfo hints = { 0 };
	struct addrinfo* result;
	struct addrinfo* cur;
	int res, i = 0;
	
	String_EncodeUtf8(str, address);
	*numValidAddrs = 0;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	String_InitArray(portStr,  portRaw);
	String_AppendInt(&portStr, port);
	portRaw[portStr.length] = '\0';
	
	// getaddrinfo IP address resolution was only added in Nov 2023
	//   https://github.com/KallistiOS/KallistiOS/pull/358
	// So include this special case for backwards compatibility
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addrs[0].data;
	if (inet_pton(AF_INET, str, &addr4->sin_addr) > 0) {
		addr4->sin_family = AF_INET;
		addr4->sin_port   = htons(port);
		
		addrs[0].size  = sizeof(struct sockaddr_in);
		*numValidAddrs = 1;
		return 0;
	}

	res = getaddrinfo(str, portRaw, &hints, &result);
	if (res == EAI_NONAME) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next, i++) 
	{
		SocketAddr_Set(&addrs[i], cur->ai_addr, cur->ai_addrlen);
	}

	freeaddrinfo(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = socket(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return errno;

	if (nonblocking) {
		fcntl(*s, F_SETFL, O_NONBLOCK);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	int res = connect(s, raw, addr->size);
	return res == -1 ? errno : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = recv(s, data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return errno;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = send(s, data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return errno;
}

void Socket_Close(cc_socket s) {
	shutdown(s, SHUT_RDWR);
	close(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	if (poll(&pfd, 1, 0) == -1) { *success = false; return errno; }
	
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
	getsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
static kos_blockdev_t sd_dev;
static uint8 partition_type;

static void TryInitSDCard(void) {
	if (sd_init()) {
		// Both SD card and debug interface use the serial port
		// So if initing SD card fails, need to restore serial port state for debug logging
		scif_init();
		Platform_LogConst("Failed to init SD card"); return;
	}
	
	if (sd_blockdev_for_partition(0, &sd_dev, &partition_type)) {
		Platform_LogConst("Unable to find first partition on SD card"); return;
	}
	Platform_Log1("Found SD card (partitioned using: %b)", &partition_type);
	
	if (fs_fat_init()) {
		Platform_LogConst("Failed to init FAT filesystem"); return;
	}
	
	if (fs_fat_mount("/sd", &sd_dev, FS_FAT_MOUNT_READWRITE)) {
		Platform_LogConst("Failed to mount SD card"); return;
	}

	root_path = String_FromReadonly("/sd/ClassiCube/");
	Platform_ReadonlyFilesystem = false;
	usingSD   = true;

	cc_filepath* root = FILEPATH_RAW("/sd/ClassiCube");
	int res = Directory_Create(root);
	Platform_Log1("ROOT DIRECTORY CREATE: %i", &res);
}

static void InitModem(void) {
	int err;
	Platform_LogConst("Initialising modem..");
	
	if (!modem_init()) {
		Platform_LogConst("Modem initing failed"); return;
	}
	ppp_init();
	
	Platform_LogConst("Dialling modem.. (can take ~20 seconds)");
	err = ppp_modem_init("111111111111", 0, NULL);
	if (err) {
		Platform_Log1("Establishing link failed (%i)", &err); return;
	}

	ppp_set_login("dream", "dreamcast");

	Platform_LogConst("Connecting link.. (can take ~20 seconds)");
	err = ppp_connect();
	if (err) {
		Platform_Log1("Connecting link failed (%i)", &err); return;
 	}
}

void Platform_Init(void) {
	Platform_ReadonlyFilesystem = true;
	TryInitSDCard();
	
	if (net_default_dev) return;
	// in case Broadband Adapter isn't active
	InitModem();
	// give some time for messages to stay on-screen
	Platform_LogConst("Starting in 5 seconds..");
	Thread_Sleep(5000);
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


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "DreamCastKOS_PVR"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}
#endif