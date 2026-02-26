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

#define LIBCGLUE_SYS_SOCKET_ALIASES 0
#define LIBCGLUE_SYS_SOCKET_NO_ALIASES
#define LIBCGLUE_ARPA_INET_NO_ALIASES
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <kernel.h>
#include <delaythread.h>
#include <debug.h>
#include <ps2ip.h>
#include <sifrpc.h>
#include <iopheap.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <netman.h>
#include <ps2ip.h>
#include <dma.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <io_common.h>
#include <iox_stat.h>
#include <libcdvd.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound       = -4;
const cc_result ReturnCode_PathNotFound       = 99999;
const cc_result ReturnCode_DirectoryExists    = -8;

const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

const char* Platform_AppNameSuffix = " PS2";
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
cc_bool Platform_ReadonlyFilesystem;
#include "../_PlatformBase.h"

// extern unsigned char DEV9_irx[];
// extern unsigned int  size_DEV9_irx;
#define STRINGIFY(a) #a
#define SifLoadBuffer(name) \
	extern unsigned char name[]; \
	extern unsigned int  size_ ## name; \
	ret = SifExecModuleBuffer(name, size_ ## name, 0, NULL, NULL); \
    if (ret < 0) Platform_Log1("SifExecModuleBuffer " STRINGIFY(name) " failed: %i", &ret);


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"
static void SetupIOPCore(void);

int main(int argc, char** argv) {
	SetupIOPCore();

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
	_print("%s\n", tmp);
}

// https://stackoverflow.com/a/42340213
static CC_INLINE int UnBCD(unsigned char bcd) {
    return bcd - 6 * (bcd >> 4);
}

#define JST_OFFSET -9 * 60 // UTC+9 -> UTC
static time_t CurrentUnixTime(void) {
    sceCdCLOCK raw;
    struct tm tim;
    sceCdReadClock(&raw);

    tim.tm_sec  = UnBCD(raw.second);
    tim.tm_min  = UnBCD(raw.minute);
    tim.tm_hour = UnBCD(raw.hour);
    tim.tm_mday = UnBCD(raw.day);
	// & 0x1F, since a user had issues with upper 3 bits being set to 1
    tim.tm_mon  = UnBCD(raw.month & 0x1F) - 1;
    tim.tm_year = UnBCD(raw.year) + 100;

	// mktime will normalise the time anyways
    tim.tm_min += JST_OFFSET;
    return mktime(&tim);
}

TimeMS DateTime_CurrentUTC(void) {
	time_t rtc_sec = CurrentUnixTime();
	return (cc_uint64)rtc_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	time_t rtc_sec = CurrentUnixTime();
	struct tm loc_time;
	localtime_r(&rtc_sec, &loc_time);

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
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) { }

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static const cc_string root_path = String_FromConst("mass:/ClassiCube/");

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
	return fioMkdir(path->buffer);
}

int File_Exists(const cc_filepath* path) {
	io_stat_t sb;
	return fioGetstat(path->buffer, &sb) >= 0 && (sb.mode & FIO_SO_IFREG);
}

// For some reason fioDread seems to be returning a iox_dirent_t, instead of a io_dirent_t
// The offset of 'name' in iox_dirent_t is different to 'iox_dirent_t', so naively trying
// to use entry.name doesn't work
// TODO: Properly investigate why this is happening
static char* GetEntryName(char* src) {
	for (int i = 0; i < 256; i++)
	{
		if (src[i]) return &src[i];
	}
	return NULL;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	io_dirent_t entry;
	int fd, res;

	Platform_EncodePath(&str, dirPath);
	fd = fioDopen(str.buffer);
	if (fd < 0) return fd;
	
	res = 0;
	String_InitArray(path, pathBuffer);
	Mem_Set(&entry, 0, sizeof(entry));

	while ((res = fioDread(fd, &entry)) > 0) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		char* src = GetEntryName(entry.name);
		if (!src) continue;

		// ignore . and .. entry
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		int is_dir = FIO_SO_ISDIR(entry.stat.mode);
		callback(&path, obj, is_dir);
	}

	fioDclose(fd);
	return res;
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	int res = fioOpen(path, mode);
	*file   = res;
	return res < 0 ? res : 0;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, FIO_O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, FIO_O_RDWR | FIO_O_CREAT | FIO_O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, FIO_O_RDWR | FIO_O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	int res    = fioRead(file, data, count);
	*bytesRead = res;
	return res < 0 ? res : 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	int res     = fioWrite(file, data, count);
	*bytesWrote = res;
	return res < 0 ? res : 0;
}

