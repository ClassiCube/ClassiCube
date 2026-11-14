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

#include <windows.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <nxdk/net.h>
#include <nxdk/mount.h>

#define _STATUS_SHARING_VIOLATION 0xC0000043L
const cc_result ReturnCode_FileShareViolation = _STATUS_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = STATUS_OBJECT_NAME_NOT_FOUND;
const cc_result ReturnCode_PathNotFound     = STATUS_OBJECT_PATH_NOT_FOUND;
const cc_result ReturnCode_DirectoryExists  = STATUS_OBJECT_NAME_COLLISION;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

const char* Platform_AppNameSuffix = " XBox";
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
	char tmp[2048 + 2];
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\n'; tmp[len + 1] = '\0';
	
	// log to cxbx-reloaded console or Xemu serial output
	OutputDebugStringA(tmp);
}

#define FILETIME_EPOCH      50491123200ULL
#define FILETIME_UNIX_EPOCH 11644473600ULL
#define FileTime_TotalSecs(time) ((time / 10000000) + FILETIME_EPOCH)
#define FileTime_UnixTime(time)  ((time / 10000000) - FILETIME_UNIX_EPOCH)
TimeMS DateTime_CurrentUTC(void) {
	LARGE_INTEGER ft;
	
	KeQuerySystemTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	return FileTime_TotalSecs(ft.QuadPart);
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	t->year   = localTime.wYear;
	t->month  = localTime.wMonth;
	t->day    = localTime.wDay;
	t->hour   = localTime.wHour;
	t->minute = localTime.wMinute;
	t->second = localTime.wSecond;
}

/* TODO: check this is actually accurate */
static cc_uint64 sw_freqDiv = 1;
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ((end - beg) * 1000000ULL) / sw_freqDiv;
}

cc_uint64 Stopwatch_Measure(void) {
	return KeQueryPerformanceCounter();
}

static void Stopwatch_Init(void) {
	ULONGLONG freq = KeQueryPerformanceFrequency();
	sw_freqDiv     = freq;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static cc_string root_path = String_FromConst("E:\\ClassiCube\\");
static BOOL hdd_mounted;

void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	
	// XBox kernel doesn't seem to convert /
	for (int i = 0; i < path->length; i++) 
	{
		char c = (char)path->buffer[i];
		if (c == '/') c = '\\';
		*str++ = c;
	}
	*str = '\0';
}

#define INIT_OBJ_ATTRIBS(path) {.ObjectName    = path, \
								.Attributes    = OBJ_CASE_INSENSITIVE, \
								.RootDirectory = ObDosDevicesDirectory() }

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	String_AppendConst(dst, path->buffer);
}

void Directory_GetCachePath(cc_string* path) { }
cc_result Directory_Create2(const cc_filepath* path) {
	ANSI_STRING str;
	RtlInitAnsiString(&str, path->buffer);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	IO_STATUS_BLOCK block;
	HANDLE handle   = NULL;
	NTSTATUS status = NtCreateFile(&handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, 
						&attribs, &block, NULL, FILE_ATTRIBUTE_NORMAL, 
						FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_CREATE, 
						FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(status)) return status;

	(void)NtClose(handle);
	return 0;
}

