#define CC_XTEA_ENCRYPTION
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define DEFAULT_COMMANDLINE_FUNC

#include "../Stream.h"
#include "../ExtMath.h"
#include "../SystemFonts.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../PackedCol.h"
#include "../Audio.h"

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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <poll.h>
#include <netdb.h>
#include <malloc.h>
#include <coreinit/debug.h>
#include <coreinit/event.h>
#include <coreinit/fastmutex.h>
#include <coreinit/thread.h>
#include <coreinit/systeminfo.h>
#include <coreinit/time.h>
#include <whb/proc.h>
#include <nn/ac.h>
#include <nsysnet/_socket.h>
#include <nsysnet/_netdb.h>
#include <nsysnet/misc.h>

#define SOCK_ERR_WOULDBLOCK  6
#define SOCK_ERR_PIPE       13
#define SOCK_ERR_INPROGRESS 22

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = SOCK_ERR_INPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = SOCK_ERR_WOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = SOCK_ERR_PIPE;

const char* Platform_AppNameSuffix = " Wii U";
cc_bool Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

int main(int argc, char** argv) {
	WHBProcInit();

	SetupProgram(argc, argv);
	while (Window_Main.Exists) { 
		RunProgram(argc, argv);
	}
	
	Window_Free();
	Process_Exit(0);
	return 0;
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	char tmp[256 + 1];
	len = min(len, 256);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	
	OSReport("%s\n", tmp);
}
#define WIIU_EPOCH_ADJUST 946684800ULL // Wii U time epoch is year 2000, not 1970