cc_result File_Close(cc_file file) {
	int res = fioClose(file);
	return res < 0 ? res : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	
	int res = fioLseek(file, offset, modes[seekType]);
	return res < 0 ? res : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int res = fioLseek(file, 0, SEEK_CUR);
	*pos    = res;
	return res < 0 ? res : 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int cur_pos = fioLseek(file, 0, SEEK_CUR);
	if (cur_pos < 0) return cur_pos; // error occurred
	
	// get end and then restore position
	int res = fioLseek(file, 0, SEEK_END);
	fioLseek(file, cur_pos, SEEK_SET);
	
	*len    = res;
	return res < 0 ? res : 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) {
	DelayThread(milliseconds * 1000);
}

static int ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
	
	int thdID = GetThreadId();
	ee_thread_status_t info;
	
	int res = ReferThreadStatus(thdID, &info);
	if (res > 0 && info.stack) Mem_Free(info.stack); // TODO is it okay to free stack of running thread ????
	
	return 0; // TODO detach ?
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	ee_thread_t thread = { 0 };
	thread.func        = ExecThread;
	thread.stack       = Mem_Alloc(stackSize, 1, "Thread stack");
	thread.stack_size  = stackSize;
	thread.gp_reg      = &_gp;
	thread.initial_priority = 18;
	
	int thdID = CreateThread(&thread);
	if (thdID < 0) Process_Abort2(thdID, "Creating thread");
	*handle = (void*)thdID;
	
	int res = StartThread(thdID, (void*)func);
	if (res < 0) Process_Abort2(res, "Running thread");
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
		if (res) Process_Abort("Checking thread status");
		
		if (info.status == THS_DORMANT) break;
		Thread_Sleep(10); // TODO better solution
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	ee_sema_t sema  = { 0 };
	sema.init_count = 1;
	sema.max_count  = 1;
	
	int semID = CreateSema(&sema);
	if (semID < 0) Process_Abort2(semID, "Creating mutex");
	return (void*)semID;
}

void Mutex_Free(void* handle) {
	int semID = (int)handle;
	int res   = DeleteSema(semID);
	
	if (res) Process_Abort2(res, "Destroying mutex");
}

void Mutex_Lock(void* handle) {
	int semID = (int)handle;
	int res   = WaitSema(semID);
	
	if (res < 0) Process_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int semID = (int)handle;
	int res   = SignalSema(semID);
	
	if (res < 0) Process_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(const char* name) {
	ee_sema_t sema  = { 0 };
	sema.init_count = 0;
	sema.max_count  = 1;
	
	int semID = CreateSema(&sema);
	if (semID < 0) Process_Abort2(semID, "Creating waitable");
	return (void*)semID;
}

void Waitable_Free(void* handle) {
	int semID = (int)handle;
	int res   = DeleteSema(semID);
	
	if (res < 0) Process_Abort2(res, "Destroying waitable");
}

void Waitable_Signal(void* handle) {
	int semID = (int)handle;
	int res   = SignalSema(semID);
	
	if (res < 0) Process_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int semID = (int)handle;
	int res   = WaitSema(semID);
	
	if (res < 0) Process_Abort2(res, "Signalling event");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	Process_Abort("Can't wait for");
	// TODO implement support
}


/*########################################################################################################################*
*-------------------------------------------------------Networking--------------------------------------------------------*
*#########################################################################################################################*/
int	ps2ip_getconfig(char* netif_name,t_ip_info* ip_info);
int ps2ip_setconfig(const t_ip_info* ip_info);

// https://github.com/ps2dev/ps2sdk/blob/master/NETMAN.txt
// https://github.com/ps2dev/ps2sdk/blob/master/ee/network/tcpip/samples/tcpip_dhcp/ps2ip.c
static void Net_StatusCheckCb(s32 alarm_id, u16 time, void *common) {
	int threadID = *(int*)common;
	iWakeupThread(threadID);
}

typedef int (*WaitCheckFunc)(void);
static int WaitUntilReady(WaitCheckFunc checkingFunction) {
	int threadID = GetThreadId();
	
	for (int retries = 0; checkingFunction() == 0; retries++)
	{	
		// Sleep for 500ms
		SetAlarm(500 * 16, &Net_StatusCheckCb, &threadID);
		SleepThread();

		if (retries >= 15) return -1; // 7.5s = 15 * 500ms 
	}
	return 0;
}

static int Net_CheckIFLinkStatus(void) {
	int status = NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0);
	return status == NETMAN_NETIF_ETH_LINK_STATE_UP;
}

