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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pspkernel.h>
#include <psputility.h>
#include <pspsdk.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <pspnet_apctl.h>
#include <psprtc.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;

#define _SCE_NET_APCTL_ERROR_WLAN_SWITCH_OFF 0x80410a06

const char* Platform_AppNameSuffix = " PSP";
cc_bool Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"

PSP_MODULE_INFO("ClassiCube", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);

// Save 140 kb by not linking in pthreads
PSP_DISABLE_AUTOSTART_PTHREAD()
// Save 70kb by not linking in sprintf/setenv code etc
PSP_DISABLE_NEWLIB_TIMEZONE_SUPPORT()


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
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}

void Platform_Log(const char* msg, int len) {
	int fd = sceKernelStdout();
	sceIoWrite(fd, msg, len);
}

TimeMS DateTime_CurrentUTC(void) {
	struct SceKernelTimeval cur;
	sceKernelLibcGettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	ScePspDateTime curTime;
	sceRtcGetCurrentClockLocalTime(&curTime);

	t->year   = curTime.year;
	t->month  = curTime.month;
	t->day    = curTime.day;
	t->hour   = curTime.hour;
	t->minute = curTime.minute;
	t->second = curTime.second;
}

#define US_PER_SEC 1000000ULL
cc_uint64 Stopwatch_Measure(void) {
	// TODO: sceKernelGetSystemTimeWide
	struct SceKernelTimeval cur;
	sceKernelLibcGettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec * US_PER_SEC + cur.tv_usec;
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
static const cc_string root_path = String_FromConst("ms0:/PSP/GAME/ClassiCube/");

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

#define GetSCEResult(result) (result >= 0 ? 0 : result & 0xFFFF)

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	int result = sceIoMkdir(path->buffer, 0777);
	return GetSCEResult(result);
}

int File_Exists(const cc_filepath* path) {
	SceIoStat sb;
	return sceIoGetstat(path->buffer, &sb) == 0 && (sb.st_attr & FIO_SO_IFREG) != 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	cc_filepath str;
	int res;

	Platform_EncodePath(&str, dirPath);
	SceUID uid = sceIoDopen(str.buffer);
	if (uid < 0) return GetSCEResult(uid); // error

	String_InitArray(path, pathBuffer);
	SceIoDirent entry;

	while ((res = sceIoDread(uid, &entry)) > 0) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		// ignore . and .. entry (PSP does return them)
		char* src = entry.d_name;
		if (src[0] == '.' && src[1] == '\0')                  continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;
		
		int len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		int is_dir = entry.d_stat.st_attr & FIO_SO_IFDIR;
		callback(&path, obj, is_dir);
	}

	sceIoDclose(uid);
	return GetSCEResult(res);
}

static cc_result File_Do(cc_file* file, const char* path, int mode) {
	int result = sceIoOpen(path, mode, 0777);
	*file      = result;
	return GetSCEResult(result);
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, PSP_O_RDONLY);
}
cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, PSP_O_RDWR | PSP_O_CREAT | PSP_O_TRUNC);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return File_Do(file, path->buffer, PSP_O_RDWR | PSP_O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	int result = sceIoRead(file, data, count);
	*bytesRead = result;
	return GetSCEResult(result);
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	int result  = sceIoWrite(file, data, count);
	*bytesWrote = result;
	return GetSCEResult(result);
}

cc_result File_Close(cc_file file) {
	int result = sceIoClose(file);
	return GetSCEResult(result);
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { PSP_SEEK_SET, PSP_SEEK_CUR, PSP_SEEK_END };
	
	int result = sceIoLseek32(file, offset, modes[seekType]);
	return GetSCEResult(result);
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	int result = sceIoLseek32(file, 0, PSP_SEEK_CUR);
	*pos       = result;
	return GetSCEResult(result);
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	int curPos = sceIoLseek32(file, 0, PSP_SEEK_CUR);
	if (curPos < 0) { *len = -1; return GetSCEResult(curPos); }
	
	*len = sceIoLseek32(file, 0, PSP_SEEK_END);
	sceIoLseek32(file, curPos, PSP_SEEK_SET); // restore position
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	sceKernelDelayThread(milliseconds * 1000); 
}

static int ExecThread(unsigned int argc, void *argv) {
	Thread_StartFunc* func = (Thread_StartFunc*)argv;
	(*func)();
	return 0;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	#define CC_THREAD_PRIORITY 17 // TODO: 18?
	#define CC_THREAD_ATTRS 0 // TODO PSP_THREAD_ATTR_VFPU?
	Thread_StartFunc func_ = func;
	
	int threadID = sceKernelCreateThread(name, ExecThread, CC_THREAD_PRIORITY, 
										stackSize, CC_THREAD_ATTRS, NULL);
										
	*handle = (void*)threadID;
	sceKernelStartThread(threadID, sizeof(func_), (void*)&func_);
}

