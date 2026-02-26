#define CC_XTEA_ENCRYPTION
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define DEFAULT_COMMANDLINE_FUNC

#include "../Stream.h"
#include "../ExtMath.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../PackedCol.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <net/net.h>
#include <net/poll.h>
#include <ppu-lv2.h>
#include <sys/file.h>
#include <sys/mutex.h>
#include <sys/sem.h>
#include <sys/thread.h>
#include <sys/systime.h>
#include <sys/tty.h>
#include <sys/process.h>
#include <sysmodule/sysmodule.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = 0x80010006; // ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = 0x80010014; // EEXIST
const cc_result ReturnCode_SocketInProgess  = NET_EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = NET_EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = NET_EPIPE;

const char* Platform_AppNameSuffix = " PS3";
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
cc_bool Platform_ReadonlyFilesystem;
#include "../_PlatformBase.h"

SYS_PROCESS_PARAM(1001, 256 * 1024); // 256kb stack size

#define uint_to_ptr(raw)    ((void*)((uintptr_t)(raw)))
#define ptr_to_uint(raw) ((uint32_t)((uintptr_t)(raw)))


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
	u32 done = 0;
	sysTtyWrite(STDOUT_FILENO, msg,  len, &done);
	sysTtyWrite(STDOUT_FILENO, "\n", 1,   &done);
}

