#define CC_XTEA_ENCRYPTION
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_OPEN
#define CC_NO_CRASHHANDLER
#define DEFAULT_COMMANDLINE_FUNC

#include "../Stream.h"
#include "../Funcs.h"
#include "../Utils.h"
#include "../Errors.h"

#include <xsdk/kernel.h>
#include <xsdk/log.h>
#include <xsdk/thread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <arpa/inet.h>


#define _FILE_POSITION_INFORMATION_CLASS 14
struct _FILE_POSITION_INFORMATION {
  uint64_t current_byte_offset;
};

#define _WSAEWOULDBLOCK 10035
#define _WSAEINPROGRESS 10036
#define _WSAECONNRESET  10054
#define _WSAHOST_NOT_FOUND 11001

#define _SD_BOTH 2

// see Xenia
#define OPT_MODE_INSECURE 0x5801
// https://github.com/xenia-canary/xenia-canary/blob/canary_experimental/src/xenia/kernel/xsocket.cc#L58C6-L58C16
#define _FIONBIO 0x8004667E

#define _NT_RETURN_CODE(status) (NT_SUCCESS(status) ? 0 : status)

// Relative timeouts are specified in units of 100 ns
#define TICKS_PER_MS 10000

const cc_result ReturnCode_FileShareViolation = STATUS_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound       = STATUS_OBJECT_NAME_NOT_FOUND;
const cc_result ReturnCode_PathNotFound       = STATUS_OBJECT_PATH_NOT_FOUND;
const cc_result ReturnCode_DirectoryExists    = STATUS_OBJECT_NAME_COLLISION;
const cc_result ReturnCode_SocketInProgess    = _WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock   = _WSAEWOULDBLOCK;
const cc_result ReturnCode_SocketDropped      = _WSAECONNRESET;

const char* Platform_AppNameSuffix = " XBox 360";
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
void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	
	// TODO apparently very slow on real hardware
	DbgPrint("%s", tmp);
}

#define FILETIME_EPOCH      50491123200ULL
#define FileTime_TotalSecs(time) ((time / 10000000) + FILETIME_EPOCH)

TimeMS DateTime_CurrentUTC(void) {
	uint64_t ft;
	
	KeQuerySystemTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	return FileTime_TotalSecs(ft);
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	struct timeval cur; 
	struct tm loc_time;
	gettimeofday(&cur, NULL);
	localtime_r(&cur.tv_sec, &loc_time);
	// TODO use NT kernel directly

	t->year   = loc_time.tm_year + 1900;
	t->month  = loc_time.tm_mon  + 1;
	t->day    = loc_time.tm_mday;
	t->hour   = loc_time.tm_hour;
	t->minute = loc_time.tm_min;
	t->second = loc_time.tm_sec;
}

#define MICROSECS_PER_SEC 1000000
#define TIMEBASE_FREQ     49875000LL
#define TIMEBASE_FREQ_US  (TIMEBASE_FREQ / MICROSECS_PER_SEC)

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / TIMEBASE_FREQ_US;
}

#define READ_MFTB_HI(value) __asm__ volatile("mftbu %0" : "=r"(value))
#define READ_MFTB_LO(value) __asm__ volatile("mftbl %0" : "=r"(value))

cc_uint64 Stopwatch_Measure(void) {
	cc_uint32 hi_1; READ_MFTB_HI(hi_1);
	cc_uint32 lo_1; READ_MFTB_LO(lo_1);
	cc_uint32 hi_2; READ_MFTB_HI(hi_2);
	cc_uint32 lo_2; READ_MFTB_LO(lo_2);

	// Upper half can theoretically tick over between the lo and hi read
	if (hi_1 != hi_2) {
		hi_1 = hi_2;
		lo_1 = lo_2;
	}

	return ((uint64_t)hi_1 << 32) | lo_1;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static cc_string root_path = String_FromConst("game:\\");

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	
	// XBox 360 kernel doesn't automatically convert /
	for (int i = 0; i < path->length; i++) 
	{
		char c = (char)path->buffer[i];
		if (c == '/') c = '\\';
		*str++ = c;
	}
	*str = '\0';
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	const char* str = path->buffer;
	String_AppendUtf8(dst, str, String_Length(str));
}