void Thread_Detach(void* handle) {
	sceKernelDeleteThread((int)handle); // TODO don't call this??
}

void Thread_Join(void* handle) {
	sceKernelWaitThreadEnd((int)handle, NULL);
	sceKernelDeleteThread((int)handle);
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	SceLwMutexWorkarea* ptr = (SceLwMutexWorkarea*)Mem_Alloc(1, sizeof(SceLwMutexWorkarea), "mutex");
	int res = sceKernelCreateLwMutex(ptr, name, 0, 0, NULL);
	if (res) Process_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = sceKernelDeleteLwMutex((SceLwMutexWorkarea*)handle);
	if (res) Process_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = sceKernelLockLwMutex((SceLwMutexWorkarea*)handle, 1, NULL);
	if (res) Process_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = sceKernelUnlockLwMutex((SceLwMutexWorkarea*)handle, 1);
	if (res) Process_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(const char* name) {
	int evid = sceKernelCreateEventFlag(name, PSP_EVENT_WAITMULTIPLE, 0, NULL);
	if (evid < 0) Process_Abort2(evid, "Creating waitable");
	return (void*)evid;
}

void Waitable_Free(void* handle) {
	sceKernelDeleteEventFlag((int)handle);
}

void Waitable_Signal(void* handle) {
	int res = sceKernelSetEventFlag((int)handle, 0x1);
	if (res < 0) Process_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	u32 match;
	int res = sceKernelWaitEventFlag((int)handle, 0x1, PSP_EVENT_WAITAND | PSP_EVENT_WAITCLEAR, &match, NULL);
	if (res < 0) Process_Abort2(res, "Event wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	SceUInt timeout = milliseconds * 1000;
	u32 match;
	int res = sceKernelWaitEventFlag((int)handle, 0x1, PSP_EVENT_WAITAND | PSP_EVENT_WAITCLEAR, &match, &timeout);
	if (res < 0) Process_Abort2(res, "Event timed wait");
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
	struct sockaddr_in* addr4 = (struct sockaddr_in*)addrs[0].data;
	char buf[1024];
	int rid;

	/* Fallback to resolving as DNS name */
	if (sceNetResolverCreate(&rid, buf, sizeof(buf)) < 0) 
		return ERR_INVALID_ARGUMENT;

	int ret = sceNetResolverStartNtoA(rid, host, &addr4->sin_addr, 1 /* timeout */, 5 /* retries */);
	sceNetResolverDelete(rid);
	if (ret < 0) return ret;
	
	addr4->sin_family = AF_INET;
	addr4->sin_port   = SockAddr_EncodePort(port);
		
	addrs[0].size  = sizeof(*addr4);
	*numValidAddrs = 1;
	return 0;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = sceNetInetSocket(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s < 0) return sceNetInetGetErrno();

	if (nonblocking) {
		int on = 1;
		sceNetInetSetsockopt(*s, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int));
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	
	int res = sceNetInetConnect(s, raw, addr->size);
	return res < 0 ? sceNetInetGetErrno() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = sceNetInetRecv(s, data, count, 0);
	if (recvCount >= 0) { *modified = recvCount; return 0; }
	
	*modified = 0; 
	return sceNetInetGetErrno();
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = sceNetInetSend(s, data, count, 0);
	if (sentCount >= 0) { *modified = sentCount; return 0; }
	
	*modified = 0; 
	return sceNetInetGetErrno();
}

void Socket_Close(cc_socket s) {
	sceNetInetShutdown(s, SHUT_RDWR);
	sceNetInetClose(s);
}

static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct SceNetInetTimeval time = { 0 };
	int selectCount;

	FD_ZERO(&set);
	FD_SET(s, &set);

	if (mode == SOCKET_POLL_READ) {
		selectCount = sceNetInetSelect(s + 1, &set, NULL, NULL, &time);
	} else {
		selectCount = sceNetInetSelect(s + 1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) {
		*success = false; 
		return errno;
	}
	
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
	sceNetInetGetsockopt(s, SOL_SOCKET, SO_ERROR, &error, &errSize);
	return error;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#define NET_PROFILE_FIRST  1
#define NET_PROFILE_LAST  23

#define _ERROR_NETPARAM_BAD_NETCONF 0x80110601

static int last_net_state = -1;

static void DisplayNetState(int state) {
	union SceNetApctlInfo net_name  = { 0 };
	union SceNetApctlInfo net_ssid  = { 0 };

	if (state == last_net_state) return;
	last_net_state = state;

	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_PROFILE_NAME, &net_name);
	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_SSID,         &net_ssid);

	cc_string str; char buffer[256];
	String_InitArray_NT(str, buffer);
	String_Format2(&str, "Profile name: %c\nNetwork SSID: %c\n\n", 
						net_name.name, net_ssid.ssid);

	switch (state)
	{
		case PSP_NET_APCTL_STATE_SCANNING: 
			String_AppendConst(&str, "Scanning for network.."); break;
		case PSP_NET_APCTL_STATE_JOINING: 
			String_AppendConst(&str, "Joining network.."); break;
		case PSP_NET_APCTL_STATE_GETTING_IP: 
			String_AppendConst(&str, "Getting IP address.."); break;
	}

	buffer[str.length] = '\0';
	VirtualDialog_Show("Connecting to network", buffer, true);

	Platform_Log1("STATE: %i", &state);
}

static cc_bool InitNetworking(void) {
    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);    
    int res;

    res = sceNetInit(128 * 1024, 0x20, 4096, 0x20, 4096);
    if (res < 0) { Logger_SimpleWarn(res, "calling sceNetInit"); return false; }

    res = sceNetInetInit();
    if (res < 0) { Logger_SimpleWarn(res, "calling sceNetInetInit"); return false; }

    res = sceNetResolverInit();
    if (res < 0) { Logger_SimpleWarn(res, "calling sceNetResolverInit"); return false; }

    res = sceNetApctlInit(10 * 1024, 0x30);
    if (res < 0) { Logger_SimpleWarn(res, "calling sceNetApctlInit"); return false; }
    
	for (int profile = NET_PROFILE_FIRST; profile <= NET_PROFILE_LAST; profile++)
	{
    	res = sceNetApctlConnect(profile);
		// Invalid profile? Try with next one
		if (res == _ERROR_NETPARAM_BAD_NETCONF && profile != NET_PROFILE_LAST) continue;

    	if (res) { Logger_SysWarn(res, "calling sceNetApctlConnect"); return false; }

    	for (int try = 0; try < 200; try++) 
		{
        	int state;
        	res = sceNetApctlGetState(&state);
        	if (res) { Logger_SimpleWarn(res, "calling sceNetApctlGetState"); return false; }
        
        	if (state == PSP_NET_APCTL_STATE_GOT_IP) return true;
			DisplayNetState(state);

        	// not successful yet? try polling again in 50 ms
        	sceKernelDelayThread(50 * 1000);
    	}
		break; // TODO auto fallback to next profile ?
	}

	Window_ShowDialog("WiFi setup failed", "Timed out establishing a WiFi connection");
	return false;
}

