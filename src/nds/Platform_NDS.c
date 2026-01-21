#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#ifdef NDS_NONET
#define CC_NO_SOCKETS
#endif
#define DEFAULT_COMMANDLINE_FUNC

#define CC_XTEA_ENCRYPTION
#include "../Stream.h"
#include "../ExtMath.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../Options.h"
#include "../Animations.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <nds.h>
#include <fat.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#ifndef NDS_NONET
#include <dswifi9.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

const char* Platform_AppNameSuffix = " NDS";
cc_bool Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

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
static uint32_t last_raw;
static uint64_t base_time;

static uint32_t GetTimerValues(void) {
	uint16_t lo = TIMER_DATA(0);
	uint16_t hi = TIMER_DATA(1);

	// Lo timer can possibly overflow between reading lo and hi
	uint16_t lo_again = TIMER_DATA(0);
	uint16_t hi_again = TIMER_DATA(1);

	// Check if lo timer has overflowed
	if (lo_again < lo) {
		// If so, use known stable timer read values instead
		lo = lo_again;
		hi = hi_again;
	}
	return lo | (hi << 16);
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;

	return timerTicks2usec(end - beg);
}

cc_uint64 Stopwatch_Measure(void) {
	uint32_t raw = GetTimerValues();
	// Since counter is only a 32 bit integer, it overflows after a minute or two
	if (last_raw > 0xF0000000 && raw < 0x10000000) {
		base_time += 0x100000000ULL;
	}

	last_raw = raw;
	return base_time + raw;
}

static void Stopwatch_Init(void) {
	// Turn off both timers
	TIMER_CR(0) = 0;
	TIMER_CR(1) = 0;

	// Reset timer values to 0
	TIMER_DATA(0) = 0;
	TIMER_DATA(1) = 0;

	// Turn on timer 1, with timer 1 incrementing timer 0 on overflow
	TIMER_CR(1) = TIMER_CASCADE | TIMER_ENABLE;
	TIMER_CR(0) = TIMER_ENABLE;
}

static void LogNocash(const char* msg, int len) {
    // Can only be up to 120 bytes total
	char buffer[120];
	len = min(len, 119);
	
	Mem_Copy(buffer, msg, len);
	buffer[len] = '\0';
	nocashWrite(buffer, len + 1);
}

extern void Console_Clear(void);
extern void Console_PrintString(const char* ptr, int len);
void Platform_Log(const char* msg, int len) {
    LogNocash(msg, len);
	Console_PrintString(msg, len);
}

TimeMS DateTime_CurrentUTC(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
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
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
static const char* crash_msg;

static __attribute__((noreturn)) void CrashHandler(void) {
	Console_Clear();
	Platform_LogConst("");
	Platform_LogConst("");
	Platform_LogConst("** CLASSICUBE FATALLY CRASHED **");
	Platform_LogConst("");

	cc_uint32 mode = getCPSR() & CPSR_MODE_MASK;
	if (crash_msg) {
		Platform_LogConst(crash_msg);
	} else if (mode == CPSR_MODE_ABORT) {
		Platform_LogConst("Read/write at invalid memory");
	} else if (mode == CPSR_MODE_UNDEFINED) {
		Platform_LogConst("Executed invalid instruction");
	} else {
		Platform_Log1("Unknown error: %h", &mode);
	}
	Platform_LogConst("");

	static const char* const regNames[] = {
		"R0 ", "R1 ", "R2 ", "R3 ", "R4 ", "R5 ", "R6 ", "R7 ",
		"R8 ", "R9 ", "R10", "R11", "R12", "SP ", "LR ", "PC "
	};

	for (int r = 0; r < 8; r++) {
		Platform_Log4("%c: %h    %c: %h",
					regNames[r],     &exceptionRegisters[r],
					regNames[r + 8], &exceptionRegisters[r + 8]);
	}

	Platform_LogConst("");
	Platform_LogConst("Please report this on the       ClassiCube Discord or forums");
	Platform_LogConst("");
	Platform_LogConst("You will need to restart your DS");

	// Make the background red since game over anyways
	BG_PALETTE_SUB[16 * 16 - 1] = RGB15(31, 0, 0);
	BG_PALETTE    [16 * 16 - 1] = RGB15(31, 0, 0);

	for (;;) { }
}

void CrashHandler_Install(void) { 
	setExceptionHandler(CrashHandler);
}

void Process_Abort2(cc_result result, const char* raw_msg) {
	crash_msg = raw_msg;
	CrashHandler();
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static cc_string root_path = String_FromConst("fat:/"); // may be overriden in InitFilesystem
static bool fat_available;
static int fat_error;

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, path);
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	const char* str = path->buffer;
	String_AppendUtf8(dst, str, String_Length(str));
}

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	if (!fat_available) return 0;

	int ret = mkdir(path->buffer, 0) == -1 ? errno : 0;
	Platform_Log2("mkdir %c = %i", path->buffer, &ret);
	return ret;
}