static int Net_CheckDHCPStatus(void) {
	t_ip_info ip_info;
	int result;
	if ((result = ps2ip_getconfig("sm0", &ip_info)) < 0) return result;
	
	if (ip_info.dhcp_enabled) {
		return ip_info.dhcp_status == DHCP_STATE_BOUND || ip_info.dhcp_status == DHCP_STATE_OFF;
	}
	return -1;
}

static int Net_EnableDHCP(void) {
	t_ip_info ip_info;
	int result;
	// SMAP is registered as the "sm0" device to the TCP/IP stack.
	if ((result = ps2ip_getconfig("sm0", &ip_info)) < 0) return result;

	if (!ip_info.dhcp_enabled) {
		ip_info.dhcp_enabled = 1;	
		return ps2ip_setconfig(&ip_info);
	}
	return 1;
}

static void Networking_Setup(void) {
	NetManInit();

	struct ip4_addr IP  = { 0 }, NM = { 0 }, GW = { 0 };
	ps2ipInit(&IP, &NM, &GW);

	int res = Net_EnableDHCP();
	if (res < 0) Platform_Log1("Error %i enabling DHCP", &res);

	Platform_LogConst("Waiting for net link connection...");
	if(WaitUntilReady(Net_CheckIFLinkStatus) != 0) {
		Window_ShowDialog("Warning", "Failed to establish network link");
		return;
	}

	Platform_LogConst("Waiting for DHCP lease...");
	if (WaitUntilReady(Net_CheckDHCPStatus) != 0) {
		Platform_LogConst("Failed to acquire DHCP lease");
		return;
	}
	Platform_LogConst("Network setup done");
}

static void Networking_LoadIOPModules(void) {
	int ret;
	
	SifLoadBuffer(DEV9_irx);
	SifLoadBuffer(NETMAN_irx);
	SifLoadBuffer(SMAP_irx);
}

/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
int lwip_shutdown(int s, int how);
int lwip_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int lwip_close(int s);
int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_recv(int s, void *mem, size_t len, int flags);
int lwip_send(int s, const void *dataptr, size_t size, int flags);
int lwip_socket(int domain, int type, int protocol);
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
int lwip_ioctl(int s, long cmd, void *argp);

int netconn_gethostbyname(const char *name, ip4_addr_t *addr);

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
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addrs[0].data;
	ip4_addr_t addr;

	int res = netconn_gethostbyname(host, &addr);
	//if (res == -NO_DATA) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

    addr4->sin_addr.s_addr = addr.addr;
	addr4->sin_family      = AF_INET;
	addr4->sin_port        = SockAddr_EncodePort(port);
		
	addrs[0].size  = sizeof(*addr4);
	*numValidAddrs = 1;
	return 0;
}