static void DisplayNetworkDetails(void) {
	union SceNetApctlInfo localip  = { 0 };
	union SceNetApctlInfo netmask  = { 0 };
	union SceNetApctlInfo gateway  = { 0 };
	union SceNetApctlInfo prim_dns = { 0 };
	union SceNetApctlInfo sec_dns  = { 0 };

	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP,         &localip);
	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_SUBNETMASK, &netmask);
	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_GATEWAY,    &gateway);
	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_PRIMDNS,    &prim_dns);
	sceNetApctlGetInfo(PSP_NET_APCTL_INFO_SECDNS,     &sec_dns);


	cc_string str; char buffer[256];
	String_InitArray_NT(str, buffer);
	String_Format3(&str, "IP address: %c\nGateway IP: %c\nNetmask %c\n", 
							&localip.ip, &netmask.subNetMask, &gateway.gateway);
	String_Format2(&str, "DNS server: %c, %c", 
							&prim_dns.primaryDns, &sec_dns.	secondaryDns);

	buffer[str.length] = '\0';
	Window_ShowDialog("Networking details", buffer);
}

void Platform_Init(void) {
	cc_bool net_ok = InitNetworking();
	if (net_ok) DisplayNetworkDetails();
	/*pspDebugSioInit();*/ 
	
	// Disabling FPU exceptions avoids sometimes crashing with this line in Physics.c
	//  *tx = vel->x == 0.0f ? MATH_LARGENUM : Math_AbsF(dx / vel->x);
	// TODO: work out why this error is actually happening (inexact or underflow?) and properly fix it
	pspSdkDisableFPUExceptions();
	
	cc_filepath* root = FILEPATH_RAW(root_path.buffer);
	Directory_Create2(root);
}
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char chars[NATIVE_STR_LEN];
	int len;

	if (res == _SCE_NET_APCTL_ERROR_WLAN_SWITCH_OFF) {
		String_AppendConst(dst, "Wifi disabled or switch is off");
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
	sceKernelDcacheWritebackRange(start, length);
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "PSP_PSP_PSP_PSP_"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