int File_Exists(const cc_filepath* path) {
	ANSI_STRING str;
	RtlInitAnsiString(&str, path->buffer);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	FILE_NETWORK_OPEN_INFORMATION info;
	NTSTATUS status = NtQueryFullAttributesFile(&attribs, &info);
	if (!NT_SUCCESS(status)) return 0;

	return !(info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

static void Directory_EnumCore(const cc_string* dirPath, const cc_string* file, DWORD attribs,
									void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	/* ignore . and .. entry */
	if (file->length == 1 && file->buffer[0] == '.') return;
	if (file->length == 2 && file->buffer[0] == '.' && file->buffer[1] == '.') return;

	String_InitArray(path, pathBuffer);
	String_Format2(&path, "%s/%s", dirPath, file);

	int is_dir = attribs & FILE_ATTRIBUTE_DIRECTORY;
	callback(&path, obj, is_dir);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_filepath dirBuf;
	Platform_EncodePath(&dirBuf, dirPath);

	ANSI_STRING str;
	RtlInitAnsiString(&str, dirBuf.buffer);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	HANDLE handle = NULL;
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtOpenFile(&handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, 
						&attribs, &block,
						FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(status)) return status;

	BOOL first_scan = true;
	for (;;)
	{
		struct {
    		FILE_DIRECTORY_INFORMATION dirInfo; // dirInfo.filename array only has first character
    		char filename_rest[MAX_PATH];
		} info = { 0 };
		status = NtQueryDirectoryFile(handle, NULL, NULL, NULL, &block, &info, sizeof(info), 
										FileDirectoryInformation, NULL, first_scan);
		first_scan = false;
		if (status) break;

		cc_string path; char pathBuffer[MAX_PATH + 10];
		String_InitArray(path, pathBuffer);
		const char* filename = info.dirInfo.FileName;

		for (int i = 0; i < info.dirInfo.FileNameLength; i++) 
		{
			String_Append(&path, Convert_CodepointToCP437(filename[i]));
		}
		Directory_EnumCore(dirPath, &path, info.dirInfo.FileAttributes, obj, callback);
	}

	NtClose(handle);
	return status == STATUS_NO_MORE_FILES ? 0 : status;
}

static cc_result DoFile(cc_file* file, const char* path, DWORD access, DWORD createMode) {
	ANSI_STRING str;
	RtlInitAnsiString(&str, path);
	OBJECT_ATTRIBUTES attribs = INIT_OBJ_ATTRIBS(&str);

	HANDLE handle = NULL;
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtCreateFile(&handle, access | FILE_READ_ATTRIBUTES | SYNCHRONIZE, 
						&attribs, &block, NULL, FILE_ATTRIBUTE_NORMAL, 
						FILE_SHARE_READ, createMode, FILE_SYNCHRONOUS_IO_NONALERT);

	if (NT_SUCCESS(status)) {
		*file = handle;
		return 0;
	}
   
	*file = NULL;
	return status;
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
	NTSTATUS status = NtReadFile(file, NULL, NULL, NULL, &block, data, count, NULL);

	if (status == STATUS_PENDING) {
		status = NtWaitForSingleObject(file, FALSE, NULL);
		if (NT_SUCCESS(status)) status = block.Status;
	}

	if (NT_SUCCESS(status)) {
		*bytesRead = block.Information;
		return 0;
	}
   
	*bytesRead = 0;
	return status == STATUS_END_OF_FILE ? 0 : status;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	IO_STATUS_BLOCK block;
	NTSTATUS status = NtWriteFile(file, NULL, NULL, NULL, &block, (void*)data, count, NULL);

	if (status == STATUS_PENDING) {
		status = NtWaitForSingleObject(file, FALSE, NULL);
		if (NT_SUCCESS(status)) status = block.Status;
	}

	if (NT_SUCCESS(status)) {
		*bytesWrote = block.Information;
		return 0;
	}

	*bytesWrote = 0;
	return status;
}

cc_result File_Close(cc_file file) {
	NTSTATUS status = NtClose(file);
	return NT_SUCCESS(status) ? 0 : status;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	NTSTATUS status;
	IO_STATUS_BLOCK block;
	FILE_POSITION_INFORMATION pos = { 0 };

	switch (seekType) {
	case FILE_SEEKFROM_BEGIN:
		pos.CurrentByteOffset.QuadPart = offset;
		break;
	case FILE_SEEKFROM_CURRENT:
		status = NtQueryInformationFile(file, &block, &pos, sizeof(pos), FilePositionInformation);
		if (!NT_SUCCESS(status)) return status;

		pos.CurrentByteOffset.QuadPart += offset;
		break;
	case FILE_SEEKFROM_END:
		FILE_NETWORK_OPEN_INFORMATION info;
		status = NtQueryInformationFile(file, &block, &info, sizeof(info), FileNetworkOpenInformation);
		if (!NT_SUCCESS(status)) return status;

		pos.CurrentByteOffset.QuadPart = info.EndOfFile.QuadPart + offset;
		break;
	}

	return NtSetInformationFile(file, &block, &pos, sizeof(pos), FilePositionInformation);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	IO_STATUS_BLOCK block;
	FILE_POSITION_INFORMATION info;

	NTSTATUS status = NtQueryInformationFile(file, &block, &info, sizeof(info), FilePositionInformation);
	if (NT_SUCCESS(status)) {
		*pos = info.CurrentByteOffset.QuadPart;
		return 0;
	}
	return status;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	IO_STATUS_BLOCK block;
	FILE_NETWORK_OPEN_INFORMATION info;

	NTSTATUS status = NtQueryInformationFile(file, &block, &info, sizeof(info), FileNetworkOpenInformation);
	if (NT_SUCCESS(status)) {
		*len = info.EndOfFile.LowPart;
		return 0;
	}
	return status;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*##########################################################################################################################*/
static void WaitForSignal(HANDLE handle, LARGE_INTEGER* duration) {
	for (;;) 
	{
		NTSTATUS status = NtWaitForSingleObjectEx((HANDLE)handle, UserMode, FALSE, duration);
		if (status != STATUS_ALERTED) break;
	}
}

void Thread_Sleep(cc_uint32 milliseconds) { Sleep(milliseconds); }
static DWORD WINAPI ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	func();
	return 0;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	DWORD threadID;
	HANDLE thread = CreateThread(NULL, stackSize, ExecThread, (void*)func, CREATE_SUSPENDED, &threadID);
	if (!thread) Process_Abort2(GetLastError(), "Creating thread");

	*handle = thread;
	NtResumeThread(thread, NULL);
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
	CRITICAL_SECTION* ptr = (CRITICAL_SECTION*)Mem_Alloc(1, sizeof(CRITICAL_SECTION), "mutex");
	RtlInitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(void* handle)   { 
	RtlDeleteCriticalSection((CRITICAL_SECTION*)handle); 
	Mem_Free(handle);
}

void Mutex_Lock(void* handle)   { 
	RtlEnterCriticalSection((CRITICAL_SECTION*)handle); 
}

void Mutex_Unlock(void* handle) { 
	RtlLeaveCriticalSection((CRITICAL_SECTION*)handle); 
}

void* Waitable_Create(const char* name) {
	HANDLE handle;
	NTSTATUS status = NtCreateEvent(&handle, NULL, SynchronizationEvent, false);

	if (!NT_SUCCESS(status)) Process_Abort2(status, "Creating waitable");
	return handle;
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
	LARGE_INTEGER duration;
	duration.QuadPart = ((LONGLONG)milliseconds) * -10000; // negative for relative timeout

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
	int i = 0;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	String_InitArray(portStr,  portRaw);
	String_AppendInt(&portStr, port);
	portRaw[portStr.length] = '\0';

	int res = lwip_getaddrinfo(host, portRaw, &hints, &result);
	if (res == EAI_FAIL) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next, i++) 
	{
		SocketAddr_Set(&addrs[i], cur->ai_addr, cur->ai_addrlen);
	}

	lwip_freeaddrinfo(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = lwip_socket(raw->sa_family, SOCK_STREAM, 0);
	if (*s == -1) return errno;

	return 0;
}

cc_result Socket_SetNonBlocking(cc_socket s, cc_bool nonblocking) {
	int mode = nonblocking ? -1 : 0;
	int res  = lwip_ioctl(s, FIONBIO, &mode);
	return res == -1 ? errno : 0;
}

void Socket_Close(cc_socket s) {
	lwip_shutdown(s, SHUT_RDWR);
	lwip_close(s);
}

cc_result Socket_Connect(cc_socket s, const void* addr, int addrSize) {
	int res = lwip_connect(s, (struct sockaddr*)addr, addrSize);
	return res == -1 ? errno : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = lwip_recv(s, data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return errno;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = lwip_send(s, data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return errno;
}

cc_result Socket_Poll(cc_socket s, int timeoutMS, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	if (lwip_poll(&pfd, 1, timeoutMS) == -1) { *success = false; return errno; }
	
	/* to match select, closed socket still counts as readable */
	flags    = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success = (pfd.revents & flags) != 0;
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
static void InitHDD(void) {
	// E: drive is usually used for storing game data 
	//   see https://xboxdevwiki.net/Hard_Drive#Partitions
	ANSI_STRING drivePath;
	RtlInitAnsiString(&drivePath, "\\??\\E:");

	OBJECT_ATTRIBUTES attribs; // NOTE: not in DOS namespace, unlike other usages of OBJECT_ATTRIBUTES
	InitializeObjectAttributes(&attribs, &drivePath, OBJ_CASE_INSENSITIVE, NULL, NULL);

	HANDLE handle = NULL;
	if (NT_SUCCESS(NtOpenSymbolicLinkObject(&handle, &attribs))) {
		NtClose(handle);
		hdd_mounted = true;
	} else {
		ANSI_STRING mountPath;
		RtlInitAnsiString(&mountPath, "\\Device\\Harddisk0\\Partition1\\");

		Platform_LogConst("Mounting HDD to E:\...");
		hdd_mounted = NT_SUCCESS(IoCreateSymbolicLink(&drivePath, &mountPath));
	}

	if (!hdd_mounted) {
		Window_ShowDialog("Failed to mount HDD", "Failed to mount E:/ from Data partition");
		return;
	}
	
	cc_filepath* root = FILEPATH_RAW(root_path.buffer);
	Directory_Create2(root);
}

extern struct netif *g_pnetif;
static void InitNetworking(void) {
	VirtualDialog_Show("Connecting to network..", "This may take up to 30 seconds", true);
	char localip[32] = {0};
	char netmask[32] = {0};
	char gateway[32] = {0};

#ifndef CC_BUILD_CXBX
	int ret = nxNetInit(NULL);
	if (ret) { 
		Logger_SimpleWarn(ret, "setting up network");
	} else {
		cc_string str; char buffer[256];
		String_InitArray_NT(str, buffer);
		String_Format3(&str, "IP address: %c\nGateway IP: %c\nNetmask %c", 
						ip4addr_ntoa_r(netif_ip4_addr(g_pnetif),    localip, 31),
						ip4addr_ntoa_r(netif_ip4_gw(g_pnetif),      gateway, 31),
						ip4addr_ntoa_r(netif_ip4_netmask(g_pnetif), netmask, 31));

		buffer[str.length] = '\0';
		Window_ShowDialog("Networking details", buffer);
	}
#endif
}

void Platform_Init(void) {
	InitHDD();
	Stopwatch_Init();
	InitNetworking();
}

void Platform_Free(void) {
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return false;
}

void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

cc_result Platform_SetDefaultCurrentDirectory(void) { return 0; }


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "XboxXboxXboxXbox"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}