static cc_result GetSocketError(cc_socket s) {
	socklen_t resultSize = sizeof(socklen_t);
	cc_result res = 0;
	lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = lwip_socket(raw->sa_family, SOCK_STREAM, 0);
	if (*s < 0) return *s;

	if (nonblocking) {
		int blocking_raw = 1;
		int res = lwip_ioctl(*s, FIONBIO, &blocking_raw);
		//Platform_Log2("RESSS %i: %i", s, &res);
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	int res = lwip_connect(s, raw, addr->size);
	return res == -1 ? GetSocketError(s) : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	//Platform_Log1("PREPARE TO READ: %i", &count);
	int recvCount = lwip_recv(s, data, count, 0);
	//Platform_Log1(" .. read %i", &recvCount);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	
	int ERR = GetSocketError(s);
	Platform_Log1("ERR: %i", &ERR);
	*modified = 0; return ERR;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	//Platform_Log1("PREPARE TO WRITE: %i", &count);
	int sentCount = lwip_send(s, data, count, 0);
	//Platform_Log1(" .. wrote %i", &sentCount);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	
	int ERR = GetSocketError(s);
	Platform_Log1("ERW: %i", &ERR);
	*modified = 0; return ERR;
}

void Socket_Close(cc_socket s) {
	lwip_shutdown(s, SHUT_RDWR);
	lwip_close(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set read_set, write_set, error_set;
	struct timeval time = { 0 };
	int selectCount;

	FD_ZERO(&read_set);
	FD_SET(s, &read_set);
	FD_ZERO(&write_set);
	FD_SET(s, &write_set);
	FD_ZERO(&error_set);
	FD_SET(s, &error_set);

	selectCount = lwip_select(s + 1, &read_set, &write_set, &error_set, &time);

	//Platform_Log4("SELECT %i = %h / %h / %h", &selectCount, &read_set, &write_set, &error_set);
	if (selectCount == -1) { *success = false; return errno; }
	*success = FD_ISSET(s, &write_set) != 0; return 0;
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return Socket_Poll(s, SOCKET_POLL_WRITE, writable);
}

cc_result Socket_GetLastError(cc_socket s) {
	// INPROGRESS error code returned if connect is still in progress
	int error = GetSocketError(s);
	Platform_Log1("POLL FAIL: %i", &error);
	if (error == EINPROGRESS) error = 0;
	
	return error;
}


/*########################################################################################################################*
*----------------------------------------------------USB mass storage-----------------------------------------------------*
*#########################################################################################################################*/
static void USBStorage_LoadIOPModules(void) {   
    int ret;
    // TODO: Seems that
    // BDM, BDMFS_FATFS, USBMASS_BD - newer ?
    // USBHDFSD - older ?
    
	SifLoadBuffer(USBD_irx);  
	//SifLoadBuffer(USBHDFSD_irx);
    
	SifLoadBuffer(BDM_irx);
	SifLoadBuffer(BDMFS_FATFS_irx);
    
	SifLoadBuffer(USBMASS_BD_irx);
	SifLoadBuffer(USBMOUSE_irx);
	SifLoadBuffer(USBKBD_irx);
}

// TODO Maybe needed ???
/*
static void USBStorage_WaitUntilDeviceReady() {
	io_stat_t sb;
	Thread_Sleep(50);

	for (int retry = 0; retry < 50; retry++)
	{
		if (fioGetstat("mass:/", &sb) >= 0) return;
		
    	nopdelay();
    }
    Platform_LogConst("USB device still not ready ??");
}*/


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
// Note that resetting IOP does mean can't debug through ps2client
// 	https://github.com/libsdl-org/SDL/commit/d355ea9981358a1df335b1f7485ce94768bbf255
// 	https://github.com/libsdl-org/SDL/pull/6022
static void ResetIOP(void) { // reboots the IOP
	SifInitRpc(0);
	while (!SifIopReset("", 0)) { }
	while (!SifIopSync())       { }
}

static void LoadIOPModules(void) {
	int ret;
	
	// file I/O module
	ret = SifLoadModule("rom0:FILEIO",  0, NULL);
    if (ret < 0) Platform_Log1("SifLoadModule FILEIO failed: %i", &ret);
    sbv_patch_fileio();
	
	// serial I/O module (needed for memory card & input pad modules)
	ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) Platform_Log1("SifLoadModule SIO2MAN failed: %i", &ret);
	
	
	// memory card module
	ret = SifLoadModule("rom0:MCMAN",   0, NULL);
    if (ret < 0) Platform_Log1("SifLoadModule MCMAN failed: %i", &ret);
	
	// memory card server module
	ret = SifLoadModule("rom0:MCSERV",  0, NULL);
    if (ret < 0) Platform_Log1("SifLoadModule MCSERV failed: %i", &ret);
    
    
    // Input pad module
    ret = SifLoadModule("rom0:PADMAN",  0, NULL);
    if (ret < 0) Platform_Log1("SifLoadModule PADMAN failed: %i", &ret);
}

static void SetupIOPCore(void) {
	//InitDebug();
	ResetIOP();
	SifInitRpc(0);
	SifLoadFileInit();
	SifInitIopHeap();
	sbv_patch_enable_lmb(); // Allows loading IRX modules from a buffer in EE RAM

	LoadIOPModules();
}

void Platform_Init(void) {
	USBStorage_LoadIOPModules();
	//USBStorage_WaitUntilDeviceReady();
	
	Networking_LoadIOPModules();
	Networking_Setup();
	
	cc_filepath* root = FILEPATH_RAW("mass:/ClassiCube");
	int res = Directory_Create2(root);
	Platform_Log1("ROOT DIRECTORY CREATE %i", &res);
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

void CPU_FlushDataCache(void* start, int length) {
	SyncDCache(start, start + length);
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "PS2_PS2_PS2_PS2_"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}