TimeMS DateTime_CurrentUTC(void) {
	u64 sec, nanosec;
	sysGetCurrentTime(&sec, &nanosec);
	return sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	time_t cur_sec; 
	struct tm loc_time;

	u64 sec, nsec;
	sysGetCurrentTime(&sec, &nsec);

	cur_sec = sec;
	localtime_r(&cur_sec, &loc_time);

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
	u64 sec, nsec;
	sysGetCurrentTime(&sec, &nsec);
	return sec * NS_PER_SEC + nsec;
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / 1000;
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
static const cc_string root_path = String_FromConst("/dev_hdd0/ClassiCube/");

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
	return sysLv2FsMkdir(path->buffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

int File_Exists(const cc_filepath* path) {
	sysFSStat sb;
	return sysLv2FsStat(path->buffer, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	sysFSDirent entry;
	char* src;
	int dir_fd, res;

	Platform_EncodePath(&str, dirPath);
	if ((res = sysLv2FsOpenDir(str.buffer, &dir_fd))) return res;

	for (;;)
	{
		u64 read = 0;
		if ((res = sysLv2FsReadDir(dir_fd, &entry, &read))) return res;
		if (!read) break; // end of entries

		// ignore . and .. entry
		src = entry.d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;
	
		String_InitArray(path, pathBuffer);
		String_Format1(&path, "%s/", dirPath);
		
		int len = String_Length(src);	
		String_AppendUtf8(&path, src, len);

		int is_dir = entry.d_type == DT_DIR;
		callback(&path, obj, is_dir);
	}

	sysLv2FsCloseDir(dir_fd);
	return res;
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	int fd = -1;
	
	int access = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int res    = sysLv2FsOpen(path, mode, &fd, access, NULL, 0);
	
	if (res) {
		*file = -1; return res;
	} else {
		// TODO: is this actually needed?
		if (mode & SYS_O_CREAT) sysLv2FsChmod(path, access);
		*file = fd; return 0;
	}
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, SYS_O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, SYS_O_RDWR | SYS_O_CREAT | SYS_O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, SYS_O_RDWR | SYS_O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	u64 read = 0;
	int res  = sysLv2FsRead(file, data, count, &read);
	
	*bytesRead = read;
	return res;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	u64 wrote = 0;
	int res   = sysLv2FsWrite(file, data, count, &wrote);
	
	*bytesWrote = wrote;
	return res;
}

cc_result File_Close(cc_file file) {
	return sysLv2FsClose(file);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[] = { SEEK_SET, SEEK_CUR, SEEK_END };
	u64 position = 0;
	return sysLv2FsLSeek64(file, offset, modes[seekType], &position);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	u64 position = 0;
	int res = sysLv2FsLSeek64(file, 0, SEEK_CUR, &position);
	
	*pos = position;
	return res;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	sysFSStat st;
	int res = sysLv2FsFStat(file, &st);
	
	*len = st.st_size;
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { 
	sysUsleep(milliseconds * 1000); 
}

static void ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	sys_ppu_thread_t* thread = (sys_ppu_thread_t*)Mem_Alloc(1, sizeof(sys_ppu_thread_t), "thread");
	*handle = thread;
	
	int res = sysThreadCreate(thread, ExecThread, (void*)func,
			0, stackSize, THREAD_JOINABLE, name);
	if (res) Process_Abort2(res, "Creating thread");
}

void Thread_Detach(void* handle) {
	sys_ppu_thread_t* thread = (sys_ppu_thread_t*)handle;
	int res = sysThreadDetach(*thread);
	if (res) Process_Abort2(res, "Detaching thread");
	Mem_Free(thread);
}

void Thread_Join(void* handle) {
	u64 retVal;
	sys_ppu_thread_t* thread = (sys_ppu_thread_t*)handle;
	int res = sysThreadJoin(*thread, &retVal);
	if (res) Process_Abort2(res, "Joining thread");
	Mem_Free(thread);
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	sys_mutex_attr_t attr;	
	sysMutexAttrInitialize(attr);
	
	sys_mutex_t* mutex = (sys_mutex_t*)Mem_Alloc(1, sizeof(sys_mutex_t), "mutex");
	int res = sysMutexCreate(mutex, &attr);
	if (res) Process_Abort2(res, "Creating mutex");
	return mutex;
}

void Mutex_Free(void* handle) {
	sys_mutex_t* mutex = (sys_mutex_t*)handle;
	int res = sysMutexDestroy(*mutex);
	if (res) Process_Abort2(res, "Destroying mutex");
	Mem_Free(mutex);
}

void Mutex_Lock(void* handle) {
	sys_mutex_t* mutex = (sys_mutex_t*)handle;
	int res = sysMutexLock(*mutex, 0);
	if (res) Process_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	sys_mutex_t* mutex = (sys_mutex_t*)handle;
	int res = sysMutexUnlock(*mutex);
	if (res) Process_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(const char* name) {
	sys_sem_attr_t attr = { 0 };
	attr.attr_protocol  = SYS_SEM_ATTR_PROTOCOL;
	attr.attr_pshared   = SYS_SEM_ATTR_PSHARED; 
	
	sys_sem_t* sem = (sys_sem_t*)Mem_Alloc(1, sizeof(sys_sem_t), "waitable");
	int res = sysSemCreate(sem, &attr, 0, 1000000);
	if (res) Process_Abort2(res, "Creating waitable");
	
	return sem;
}

void Waitable_Free(void* handle) {
	sys_sem_t* sem = (sys_sem_t*)handle;

	int res = sysSemDestroy(*sem);
	if (res) Process_Abort2(res, "Destroying waitable");
	Mem_Free(sem);
}

void Waitable_Signal(void* handle) {
	sys_sem_t* sem = (sys_sem_t*)handle;
	int res = sysSemPost(*sem, 1);
	if (res) Process_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	sys_sem_t* sem = (sys_sem_t*)handle;
	int res = sysSemWait(*sem, 0);
	if (res) Process_Abort2(res, "Waitable wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	sys_sem_t* sem = (sys_sem_t*)handle;
	int res = sysSemWait(*sem, milliseconds * 1000);
	if (res) Process_Abort2(res, "Waitable wait for");
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
	struct net_hostent* res = netGetHostByName(host);
	struct sockaddr_in* addr4;
	if (!res) return net_h_errno;
	
	// Must have at least one IPv4 address
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)          return ERR_INVALID_ARGUMENT;
	
	// each address pointer is only 4 bytes long
	u32* addr_list = (u32*)res->h_addr_list;
	int i;
	
	for (i = 0; i < SOCKET_MAX_ADDRS; i++) 
	{
		char* src_addr = uint_to_ptr(addr_list[i]);
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

	*s = netSocket(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s < 0) return net_errno;

	if (nonblocking) {
		int on = 1;
		netSetSockOpt(*s, SOL_SOCKET, SO_NBIO, &on, sizeof(int));
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	
	int res = netConnect(s, raw, addr->size);
	return res < 0 ? net_errno : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = netRecv(s, data, count, 0);
	if (res < 0) return net_errno;
	
	*modified = res; return 0;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int res = netSend(s, data, count, 0);
	if (res < 0) return net_errno;
	
	*modified = res; return 0;
}

void Socket_Close(cc_socket s) {
	netShutdown(s, SHUT_RDWR);
	netClose(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	
	if (netPoll(&pfd, 1, 0) < 0) return net_errno;
	
	/* to match select, closed socket still counts as readable */
	flags    = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success = (pfd.revents & flags) != 0;
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
	netGetSockOpt(s, SOL_SOCKET, SO_ERROR, &error, &errSize);
	return error;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#define LIBNET_MEM_SIZE (128 * 1024)
static char libnet_mem[LIBNET_MEM_SIZE];

static int InitNetworking(void) {
	int res = sysModuleLoad(SYSMODULE_NET);
	if (res < 0) return res;

	netInitParam params = { 0 };
	params.memory = ptr_to_uint(libnet_mem);
	params.memory_size = LIBNET_MEM_SIZE;

	return netInitializeNetworkEx(&params);
}

void Platform_Init(void) {
	int res = InitNetworking();
	if (res) Platform_Log1("Error setting up network: %i", &res);
	
	cc_filepath* root = FILEPATH_RAW(root_path.buffer);
	Directory_Create2(root);
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

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "PS3_PS3_PS3_PS3_"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