int File_Exists(const cc_filepath* path) {
	if (!fat_available) return false;
	Platform_Log1("Check %c", path->buffer);
	
	int attribs = FAT_getAttr(path->buffer);
	return attribs >= 0 && (attribs & ATTR_DIRECTORY) == 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	struct dirent* entry;
	int res;

	String_EncodeUtf8(&str, dirPath);
	DIR* dirPtr = opendir(str.buffer);
	if (!dirPtr) return errno;

	/* POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed." */
	/* errno is sometimes leftover from previous calls, so always reset it before readdir gets called */
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		char* src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		int is_dir = entry->d_type == DT_DIR;
		callback(&path, obj, is_dir);
		errno = 0;
	}

	res = errno; /* return code from readdir */
	closedir(dirPtr);
	return res;
}

static cc_result File_Do(cc_file* file, const char* path, int mode, const char* type) {
	Platform_Log2("%c %c", type, path);

	*file = open(path, mode, 0);
	return *file == -1 ? errno : 0;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	if (!fat_available) return ReturnCode_FileNotFound;
	return File_Do(file, path->buffer, O_RDONLY, "Open");
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	if (!fat_available) return ERR_NON_WRITABLE_FS;
	return File_Do(file, path->buffer, O_RDWR | O_CREAT | O_TRUNC, "Create");
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	if (!fat_available) return ERR_NON_WRITABLE_FS;
	return File_Do(file, path->buffer, O_RDWR | O_CREAT, "Update");
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

static int LoadFatFilesystem(void* arg) {
	errno = 0;
	fat_available = fatInitDefault();
	fat_error     = errno;
	return 0;
}

static void MountFilesystem(void) {
	LoadFatFilesystem(NULL);
	return;
	
	cothread_t thread = cothread_create(LoadFatFilesystem, NULL, 0, 0);
	// If running with DSi mode in melonDS and the internal SD card is enabled, then
	//  fatInitDefault gets stuck in sdmmc_ReadSectors - because the fifoWaitValue32Async will never return
	// (You can tell when this happens - "MMC: unknown CMD 17 00000000" is logged to console)
	// However, since it does yield to cothreads, workaround this by running fatInitDefault on another thread
	//  and then giving up if it takes too long.. not the most elegant solution, but it does work
	if (thread == -1) {
		LoadFatFilesystem(NULL);
		return;
	}
	
	for (int i = 0; i < 1000; i++)
	{
		cothread_yield();
		if (cothread_has_joined(thread)) return;
		
		swiDelay(500);
	}
	Platform_LogConst("Gave up after 1000 tries");
}

static void InitFilesystem(void) {
	MountFilesystem();
	char* dir = fatGetDefaultCwd();
	
	if (dir) {
		Platform_Log1("CWD: %c", dir);
		root_path.buffer = dir;
		root_path.length = String_Length(dir);
	}
	
	Platform_ReadonlyFilesystem = !fat_available;
	if (fat_available) return;
	Platform_Log1("** FAILED TO MOUNT FILESYSTEM (error %i) **", &fat_error);
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	swiDelay(8378 * milliseconds); // TODO probably wrong
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	*handle = NULL;
}

void Thread_Detach(void* handle) {
}

void Thread_Join(void* handle) {
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	return NULL;
}

void Mutex_Free(void* handle) {
}

void Mutex_Lock(void* handle) {
}

void Mutex_Unlock(void* handle) {
}

void* Waitable_Create(const char* name) {
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
#ifndef NDS_NONET
static cc_bool net_supported = true;

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addr->data;

	if (addr4->sin_family == AF_INET) 
		return IPv4_ToString(&addr4->sin_addr, &addr4->sin_port, dst);
	return false;
}

static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)dst->data;
	cc_uint32 ip_addr = 0;

	if (!net_supported) return false; // TODO still accept?
	if (!ParseIPv4Address(ip, &ip_addr)) return false;

	addr4->sin_addr.s_addr = ip_addr;
	addr4->sin_family      = AF_INET;
	addr4->sin_port        = SockAddr_EncodePort(port);
		
	dst->size = sizeof(*addr4);
	return true;
}

static cc_bool ParseIPv6(const char* ip, int port, cc_sockaddr* dst) {
	return false;
}

