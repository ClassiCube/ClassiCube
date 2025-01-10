#ifndef CC_WINSOCK_FUNC
#define CC_WINSOCK_FUNC
#endif

/* Not available on older SDKs */
typedef cc_uintptr _UINT_PTR;

/* IP addresses */
typedef struct sockaddr {
	USHORT sa_family;
	char   sa_data[14];
} SOCKADDR;

typedef struct in_addr {
	union in_data {
		struct { UCHAR s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { USHORT s_w1,s_w2; } S_un_w;
		ULONG S_addr;
	} S_un;
} IN_ADDR;

typedef struct sockaddr_in {
	SHORT   sin_family;
	USHORT  sin_port;
	struct  in_addr sin_addr;
	char    sin_zero[8];
} SOCKADDR_IN;

typedef struct in6_addr {
	union in6_data {
		UCHAR  Byte[16];
		USHORT Word[8];
	} u;
} IN6_ADDR;

typedef struct sockaddr_in6 {
	USHORT sin6_family;
	USHORT sin6_port;
	ULONG  sin6_flowinfo;
	IN6_ADDR sin6_addr;
	ULONG sin6_scope_id;
} SOCKADDR_IN6;

typedef struct sockaddr_storage {
    short ss_family;
    CHAR  ss_pad[126];
} SOCKADDR_STORAGE;

/* Type declarations */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef _UINT_PTR SOCKET;

/* Constants */
#define AF_INET   2
#define AF_INET6 23

#define SOCK_STREAM 1

#define IPPROTO_TCP 6

#define SOL_SOCKET 0xffff

#define SO_ERROR 0x1007

#define SD_BOTH 0x02

#define IOC_IN 0x80000000
/* has input parameters | input size| func group | command */
#define FIONBIO (IOC_IN | (4 << 16) | ('f' << 8) | 126)

/* Struct declarations */
#define FD_SETSIZE 64
typedef struct fd_set {
	u_int  fd_count;
	SOCKET fd_array[FD_SETSIZE];
} fd_set;

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct  hostent {
	char*   h_name;
	char**  h_aliases;
	short   h_addrtype;
	short   h_length;
	char**  h_addr_list;
};

typedef struct addrinfo {
    int                 ai_flags;
    int                 ai_family;
    int                 ai_socktype;
    int                 ai_protocol;
    size_t              ai_addrlen;
    char *              ai_canonname;
    struct sockaddr *   ai_addr;
    struct addrinfo *   ai_next;
} ADDRINFOA;

#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128
typedef struct WSAData {
	WORD           wVersion;
	WORD           wHighVersion;
#ifdef _WIN64
	unsigned short iMaxSockets;
	unsigned short iMaxUdpDg;
	char*          lpVendorInfo;
	char           szDescription[WSADESCRIPTION_LEN + 1];
	char           szSystemStatus[WSASYS_STATUS_LEN + 1];
#else
	char           szDescription[WSADESCRIPTION_LEN + 1];
	char           szSystemStatus[WSASYS_STATUS_LEN + 1];
	unsigned short iMaxSockets;
	unsigned short iMaxUdpDg;
	char*          lpVendorInfo;
#endif
} WSADATA;

/* Function declarations */
CC_WINSOCK_FUNC int (WINAPI *_WSAStartup)(WORD versionRequested, WSADATA* wsaData);
CC_WINSOCK_FUNC int (WINAPI *_WSACleanup)(void);
CC_WINSOCK_FUNC int (WINAPI *_WSAGetLastError)(void);
CC_WINSOCK_FUNC int (WINAPI *_WSAStringToAddressW)(LPWSTR addressString, INT addressFamily, LPVOID protocolInfo, LPVOID address, LPINT addressLength);

CC_WINSOCK_FUNC int (WINAPI *_socket)(int af, int type, int protocol);
CC_WINSOCK_FUNC int (WINAPI *_closesocket)(SOCKET s);
CC_WINSOCK_FUNC int (WINAPI *_connect)(SOCKET s, const struct sockaddr* name, int namelen);
CC_WINSOCK_FUNC int (WINAPI *_shutdown)(SOCKET s, int how);

CC_WINSOCK_FUNC int (WINAPI *_ioctlsocket)(SOCKET s, long cmd, u_long* argp);
CC_WINSOCK_FUNC int (WINAPI *_getsockopt)(SOCKET s, int level, int optname, char* optval, int* optlen);
CC_WINSOCK_FUNC int (WINAPI *_recv)(SOCKET s, char* buf, int len, int flags);
CC_WINSOCK_FUNC int (WINAPI *_send)(SOCKET s, const char FAR * buf, int len, int flags);
CC_WINSOCK_FUNC int (WINAPI *_select)(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout);

CC_WINSOCK_FUNC struct hostent* (WINAPI *_gethostbyname)(const char* name);
CC_WINSOCK_FUNC unsigned short  (WINAPI *_htons)(u_short hostshort);
CC_WINSOCK_FUNC int  (WINAPI *_getaddrinfo )(PCSTR nodeName, PCSTR serviceName, const ADDRINFOA* hints, ADDRINFOA** result);
CC_WINSOCK_FUNC void (WINAPI* _freeaddrinfo)(ADDRINFOA* addrInfo);

static void Winsock_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_ReqSym(WSAStartup),      DynamicLib_ReqSym(WSACleanup),
		DynamicLib_ReqSym(WSAGetLastError), DynamicLib_OptSym(WSAStringToAddressW),
		DynamicLib_ReqSym(socket),          DynamicLib_ReqSym(closesocket),
		DynamicLib_ReqSym(connect),         DynamicLib_ReqSym(shutdown),
		DynamicLib_ReqSym(ioctlsocket),     DynamicLib_ReqSym(getsockopt),
		DynamicLib_ReqSym(gethostbyname),   DynamicLib_ReqSym(htons),
		DynamicLib_OptSym(getaddrinfo),     DynamicLib_OptSym(freeaddrinfo),
		DynamicLib_ReqSym(recv), DynamicLib_ReqSym(send), DynamicLib_ReqSym(select)
	};
	static const cc_string winsock1 = String_FromConst("wsock32.DLL");
	static const cc_string winsock2 = String_FromConst("WS2_32.DLL");
	void* lib;

	DynamicLib_LoadAll(&winsock2, funcs, Array_Elems(funcs), &lib);
	/* Windows 95 is missing WS2_32 dll */
	if (!lib) DynamicLib_LoadAll(&winsock1, funcs, Array_Elems(funcs), &lib);
}