void Directory_GetCachePath(cc_string* path) { }

static CC_INLINE ANSI_STRING INIT_ANSI_STRING(const char* src) {
	int len = String_Length(src);
	return (ANSI_STRING){ .pointer = src, .length = len, .maximum_length = len + 1 };
}

#define INIT_OBJ_ATTRIBS(path) {.name           = path, \
								.root_directory = OB_DOS_DEVICES }


cc_result Directory_Create2(const cc_filepath* path) {
	ANSI_STRING str = INIT_ANSI_STRING(path->buffer);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	HANDLE handle = 0;
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtCreateFile(&handle, GENERIC_READ | SYNCHRONIZE, 
						&attribs, &block, NULL, FILE_ATTRIBUTE_NORMAL, 
						0, FILE_CREATE, 
						FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(status)) return status;

	(void)NtClose(handle);
	return 0;
}

int File_Exists(const cc_filepath* path) {
	ANSI_STRING str = INIT_ANSI_STRING(path->buffer);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	FILE_NETWORK_OPEN_INFORMATION info;
	NTSTATUS status = NtQueryFullAttributesFile(&attribs, &info);
	if (!NT_SUCCESS(status)) return 0;

	return !(info.attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static void Directory_EnumCore(const cc_string* dirPath, const cc_string* file, uint32_t attribs,
									void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[NATIVE_STR_LEN + 10];
	/* ignore . and .. entry */
	if (file->length == 1 && file->buffer[0] == '.') return;
	if (file->length == 2 && file->buffer[0] == '.' && file->buffer[1] == '.') return;

	String_InitArray(path, pathBuffer);
	String_Format2(&path, "%s/%s", dirPath, file);

	callback(&path, obj, attribs & FILE_ATTRIBUTE_DIRECTORY);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_filepath dirBuf;
	Platform_EncodePath(&dirBuf, dirPath);

	ANSI_STRING str = INIT_ANSI_STRING(dirBuf.buffer);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	HANDLE handle = 0;
	IO_STATUS_BLOCK block;

	NTSTATUS status = NtCreateFile(&handle, GENERIC_READ /*FILE_LIST_DIRECTORY*/ | SYNCHRONIZE, 
						&attribs, &block, NULL, FILE_ATTRIBUTE_NORMAL, 
						FILE_SHARE_READ, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
	if (!NT_SUCCESS(status)) return status;

	uint32_t first_scan = true;
	for (;;)
	{
		struct {
    		FILE_DIRECTORY_INFORMATION dirInfo; // dirInfo.filename array has no actual characters
    		char filename_rest[NATIVE_STR_LEN];
		} info = { 0 };
		status = NtQueryDirectoryFile(handle, 0, NULL, NULL, &block, &info.dirInfo, sizeof(info), 
										NULL, first_scan);
		first_scan = false;
		if (status) break;

		cc_string path; char pathBuffer[NATIVE_STR_LEN + 10];
		String_InitArray(path, pathBuffer);
		const char* filename = info.dirInfo.file_name;

		for (int i = 0; i < info.dirInfo.file_name_length; i++) 
		{
			String_Append(&path, Convert_CodepointToCP437(filename[i]));
		}
		Directory_EnumCore(dirPath, &path, info.dirInfo.attributes, obj, callback);
	}

	NtClose(handle);
	return status == STATUS_NO_MORE_FILES ? 0 : status;
}

static cc_result DoFile(cc_file* file, const char* path, uint32_t access, uint32_t createMode) {
	ANSI_STRING str = INIT_ANSI_STRING(path);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	HANDLE handle = 0;
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtCreateFile(&handle, access | SYNCHRONIZE, 
						&attribs, &block, NULL, FILE_ATTRIBUTE_NORMAL, 
						FILE_SHARE_READ, createMode, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);

	*file = NT_SUCCESS(status) ? handle : 0;
	return _NT_RETURN_CODE(status);
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return DoFile(file, path->buffer, GENERIC_READ, FILE_OPEN);
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return DoFile(file, path->buffer, GENERIC_WRITE | GENERIC_READ, FILE_OVERWRITE_IF);
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return DoFile(file, path->buffer, GENERIC_WRITE | GENERIC_READ, FILE_OPEN_IF);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtReadFile(file, 0, NULL, NULL, &block, data, count, NULL);
   
	*bytesRead = NT_SUCCESS(status) ? block.information : 0;
	if (status == STATUS_END_OF_FILE) return 0;
	return _NT_RETURN_CODE(status);
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtWriteFile(file, 0, NULL, NULL, &block, (void*)data, count, NULL);

	*bytesWrote = NT_SUCCESS(status) ? block.information : 0;
	return _NT_RETURN_CODE(status);
}

cc_result File_Close(cc_file file) {
	NTSTATUS status = NtClose(file);
	return _NT_RETURN_CODE(status);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	struct _FILE_POSITION_INFORMATION pos = { 0 };
	IO_STATUS_BLOCK block;
	NTSTATUS status;

	switch (seekType) {
	case FILE_SEEKFROM_BEGIN:
		pos.current_byte_offset = offset;
		break;
	case FILE_SEEKFROM_CURRENT:
		status = NtQueryInformationFile(file, &block, &pos, sizeof(pos), _FILE_POSITION_INFORMATION_CLASS);
		if (!NT_SUCCESS(status)) return status;

		pos.current_byte_offset += offset;
		break;
	case FILE_SEEKFROM_END:
		FILE_NETWORK_OPEN_INFORMATION info;
		status = NtQueryInformationFile(file, &block, &info, sizeof(info), FILE_NETWORK_OPEN_INFORMATION_CLASS);
		if (!NT_SUCCESS(status)) return status;

		pos.current_byte_offset = info.end_of_file + offset;
		break;
	}

	status = NtSetInformationFile(file, &block, &pos, sizeof(pos), _FILE_POSITION_INFORMATION_CLASS);
	return _NT_RETURN_CODE(status);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	struct _FILE_POSITION_INFORMATION info;
	IO_STATUS_BLOCK block;

	NTSTATUS status = NtQueryInformationFile(file, &block, &info, sizeof(info), _FILE_POSITION_INFORMATION_CLASS);

	*pos = NT_SUCCESS(status) ? info.current_byte_offset : 0;
	return _NT_RETURN_CODE(status);
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	FILE_NETWORK_OPEN_INFORMATION info;
	IO_STATUS_BLOCK block;

	NTSTATUS status = NtQueryInformationFile(file, &block, &info, sizeof(info), FILE_NETWORK_OPEN_INFORMATION_CLASS);

	*len = NT_SUCCESS(status) ? info.end_of_file : 0;
	return _NT_RETURN_CODE(status);
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#############################################################################################################p############*/
static void WaitForSignal(HANDLE handle, int64_t* duration) {
	for (;;) 
	{
		NTSTATUS status = NtWaitForSingleObjectEx(handle, USER_MODE, false, duration);
		if (status != STATUS_ALERTED) break;
	}
}

void Thread_Sleep(cc_uint32 milliseconds) {
	// negative value specifies relative timeout
	int64_t delay = -(int64_t)milliseconds * TICKS_PER_MS;
	KeDelayExecutionThread(USER_MODE, 0, &delay);
}

static void run_thread(Thread_StartFunc func) {
	func();
	ExTerminateThread(0);
}

#define CREATE_SUSPENDED 0x00000001
void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	uint32_t thread_id;
	HANDLE thread;

	NTSTATUS status = ExCreateThread((HANDLE*)handle, stackSize, &thread_id, NULL, (void *)run_thread, func, 0);
	if (!NT_SUCCESS(status)) Process_Abort2(status, "creating thread");

	// TODO use NtResumeThread once added
	//NTSTATUS status = ExCreateThread(&thread, stackSize, &thread_id, NULL, (void *)run_thread, func, CREATE_SUSPENDED);
	//if (!NT_SUCCESS(status)) Process_Abort2(status, "creating thread");

	//*handle = (void*)thread;
	//NtResumeThread(thread, NULL);
}

void Thread_Detach(void* handle) {
	NTSTATUS status = NtClose((HANDLE)handle);
	if (!NT_SUCCESS(status)) Process_Abort2(status, "Freeing thread handle");
}

void Thread_Join(void* handle) {
	WaitForSignal((HANDLE)handle, NULL);
	Thread_Detach(handle);
}

/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	RTL_CRITICAL_SECTION* ptr = (RTL_CRITICAL_SECTION*)Mem_Alloc(1, sizeof(RTL_CRITICAL_SECTION), "mutex");
	RtlInitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(void* handle)   { 
	// per Xenia, "... Also, the 360 never calls RtlDeleteCriticalSection"
	//RtlDeleteCriticalSection((RTL_CRITICAL_SECTION*)handle); 
	Mem_Free(handle);
}

void Mutex_Lock(void* handle)   { 
	RtlEnterCriticalSection((RTL_CRITICAL_SECTION*)handle); 
}

void Mutex_Unlock(void* handle) { 
	RtlLeaveCriticalSection((RTL_CRITICAL_SECTION*)handle); 
}

void* Waitable_Create(const char* name) {
	HANDLE handle;
	NTSTATUS status = NtCreateEvent(&handle, NULL, SYNCHRONIZATION_EVENT, false);

	if (!NT_SUCCESS(status)) Process_Abort2(status, "Creating waitable");
	return (void*)handle;
}

void Waitable_Free(void* handle) {
	NTSTATUS status = NtClose((HANDLE)handle);
	if (!NT_SUCCESS(status)) Process_Abort2(status, "Freeing waitable");
}

void Waitable_Signal(void* handle) { 
	NtSetEvent((HANDLE)handle, NULL); 
}

void Waitable_Wait(void* handle) {
	WaitForSignal((HANDLE)handle, NULL);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	// negative values specify relative timeouts
	int64_t duration = -(int64_t)milliseconds * TICKS_PER_MS;

	WaitForSignal((HANDLE)handle, &duration);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addr->data;

	if (addr4->sin_family == AF_INET) 
		return IPv4_ToString(&addr4->sin_addr, &addr4->sin_port, dst);
	return false;
}

static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)dst->data;
	cc_uint32 ip_addr;
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
	HANDLE done_signal = 0;
	NTSTATUS status = NtCreateEvent(&done_signal, NULL, NOTIFICATION_EVENT, 0);
	if (!NT_SUCCESS(status)) return ERR_OUT_OF_MEMORY;

	XNDNS* lookup = NULL;
	int result = NetDll_XNetDnsLookup(XNCALLER_TITLE, host, done_signal, &lookup);
	if (result != 0 || !lookup) return ERR_NOT_SUPPORTED;

	int64_t timeout = -(int64_t)5000 * TICKS_PER_MS;
	status = NtWaitForSingleObjectEx(done_signal, USER_MODE, 0, &timeout);
	NtClose(done_signal);

	if (status == STATUS_TIMEOUT) {
		result = _WSAECONNRESET;
	} else if (lookup->status) {
		result = lookup->status == _WSAHOST_NOT_FOUND ? SOCK_ERR_UNKNOWN_HOST : lookup->status;
	} else {
		int count = min(lookup->count, SOCKET_MAX_ADDRS);
		for (int i = 0; i < count; i++)
		{
			struct sockaddr_in addr;
			addr.sin_family      = AF_INET;
			addr.sin_port        = port;
			addr.sin_addr.s_addr = lookup->addresses[i];

			SocketAddr_Set(&addrs[i], &addr, sizeof(addr));
		}

		*numValidAddrs = count;
		result = count == 0 ? ERR_INVALID_ARGUMENT : 0;
	}

	NetDll_XNetDnsRelease(XNCALLER_TITLE, lookup);
	return result;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr) {
	struct sockaddr_in* raw_addr = (struct sockaddr_in*)addr->data;

	*s = NetDll_socket(XNCALLER_TITLE, raw_addr->sin_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return NetDll_WSAGetLastError();

	// default 'secure' sockets are unusable for general LAN/Internet connections
	cc_uint32 insecure_on = 1;
	NetDll_setsockopt(XNCALLER_TITLE, *s, SOL_SOCKET, OPT_MODE_INSECURE, &insecure_on, sizeof(insecure_on));

	return 0;
}

cc_result Socket_SetNonBlocking(cc_socket s, cc_bool nonblocking) {
	uint32_t mode = nonblocking ? -1 : 0;
	int res       = NetDll_ioctlsocket(XNCALLER_TITLE, s, _FIONBIO, &mode);
	return res == -1 ? NetDll_WSAGetLastError() : 0;
}

void Socket_Close(cc_socket s) {
	NetDll_shutdown(XNCALLER_TITLE, s, _SD_BOTH);
	NetDll_closesocket(XNCALLER_TITLE, s);
}

cc_result Socket_Connect(cc_socket s, const void* addr, int addrSize) {
	int res = NetDll_connect(XNCALLER_TITLE, s, addr, addrSize);
	return res == -1 ? NetDll_WSAGetLastError() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = NetDll_recv(XNCALLER_TITLE, s, (char*)data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return NetDll_WSAGetLastError();
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = NetDll_send(XNCALLER_TITLE, s, (const char*)data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return NetDll_WSAGetLastError();
}

cc_result Socket_Poll(cc_socket s, int timeoutMS, int mode, cc_bool* success) {
	WSA_FD_SET set1, set2;
	int selectCount;
	WSA_TIMEVAL time;
	
	time.seconds      = timeoutMS / 1000;          // seconds
	time.microseconds = (timeoutMS % 1000) * 1000; // microseconds

	set1.count      = 1; set2.count      = 1;
	set1.sockets[0] = s; set2.sockets[0] = s;

	/* As per https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select */
	/* A socket will be pollable (select returns true) in following cases): */
	/* - readfds: Data is available for reading, or Connection has been closed/reset/terminated. */
	/* - writefds: Non-blocking connection attempt succeeded, or data can be sent */
	/* - exceptfds: Non-blocking connection attempt failed, or OOB data is available for reading */

	if (mode == SOCKET_POLL_READ) {
		selectCount = NetDll_select(XNCALLER_TITLE, 1, &set1, NULL, NULL, &time);

		*success = set1.count != 0; 
	} else {
		selectCount = NetDll_select(XNCALLER_TITLE, 1, NULL, &set1, &set2, &time);

		*success = set1.count != 0 || set2.count != 0;
	}

	if (selectCount >= 0) return 0;
	*success = false; 
	return NetDll_WSAGetLastError();
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#define WINSOCK_VERSION 0x0202 // MAKEWORD(2, 2)

static void InitNetworking(void) {
	int status1 = NetDll_XNetStartup(XNCALLER_TITLE, NULL);
	if (status1) { Logger_SimpleWarn(status1, "setting up network"); }

	cc_uint8 wsa_data[512]; // TODO: only 400 bytes? does it need to be static?
	int status2 = NetDll_WSAStartup(XNCALLER_TITLE, WINSOCK_VERSION, wsa_data);
	if (status2) { Logger_SimpleWarn(status2, "setting up WSA"); }
}

void Platform_Init(void) {
	// TODO: remove on console
	xsdk_log_set_dbgprint(true);
	InitNetworking();
}

void Platform_Free(void) {
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) { return false; }
void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

cc_result Platform_SetDefaultCurrentDirectory(void) { return 0; }


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "360_360_360_360_"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
