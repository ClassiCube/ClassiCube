#include "../Core.h"
#if defined CC_BUILD_GAMECUBE

#define CC_XTEA_ENCRYPTION
#define CC_NO_UPDATER
#define CC_NO_DYNLIB

#include "../_PlatformBase.h"
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
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <ogc/cond.h>
#include <ogc/lwp_watchdog.h>
#include <fat.h>
#include <ogc/exi.h>
#include "../_PlatformConsole.h"

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = -EINPROGRESS; // net_XYZ error results are negative
const cc_result ReturnCode_SocketWouldBlock = -EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = -EPIPE;

const char* Platform_AppNameSuffix = " GameCube";
cc_bool Platform_ReadonlyFilesystem;
#include "Platform_GCWii.h"


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	// DNS resolution not implemented in gamecube libbba
	static struct fixed_dns_map {
		const cc_string host, ip;
	} mappings[] = {
		{ String_FromConst("cdn.classicube.net"), String_FromConst("104.20.90.158") },
		{ String_FromConst("www.classicube.net"), String_FromConst("104.20.90.158") }
	};
	if (!net_supported) return ERR_NO_NETWORKING;

	for (int i = 0; i < Array_Elems(mappings); i++) 
	{
		if (!String_CaselessEqualsConst(&mappings[i].host, host)) continue;

		ParseIPv4(&mappings[i].ip, port, &addrs[0]);
		*numValidAddrs = 1;
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
	net_getsockopt(s, SOL_SOCKET, SO_ERROR, &error, errSize);
	return error;
}

static void InitSockets(void) {
	// https://github.com/devkitPro/wii-examples/blob/master/devices/network/sockettest/source/sockettest.c
	char localip[16] = {0};
	char netmask[16] = {0};
	char gateway[16] = {0};
	
	int ret = if_config(localip, netmask, gateway, TRUE, 20);
	if (ret >= 0) {
		Platform_Log3("Network ip: %c, gw: %c, mask %c", localip, gateway, netmask);
	} else {
		Logger_SimpleWarn(ret, "setting up network");
		net_supported = false;
	}
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