static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	struct hostent* res = gethostbyname(host);
	struct sockaddr_in* addr4;
	int i;
	
	if (!net_supported) return ERR_NO_NETWORKING;
	// avoid confusion with SSL error codes
	// e.g. FFFF FFF7 > FF00 FFF7
	if (!res) return -0xFF0000 + errno;
	
	// Must have at least one IPv4 address
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)          return ERR_INVALID_ARGUMENT;

	for (i = 0; i < SOCKET_MAX_ADDRS; i++) 
	{
		char* src_addr = res->h_addr_list[i];
		if (!src_addr) break;
		addrs[i].size = sizeof(struct sockaddr_in);

		addr4 = (struct sockaddr_in*)addrs[i].data;
		addr4->sin_family = AF_INET;
		addr4->sin_port   = SockAddr_EncodePort(port);
		addr4->sin_addr   = *(struct in_addr*)src_addr;
	}

	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	if (!net_supported) { *s = -1; return ERR_NO_NETWORKING; }

	*s = socket(raw->sa_family, SOCK_STREAM, 0);
	if (*s < 0) return errno;

	if (nonblocking) {
		int blocking_raw = 1; /* non-blocking mode */
		ioctl(*s, FIONBIO, &blocking_raw);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	int res = connect(s, raw, addr->size);
	return res < 0 ? errno : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = recv(s, data, count, 0);
	if (res < 0) { *modified = 0; return errno; }
	
	*modified = res; return 0;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = send(s, data, count, 0);
	if (res < 0) { *modified = 0; return errno; }
	
	*modified = res; return 0;
}

void Socket_Close(cc_socket s) {
	shutdown(s, 2); // SHUT_RDWR = 2
	closesocket(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int res; // number of 'ready' sockets
	FD_ZERO(&set);
	FD_SET(s, &set);
	
	if (mode == SOCKET_POLL_READ) {
		res = select(s + 1, &set, NULL, NULL, &time);
	} else {
		res = select(s + 1, NULL, &set, NULL, &time);
	}
	
	if (res < 0) { *success = false; return errno; }
	*success = FD_ISSET(s, &set) != 0; 
	return 0;
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return Socket_Poll(s, SOCKET_POLL_WRITE, writable);
}

cc_result Socket_GetLastError(cc_socket s) {
	int error = ERR_INVALID_ARGUMENT;
	socklen_t errSize = sizeof(error);

	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &errSize);
	return error;
}

static void LogWifiStatus(int status) {
	switch (status) {
		case ASSOCSTATUS_SEARCHING:
			Platform_LogConst("Wifi: Searching.."); return;
		case ASSOCSTATUS_AUTHENTICATING:
			Platform_LogConst("Wifi: Authenticating.."); return;
		case ASSOCSTATUS_ASSOCIATING:
			Platform_LogConst("Wifi: Connecting.."); return;
		case ASSOCSTATUS_ACQUIRINGDHCP:
			Platform_LogConst("Wifi: Acquiring.."); return;
		case ASSOCSTATUS_ASSOCIATED:
			Platform_LogConst("Wifi: Connected successfully!"); return;
		case ASSOCSTATUS_CANNOTCONNECT:
			Platform_LogConst("Wifi: FAILED TO CONNECT"); return;
		default: 
			Platform_Log1("Wifi: status = %i", &status); return;
	}
}

#define VSYNCS_PER_SEC 60
static void InitNetworking(void) {
    if (!Wifi_InitDefault(INIT_ONLY | WIFI_ATTEMPT_DSI_MODE)) {
        Platform_LogConst("Initing WIFI failed"); 
		net_supported = false; return;
    }
    Wifi_AutoConnect();
	int last_status = -1;

    for (int i = 0; i < VSYNCS_PER_SEC * 10; i++)
    {
        int status = Wifi_AssocStatus();	
		int do_log = status != last_status || ((i % VSYNCS_PER_SEC) == 0);

		if (do_log) LogWifiStatus(status);
		last_status = status;

        if (status == ASSOCSTATUS_ASSOCIATED) return;
        if (status == ASSOCSTATUS_CANNOTCONNECT) {
			net_supported = false; return;
        }
        cothread_yield_irq(IRQ_VBLANK);
    }
    Platform_LogConst("Gave up after ~10 seconds");
	net_supported = false;
}
#else
static void InitNetworking(void) { }
#endif


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Init(void) {
	cc_bool dsiMode = isDSiMode();
	Platform_Log1("Running in %c mode", dsiMode ? "DSi" : "DS");

	InitFilesystem();
    InitNetworking();
	Stopwatch_Init();
}
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char chars[NATIVE_STR_LEN];
	int len;

	if (res == ERR_NON_WRITABLE_FS) {
		String_AppendConst(dst, "No SD card or DLDI driver found \nWon't be able to save any files");
		return true;
	}

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

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

void CPU_FlushDataCache(void* start, int length) {
	DC_FlushRange(start, length);
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "NDS_NDS_NDS_NDS_"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}

