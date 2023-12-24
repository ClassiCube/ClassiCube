#include "Core.h"
#if defined PLAT_PS2

#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "PackedCol.h"
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <kernel.h>
#include <timer_alarm.h>
#include <debug.h>
#include <sifrpc.h>
#include <iopheap.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <netman.h>
#include <ps2ip.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <io_common.h>
#include <iox_stat.h>
#include "_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound       = -ENOENT;
const cc_result ReturnCode_DirectoryExists    = -EEXIST;

const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const char* Platform_AppNameSuffix = " PS2";


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	
	_print("%s", tmp);
}

#define UnixTime_TotalMS(time) ((cc_uint64)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

void DateTime_CurrentLocal(struct DateTime* t) {
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
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
static const cc_string root_path = String_FromConst("mass:/ClassiCube/");

static void GetNativePath(char* str, const cc_string* path) {
	Mem_Copy(str, root_path.buffer, root_path.length);
	str += root_path.length;
	String_EncodeUtf8(str, path);
}

cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	return fioMkdir(str);
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	io_stat_t sb;
	GetNativePath(str, path);
	return fioGetstat(str, &sb) >= 0 && (sb.mode & FIO_SO_IFREG);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
	/*cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	struct dirent* entry;
	int res;

	GetNativePath(str, dirPath);
	DIR* dirPtr = opendir(str);
	if (!dirPtr) return errno;

	// POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed."
	// errno is sometimes leftover from previous calls, so always reset it before readdir gets called
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry
		char* src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);
		int is_dir = entry->d_type == DT_DIR;
		// TODO: fallback to stat when this fails

		if (is_dir) {
			res = Directory_Enum(&path, obj, callback);
			if (res) { closedir(dirPtr); return res; }
		} else {
			callback(&path, obj);
		}
		errno = 0;
	}

	res = errno; // return code from readdir
	closedir(dirPtr);
	return res;*/
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	char str[NATIVE_STR_LEN];
	GetNativePath(str, path);
	
	int res = fioOpen(str, mode);
	*file   = res;
	return res < 0 ? res : 0;
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return File_Do(file, path, FIO_O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	return File_Do(file, path, FIO_O_RDWR | FIO_O_CREAT | FIO_O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return File_Do(file, path, FIO_O_RDWR | FIO_O_CREAT);
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
	return fioClose(file);
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
#define STACK_SIZE (128 * 1024)

void Thread_Sleep(cc_uint32 milliseconds) {
	uint64_t cycles = MSec2TimerBusClock(milliseconds);
	ThreadWaitClock(cycles);
}

static int ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
	
	int thdID = GetThreadId();
	ee_thread_status_t info;
	
	int res = ReferThreadStatus(thdID, &info);
	if (res > 0 && info.stack) Mem_Free(info.stack);
	
	return 0; // TODO detach ?
}

void* Thread_Create(Thread_StartFunc func) {
	ee_thread_t thread = { 0 };
	thread.func        = ExecThread;
	thread.stack       = Mem_Alloc(STACK_SIZE, 1, "Thread stack");
	thread.stack_size  = STACK_SIZE;
	thread.gp_reg      = &_gp;
	thread.initial_priority = 18;
	
	int thdID = CreateThread(&thread);
	if (thdID < 0) Logger_Abort2(thdID, "Creating thread");
	return (void*)thdID;
}

void Thread_Start2(void* handle, Thread_StartFunc func) {
	int thdID = (int)handle;
	int res   = StartThread(thdID, (void*)func);
	
	if (res < 0) Logger_Abort2(res, "Running thread");
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
		if (res) Logger_Abort("Checking thread status");
		
		if (info.status == THS_DORMANT) break;
		Thread_Sleep(10); // TODO better solution
	}
}

void* Mutex_Create(void) {
	ee_sema_t sema  = { 0 };
	sema.init_count = 1;
	sema.max_count  = 1;
	
	int semID = CreateSema(&sema);
	if (semID < 0) Logger_Abort2(semID, "Creating mutex");
	return (void*)semID;
}

void Mutex_Free(void* handle) {
	int semID = (int)handle;
	int res   = DeleteSema(semID);
	
	if (res) Logger_Abort2(res, "Destroying mutex");
}

void Mutex_Lock(void* handle) {
	int semID = (int)handle;
	int res   = WaitSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int semID = (int)handle;
	int res   = SignalSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(void) {
	ee_sema_t sema  = { 0 };
	sema.init_count = 0;
	sema.max_count  = 1;
	
	int semID = CreateSema(&sema);
	if (semID < 0) Logger_Abort2(semID, "Creating waitable");
	return (void*)semID;
}

void Waitable_Free(void* handle) {
	int semID = (int)handle;
	int res   = DeleteSema(semID);
	
	if (res) Logger_Abort2(res, "Destroying waitable");
}

void Waitable_Signal(void* handle) {
	int semID = (int)handle;
	int res   = SignalSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int semID = (int)handle;
	int res   = WaitSema(semID);
	
	if (res < 0) Logger_Abort2(res, "Signalling event");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	Logger_Abort("Can't wait for");
	// TODO implement support
}


/*########################################################################################################################*
*-------------------------------------------------------Networking--------------------------------------------------------*
*#########################################################################################################################*/
// https://github.com/ps2dev/ps2sdk/blob/master/NETMAN.txt
// https://github.com/ps2dev/ps2sdk/blob/master/ee/network/tcpip/samples/tcpip_dhcp/ps2ip.c
extern unsigned char DEV9_irx[];
extern unsigned int  size_DEV9_irx;

extern unsigned char SMAP_irx[];
extern unsigned int  size_SMAP_irx;

extern unsigned char NETMAN_irx[];
extern unsigned int  size_NETMAN_irx;

static void ethStatusCheckCb(s32 alarm_id, u16 time, void *common) {
	int threadID = *(int*)common;
	iWakeupThread(threadID);
}

static int WaitValidNetState(int (*checkingFunction)(void)) {
	// Wait for a valid network status
	int threadID = GetThreadId();
	
	for (int retries = 0; checkingFunction() == 0; retries++)
	{	
		// Sleep for 500ms
		SetAlarm(500 * 16, &ethStatusCheckCb, &threadID);
		SleepThread();

		if (retries >= 10) // 5s = 10 * 500ms
			return -1;
	}
	return 0;
}

static int ethGetNetIFLinkStatus(void) {
	return NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP;
}

static int ethWaitValidNetIFLinkState(void) {
	return WaitValidNetState(&ethGetNetIFLinkStatus);
}

static int ethGetDHCPStatus(void) {
	t_ip_info ip_info;
	int result;
	if ((result = ps2ip_getconfig("sm0", &ip_info)) < 0) return result;
	
	if (ip_info.dhcp_enabled) {
		return ip_info.dhcp_status == DHCP_STATE_BOUND || ip_info.dhcp_status == DHCP_STATE_OFF;
	}
	return -1;
}

static int ethWaitValidDHCPState(void) {
	return WaitValidNetState(&ethGetDHCPStatus);
}

static int ethEnableDHCP(void) {
	t_ip_info ip_info;
	int result;
	// SMAP is registered as the "sm0" device to the TCP/IP stack.
	if ((result = ps2ip_getconfig("sm0", &ip_info)) < 0) return result;

	if (!ip_info.dhcp_enabled) {
		ip_info.dhcp_enabled = 1;	
		return ps2ip_setconfig(&ip_info);
	}
	return 0;
}

static void Networking_Setup(void) {
	struct ip4_addr IP  = { 0 }, NM = { 0 }, GW = { 0 };
	ps2ipInit(&IP, &NM, &GW);
	ethEnableDHCP();

	Platform_LogConst("Waiting for net link connection...");
	if(ethWaitValidNetIFLinkState() != 0) {
		Platform_LogConst("Failed to establish net link");
		return;
	}

	Platform_LogConst("Waiting for DHCP lease...");
	if (ethWaitValidDHCPState() != 0) {
		Platform_LogConst("Failed to acquire DHCP lease");
		return;
	}
	Platform_LogConst("Network setup done");
}

static void Networking_Init(void) {
	NetManInit();
}

static void Networking_LoadIOPModules(void) {
	int ret;
	
	ret = SifExecModuleBuffer(DEV9_irx,   size_DEV9_irx,   0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer DEV9_irx failed: %i", &ret);
	
	ret = SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer NETMAN_irx failed: %i", &ret);
	
	ret = SifExecModuleBuffer(SMAP_irx,   size_SMAP_irx,   0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer SMAP_irx failed: %i", &ret);
}

/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
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

	res = getaddrinfo(host, portRaw, &hints, &result);
	if (res == -NO_DATA) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	for (cur = result; cur && i < SOCKET_MAX_ADDRS; cur = cur->ai_next, i++) 
	{
		Mem_Copy(addrs[i].data, cur->ai_addr, cur->ai_addrlen);
		addrs[i].size = cur->ai_addrlen;
	}
	
	freeaddrinfo(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}

cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addrs[0].data;
	char str[NATIVE_STR_LEN];
	
	String_EncodeUtf8(str, address);
	*numValidAddrs = 0;

	if (inet_aton(str, &addr4->sin_addr) > 0) {
		addr4->sin_family = AF_INET;
		addr4->sin_port   = htons(port);
		
		addrs[0].size  = sizeof(*addr4);
		*numValidAddrs = 1;
		return 0;
	}
	
	return ParseHost(str, port, addrs, numValidAddrs);
}

static cc_result GetSocketError(cc_socket s) {
	socklen_t resultSize = sizeof(socklen_t);
	cc_result res = 0;
	getsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}

cc_result Socket_Connect(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	int res;

	*s = socket(raw->sa_family, SOCK_STREAM, 0);
	if (*s < 0) return *s;
	
	if (nonblocking) {
		int blocking_raw = -1; // non-blocking mode
		//ioctlsocket(*s, FIONBIO, &blocking_raw); TODO doesn't work
	}

	res = connect(*s, raw, addr->size);
	return res == -1 ? GetSocketError(*s) : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	Platform_Log1("PREPARE TO READ: %i", &count);
	int recvCount = recv(s, data, count, 0);
	Platform_Log1(" .. read %i", &recvCount);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	
	int ERR = GetSocketError(s);
	Platform_Log1("ERR: %i", &ERR);
	*modified = 0; return ERR;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	Platform_Log1("PREPARE TO WRITE: %i", &count);
	int sentCount = send(s, data, count, 0);
	Platform_Log1(" .. wrote %i", &sentCount);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	
	int ERR = GetSocketError(s);
	Platform_Log1("ERR: %i", &ERR);
	*modified = 0; return ERR;
}

void Socket_Close(cc_socket s) {
	shutdown(s, SHUT_RDWR);
	close(s);
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

	selectCount = select(s + 1, &read_set, &write_set, &error_set, &time);

	Platform_Log4("SELECT %i = %h / %h / %h", &selectCount, &read_set, &write_set, &error_set);
	if (selectCount == -1) { *success = false; return errno; }
	*success = FD_ISSET(s, &write_set) != 0; return 0;
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	Platform_LogConst("POLL READ");
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

static int tries;
cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	cc_result res = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	Platform_Log1("POLL WRITE: %i", &res);
	if (res || *writable) return res;

	// INPROGRESS error code returned if connect is still in progress
	res = GetSocketError(s);
	Platform_Log1("POLL FAIL: %i", &res);
	if (res == EINPROGRESS) res = 0;
	
	if (tries++ > 20) { *writable = true; }
	return res;
}


/*########################################################################################################################*
*----------------------------------------------------USB mass storage-----------------------------------------------------*
*#########################################################################################################################*/
extern unsigned char USBD_irx[];
extern unsigned int  size_USBD_irx;

extern unsigned char BDM_irx[];
extern unsigned int  size_BDM_irx;

extern unsigned char BDMFS_FATFS_irx[];
extern unsigned int  size_BDMFS_FATFS_irx;

extern unsigned char USBMASS_BD_irx[];
extern unsigned int  size_USBMASS_BD_irx;

extern unsigned char USBHDFSD_irx[];
extern unsigned int  size_USBHDFSD_irx;

static void USBStorage_LoadIOPModules(void) {   
    int ret;
    // TODO: Seems that
    // BDM, BDMFS_FATFS, USBMASS_BD - newer ?
    // USBHDFSD - older ?
    
	ret = SifExecModuleBuffer(USBD_irx, size_USBD_irx, 0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer USBD_irx failed: %i", &ret);
    
	//ret = SifExecModuleBuffer(USBHDFSD_irx,  size_USBHDFSD_irx,  0, NULL, NULL);
    //if (ret < 0) Platform_Log1("SifExecModuleBuffer USBHDFSD_irx failed: %i", &ret);
    
	ret = SifExecModuleBuffer(BDM_irx,  size_BDM_irx,  0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer BDM_irx failed: %i", &ret);
    
	ret = SifExecModuleBuffer(BDMFS_FATFS_irx, size_BDMFS_FATFS_irx, 0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer BDMFS_FATFS_irx failed: %i", &ret);
    
	ret = SifExecModuleBuffer(USBMASS_BD_irx,  size_USBMASS_BD_irx,  0, NULL, NULL);
    if (ret < 0) Platform_Log1("SifExecModuleBuffer USBMASS_BD_irx failed: %i", &ret);
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

void Platform_Init(void) {
	//InitDebug();
	ResetIOP();
	SifInitRpc(0);
	SifLoadFileInit();
	SifInitIopHeap();
	sbv_patch_enable_lmb(); // Allows loading IRX modules from a buffer in EE RAM

	LoadIOPModules();
	USBStorage_LoadIOPModules();
	//USBStorage_WaitUntilDeviceReady();
	
	Networking_LoadIOPModules();
	Networking_Init();
	Networking_Setup();
	
	// Create root directory
	int res = fioMkdir("mass:/ClassiCube");
	Platform_Log1("ROOT CREATE %i", &res);
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


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
static cc_result GetMachineID(cc_uint32* key) {
	return ERR_NOT_SUPPORTED;
}
#endif
