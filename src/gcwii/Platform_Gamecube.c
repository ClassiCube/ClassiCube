#include "../Core.h"
#if defined CC_BUILD_GAMECUBE

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <network.h>
#include <ogc/cache.h>
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <ogc/semaphore.h>
#include <ogc/system.h>
#include <ogc/timesupp.h>
#include <fat.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = -EINPROGRESS; // net_XYZ error results are negative
const cc_result ReturnCode_SocketWouldBlock = -EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = -EPIPE;

const char* Platform_AppNameSuffix = " GameCube";
cc_bool Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"
#include "Platform_GCWii.h"


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
	SYS_Report("%.*s\n", len, msg);
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

cc_uint64 Stopwatch_Measure(void) {
	return __SYS_GetSystemTime();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ticks_to_microsecs(end - beg);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	// DNS resolution not implemented in gamecube libbba
	static struct fixed_dns_map {
		const cc_string host, ip[2];
	} mappings[] = {
		{ String_FromConst("cdn.classicube.net"),               { String_FromConst("172.66.134.165"), String_FromConst("172.66.138.91") }},
		{ String_FromConst("static.classicube.net"),            { String_FromConst("172.66.134.165"), String_FromConst("172.66.138.91") }},
		{ String_FromConst("www.classicube.net"),               { String_FromConst("172.66.134.165"), String_FromConst("172.66.138.91") }},
		{ String_FromConst("resources.download.minecraft.net"), { String_FromConst("13.107.213.36"),  String_FromConst("13.107.246.36") }},
		{ String_FromConst("launcher.mojang.com"),              { String_FromConst("13.107.213.36"),  String_FromConst("13.107.246.36") }}
	};
	if (!net_supported) return ERR_NO_NETWORKING;

	for (int i = 0; i < Array_Elems(mappings); i++) 
	{
		if (!String_CaselessEqualsConst(&mappings[i].host, host)) continue;

		ParseIPv4(&mappings[i].ip[0], port, &addrs[0]);
		ParseIPv4(&mappings[i].ip[1], port, &addrs[1]);
		*numValidAddrs = 2;
		return 0;
	}
	return ERR_NOT_SUPPORTED;
}

// libogc only implements net_select for gamecube currently
static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int res; // number of 'ready' sockets
	FD_ZERO(&set);
	FD_SET(s, &set);
	if (mode == SOCKET_POLL_READ) {
		res = net_select(s + 1, &set, NULL, NULL, &time);
	} else {
		res = net_select(s + 1, NULL, &set, NULL, &time);
	}
	if (res < 0) { *success = false; return res; }
	*success = FD_ISSET(s, &set) != 0; return 0;
}

cc_result Socket_GetLastError(cc_socket s) {
	int error   = ERR_INVALID_ARGUMENT;
	u32 errSize = sizeof(error);
	
	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	net_getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &errSize);
	return error;
}

static void InitSockets(void) {
	// https://github.com/devkitPro/wii-examples/blob/master/devices/network/sockettest/source/sockettest.c
	char localip[32] = {0};
	char netmask[32] = {0};
	char gateway[32] = {0};
	
	VirtualDialog_Show("Connecting to network..", "This may take up to 30 seconds", true);
	int ret = if_config(localip, netmask, gateway, true);

	if (ret >= 0) {
		cc_string str; char buffer[256];
		String_InitArray_NT(str, buffer);
		String_Format3(&str, "IP address: %c\nGateway IP: %c\nNetmask %c", 
							localip, gateway, netmask);

		buffer[str.length] = '\0';
		Window_ShowDialog("Networking details", buffer);
	} else {
		Logger_SimpleWarn(ret, "setting up network");
		net_supported = false;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Synchronisation-----------------------------------------------------*
*#########################################################################################################################*/
void* Mutex_Create(const char* name) {
	mutex_t* ptr = (mutex_t*)Mem_Alloc(1, sizeof(mutex_t), "mutex");
	int res = LWP_MutexInit(ptr, false);
	if (res) Process_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	mutex_t* mutex = (mutex_t*)handle;
	int res = LWP_MutexDestroy(*mutex);
	if (res) Process_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	mutex_t* mutex = (mutex_t*)handle;
	int res = LWP_MutexLock(*mutex);
	if (res) Process_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	mutex_t* mutex = (mutex_t*)handle;
	int res = LWP_MutexUnlock(*mutex);
	if (res) Process_Abort2(res, "Unlocking mutex");
}

void* Waitable_Create(const char* name) {
	sem_t* ptr = (sem_t*)Mem_Alloc(1, sizeof(sem_t), "waitable");
	int res = LWP_SemInit(ptr, 0, 1);
	if (res) Process_Abort2(res, "Creating waitable");
	return ptr;
}

void Waitable_Free(void* handle) {
	sem_t* ptr = (sem_t*)handle;
	int res = LWP_SemDestroy(*ptr);
	if (res) Process_Abort2(res, "Destroying waitable");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	sem_t* ptr = (sem_t*)handle;
	int res = LWP_SemPost(*ptr);
	if (res && res != EOVERFLOW) Process_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	sem_t* ptr = (sem_t*)handle;
	int res = LWP_SemWait(*ptr);
	if (res) Process_Abort2(res, "Event wait");
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	sem_t* ptr = (sem_t*)handle;
	struct timespec ts;
	int res;

	ts.tv_sec  = milliseconds  / TB_MSPERSEC;
	ts.tv_nsec = (milliseconds % TB_MSPERSEC) * TB_NSPERMS;

	res = LWP_SemTimedWait(*ptr, &ts);
	if (res && res != ETIMEDOUT) Process_Abort2(res, "Event timed wait");
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "GameCubeGameCube"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
#endif