TimeMS DateTime_CurrentUTC(void) {
	OSTime time   = OSGetTime();
	cc_int64 secs = (time_t)OSTicksToSeconds(time);
	return secs + UNIX_EPOCH_SECONDS + WIIU_EPOCH_ADJUST;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	struct OSCalendarTime loc_time;
	OSTicksToCalendarTime(OSGetTime(), &loc_time);

	t->year   = loc_time.tm_year;
	t->month  = loc_time.tm_mon  + 1;
	t->day    = loc_time.tm_mday;
	t->hour   = loc_time.tm_hour;
	t->minute = loc_time.tm_min;
	t->second = loc_time.tm_sec;
}


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
cc_uint64 Stopwatch_Measure(void) {
	return OSGetSystemTime();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return OSTicksToMicroseconds(end - beg);
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
// fs:/vol/external01
//const char* sd_root = WHBGetSdCardMountPath();
	//int sd_length = String_Length(sd_root);
	//Mem_Copy(str, sd_root, sd_length);
	//str   += sd_length;
	
static const cc_string root_path = String_FromConst("ClassiCube/");

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
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return mkdir(path->buffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

int File_Exists(const cc_filepath* path) {
	struct stat sb;
	return stat(path->buffer, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	DIR* dirPtr;
	struct dirent* entry;
	char* src;
	int len, res, is_dir;

	Platform_EncodePath(&str, dirPath);
	dirPtr = opendir(str.buffer);
	if (!dirPtr) return errno;

	/* POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed." */
	/* errno is sometimes leftover from previous calls, so always reset it before readdir gets called */
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		is_dir = entry->d_type == DT_DIR;
		/* TODO: fallback to stat when this fails */

		callback(&path, obj, is_dir);
		errno = 0;
	}

	res = errno; /* return code from readdir */
	closedir(dirPtr);
	return res;
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
	static cc_uint8 modes[] = { SEEK_SET, SEEK_CUR, SEEK_END };
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
void Thread_Sleep(cc_uint32 milliseconds) { 
	OSSleepTicks(OSMillisecondsToTicks(milliseconds));
}

static int ExecThread(int argc, const char **argv) {
	((Thread_StartFunc)argv)(); 
	return 0;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	OSThread* thread = (OSThread*)Mem_Alloc(1, sizeof(OSThread), "thread");
	void* stack = memalign(16, stackSize);
	
	OSCreateThread(thread, ExecThread,
                       1, (void*)func,
                       stack + stackSize, stackSize,
                       16, OS_THREAD_ATTRIB_AFFINITY_ANY);

	*handle = thread;
	// TODO revisit this
	OSSetThreadRunQuantum(thread, 1000); // force yield after 1 millisecond
	OSResumeThread(thread);
}

void Thread_Detach(void* handle) {
	OSDetachThread((OSThread*)handle);
}

void Thread_Join(void* handle) {
	int result;
	OSJoinThread((OSThread*)handle, &result);
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	OSFastMutex* mutex = (OSFastMutex*)Mem_Alloc(1, sizeof(OSFastMutex), "mutex");
	
	OSFastMutex_Init(mutex, name);
	return mutex;
}

void Mutex_Free(void* handle) {
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	OSFastMutex_Lock((OSFastMutex*)handle);
}

void Mutex_Unlock(void* handle) {
	OSFastMutex_Unlock((OSFastMutex*)handle);
}

void* Waitable_Create(const char* name) {
	OSEvent* event = (OSEvent*)Mem_Alloc(1, sizeof(OSEvent), "waitable");

	OSInitEvent(event, false, OS_EVENT_MODE_AUTO);
	return event;
}

void Waitable_Free(void* handle) {
	OSResetEvent((OSEvent*)handle);
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	OSSignalEvent((OSEvent*)handle);
}

void Waitable_Wait(void* handle) {
	OSWaitEvent((OSEvent*)handle);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	OSTime timeout = OSMillisecondsToTicks(milliseconds);
	OSWaitEventWithTimeout((OSEvent*)handle, timeout);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr raw;
	struct sockaddr_in v4;
	struct sockaddr_storage total;
};

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addr->data;

	if (addr4->sin_family == AF_INET) 
		return IPv4_ToString(&addr4->sin_addr, &addr4->sin_port, dst);
	return false;
}

static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)dst->data;
	cc_uint32 ip_addr = 0;
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
	char portRaw[32]; cc_string portStr;
	struct addrinfo hints = { 0 };
	struct addrinfo* result;
	struct addrinfo* cur;
	int res, i = 0;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	String_InitArray(portStr,  portRaw);
	String_AppendInt(&portStr, port);
	portRaw[portStr.length] = '\0';

	res = RPLWRAP(getaddrinfo)(host, portRaw, &hints, &result);
	if (res == EAI_AGAIN) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	/* Prefer IPv4 addresses first */
	// TODO: Wii U has no ipv6 support anyways?
	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next) 
	{
		if (cur->ai_family != AF_INET) continue;
		SocketAddr_Set(&addrs[i], cur->ai_addr, cur->ai_addrlen); i++;
	}
	
	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next) 
	{
		if (cur->ai_family == AF_INET) continue;
		SocketAddr_Set(&addrs[i], cur->ai_addr, cur->ai_addrlen); i++;
	}

	RPLWRAP(freeaddrinfo)(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

static CC_INLINE int Socket_LastError(void) {
	return RPLWRAP(socketlasterr)();
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = RPLWRAP(socket)(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s < 0) return Socket_LastError();

	if (nonblocking) {
		int nonblock = 1; /* non-blocking mode */
		RPLWRAP(setsockopt)(*s, SOL_SOCKET, SO_NONBLOCK, &nonblock, sizeof(int));
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	int res = RPLWRAP(connect)(s, raw, addr->size);
	return res < 0 ? Socket_LastError() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = RPLWRAP(recv)(s, data, count, 0);

	*modified = recvCount >= 0 ? recvCount : 0; 
	return recvCount < 0 ? Socket_LastError() : 0;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = RPLWRAP(send)(s, data, count, 0);

	*modified = sentCount >= 0 ? sentCount : 0;
	return sentCount < 0 ? Socket_LastError() : 0;
}

void Socket_Close(cc_socket s) {
	RPLWRAP(shutdown)(s, SHUT_RDWR);
	RPLWRAP(socketclose)(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	nsysnet_fd_set rd_set, wr_set, ex_set;
	struct nsysnet_timeval timeout = { 0, 0 };

	NSYSNET_FD_ZERO(&rd_set);
	NSYSNET_FD_ZERO(&wr_set);
	NSYSNET_FD_ZERO(&ex_set);

	nsysnet_fd_set* set = (mode == SOCKET_POLL_READ) ? &rd_set : &wr_set;
	NSYSNET_FD_SET(s, set);
	int res = RPLWRAP(select)(s + 1, &rd_set, &wr_set, &ex_set, &timeout);

	if (res < 0) { 
		*success = false; return Socket_LastError(); 
	} else {
		*success = NSYSNET_FD_ISSET(s, set) != 0; return 0;
	}
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
	RPLWRAP(getsockopt)(s, SOL_SOCKET, SO_ERROR, &error, &errSize);

	// Apparently, actual Wii U hardware returns INPROGRESS error code if connect is still in progress
	if (error == SOCK_ERR_INPROGRESS) error = 0;
	return error;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void __init_wut_socket()
{
   socket_lib_init();
   set_multicast_state(TRUE);
   ACInitialize();
   ACConnectAsync(); // TODO not Async
}

void __fini_wut_socket() {
   /*ACClose(); // TODO threadsafe?
   ACFinalize();
   socket_lib_finish();*/
}

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

void Platform_Init(void) {
	// Otherwise loading sound gets stuck endlessly repeating
	AudioBackend_Init();

	mkdir("ClassiCube", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

cc_bool Process_OpenSupported = false;
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

void Process_Exit(cc_result code) {
	WHBProcShutdown();
	exit(code); 
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "WiiUWiiUWiiUWiiU"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
