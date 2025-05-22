#include "Core.h"
#if defined CC_BUILD_SYMBIAN
extern "C" {
#include "Errors.h"
#include "Platform.h"
#include "Logger.h"
#include "String.h"
#define CC_XTEA_ENCRYPTION
#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "SystemFonts.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "PackedCol.h"

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <dlfcn.h>

#include <stdapis/string.h>
#include <stdapis/arpa/inet.h>
#include <stdapis/netinet/in.h>
#include <stdapis/sys/socket.h>
#include <stdapis/sys/ioctl.h>
#include <stdapis/sys/types.h>
#include <stdapis/sys/stat.h>
#include <stdapis/sys/time.h>
#include <stdapis/sys/select.h>
#include <stdapis/netdb.h>
}
#include <e32base.h>
#include <hal.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_SocketDropped    = EPIPE;
#define SUPPORTS_GETADDRINFO 1

const char* Platform_AppNameSuffix = "";
cc_bool Platform_SingleProcess = true;
cc_bool Platform_ReadonlyFilesystem;


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_Set(void*  dst, cc_uint8 value,  unsigned numBytes) { return (void*)memset( dst, value, numBytes); }
void* Mem_Copy(void* dst, const void* src, unsigned numBytes) { return (void*)memcpy( dst, src,   numBytes); }
void* Mem_Move(void* dst, const void* src, unsigned numBytes) { return (void*)memmove(dst, src,   numBytes); }

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? malloc(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	return calloc(numElems, elemsSize);
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? realloc(mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) free(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	int ret;
	/* Avoid "ignoring return value of 'write' declared with attribute 'warn_unused_result'" warning */
	ret = write(STDOUT_FILENO, msg,  len);
	ret = write(STDOUT_FILENO, "\n",   1);
}

TimeMS DateTime_CurrentUTC(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	struct timeval cur;
	struct tm loc_time;
	time_t s;
	
	gettimeofday(&cur, NULL);
	s = cur.tv_sec;
	localtime_r(&s, &loc_time);

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
static TInt tickPeriod;

static void Stopwatch_Init(void) {
	if (HAL::Get(HAL::ENanoTickPeriod, tickPeriod) != KErrNone) {
		User::Panic(_L("Could not init timer"), 0);
	}
}

cc_uint64 Stopwatch_Measure(void) {
	return (cc_uint64)User::NTickCount();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) * tickPeriod;
}


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
static void ExceptionHandler(TExcType type) {
	cc_string msg; char msgB[64];
	String_InitArray(msg, msgB);
	String_AppendConst(&msg, "Exception: ");
	String_AppendInt(&msg, (int) type);
	msg.buffer[msg.length] = '\0';
	
	Logger_DoAbort(0, msg.buffer, 0);
	
	User::HandleException((TUint32*) &type);
}

void CrashHandler_Install(void) {
#if !defined _DEBUG
	User::SetExceptionHandler(ExceptionHandler, 0xffffffff);
#endif
}

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	String_EncodeUtf8(str, path);
}

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create(const cc_filepath* path) {
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

#if defined CC_BUILD_SYMBIAN
		{
			char full_path[NATIVE_STR_LEN];
			struct stat sb;
			String_EncodeUtf8(full_path, &path);
			is_dir = stat(full_path, &sb) == 0 && S_ISDIR(sb.st_mode);
		}
#else
		is_dir = entry->d_type == DT_DIR;
		/* TODO: fallback to stat when this fails */
#endif

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
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
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
#define NS_PER_SEC 1000000000ULL

void Thread_Sleep(cc_uint32 milliseconds) { 
	User::After(milliseconds * 1000); 
}

static void* ExecThread(void* param) {
	((Thread_StartFunc)param)();
	return NULL;
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	pthread_t* ptr = (pthread_t*)Mem_Alloc(1, sizeof(pthread_t), "thread");
	pthread_attr_t attrs;
	int res;
	
	*handle = ptr;
	pthread_attr_init(&attrs);
	
	/* Symbian only supports up to 64 KB stack size */
	if (stackSize >= 64 * 1024) {
		stackSize = 64 * 1024;
	}
	
	pthread_attr_setstacksize(&attrs, stackSize);
	
	res = pthread_create(ptr, &attrs, ExecThread, (void*)func);
	if (res) Process_Abort2(res, "Creating thread");
	pthread_attr_destroy(&attrs);
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_detach(*ptr);
	if (res) Process_Abort2(res, "Detaching thread");
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_join(*ptr, NULL);
	if (res) Process_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(const char* name) {
	RMutex* mutex = new RMutex;
	if (!mutex) Process_Abort("Creating mutex");
	
	mutex->CreateLocal();
	return mutex;
}

void Mutex_Free(void* handle) {
	RMutex* mutex = (RMutex*)handle;
	
	mutex->Close();
	delete mutex;
}

void Mutex_Lock(void* handle) {
	RMutex* mutex = (RMutex*)handle;
	mutex->Wait();
}

void Mutex_Unlock(void* handle) {
	RMutex* mutex = (RMutex*)handle;
	mutex->Signal();
}

void* Waitable_Create(const char* name) {
	RSemaphore* sem = new RSemaphore;
	TInt res;
	if (!sem) Process_Abort("Creating waitable");
	
	res = sem->CreateLocal(0);
	if (res) Process_Abort2(res, "Creating waitable");
	return sem;
}

void Waitable_Free(void* handle) {
	RSemaphore* sem = (RSemaphore*)handle;
	
	sem->Close();
	delete sem;
}

void Waitable_Signal(void* handle) {
	RSemaphore* sem = (RSemaphore*)handle;
	
	sem->Signal();
	delete sem;
}

void Waitable_Wait(void* handle) {
	RSemaphore* sem = (RSemaphore*)handle;
	
	sem->Wait();
	delete sem;
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	RSemaphore* sem = (RSemaphore*)handle;
	
	sem->Wait(milliseconds * 1000);
	delete sem;
}


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj, int isDirectory) {
	if (isDirectory) {
		Directory_Enum(path, NULL, FontDirCallback);
	} else {
		SysFonts_Register(path, NULL);
	}
}

void Platform_LoadSysFonts(void) {
	int i;
	static const cc_string dirs[] = {
		String_FromConst("Z:\\resource\\fonts"),
		String_FromConst("C:\\resource\\fonts")
	};
	
	for (i = 0; i < Array_Elems(dirs); i++) 
	{
		Platform_Log1("Searching for fonts in %s", &dirs[i]);
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
	Platform_LogConst("Finished searching for fonts");
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
union SocketAddress {
	struct sockaddr   raw;
	struct sockaddr_in v4;
};
/* Sanity check to ensure cc_sockaddr struct is large enough to contain all socket addresses supported by this platform */
static char sockaddr_size_check[sizeof(union SocketAddress) < CC_SOCKETADDR_MAXSIZE ? 1 : -1];

#if SUPPORTS_GETADDRINFO
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
	if (res == EAI_AGAIN) return SOCK_ERR_UNKNOWN_HOST;
	if (res) return res;

	/* Prefer IPv4 addresses first */
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

	freeaddrinfo(result);
	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}
#else
static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	struct hostent* res = gethostbyname(host);
	struct sockaddr_in* addr4;
	char* src_addr;
	int i;
	
	// Must have at least one IPv4 address
	if (res->h_addrtype != AF_INET) return ERR_INVALID_ARGUMENT;
	if (!res->h_addr_list)          return ERR_INVALID_ARGUMENT;

	for (i = 0; i < SOCKET_MAX_ADDRS; i++) 
	{
		src_addr = res->h_addr_list[i];
		if (!src_addr) break;
		addrs[i].size = sizeof(struct sockaddr_in);

		addr4 = (struct sockaddr_in*)addrs[i].data;
		addr4->sin_family = AF_INET;
		addr4->sin_port   = htons(port);
		addr4->sin_addr   = *(struct in_addr*)src_addr;
	}

	*numValidAddrs = i;
	return i == 0 ? ERR_INVALID_ARGUMENT : 0;
}
#endif

cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	union SocketAddress* addr = (union SocketAddress*)addrs[0].data;
	char str[NATIVE_STR_LEN];

	String_EncodeUtf8(str, address);
	*numValidAddrs = 0;

	if (inet_pton(AF_INET,  str, &addr->v4.sin_addr)  > 0) {
		addr->v4.sin_family = AF_INET;
		addr->v4.sin_port   = htons(port);
		
		addrs[0].size  = sizeof(addr->v4);
		*numValidAddrs = 1;
		return 0;
	}
	
	return ParseHost(str, port, addrs, numValidAddrs);
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;

	*s = socket(raw->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (*s == -1) return errno;

	if (nonblocking) {
		int res = fcntl(*s, F_GETFL, 0);
		if (res < 0) return errno;
		
		res = fcntl(*s, F_SETFL, res | O_NONBLOCK);
		if (res < 0) return errno;
	}
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	struct sockaddr* raw = (struct sockaddr*)addr->data;
	
	int res = connect(s, raw, addr->size);
	return res == -1 ? errno : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int recvCount = recv(s, data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return errno;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = send(s, data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return errno;
}

void Socket_Close(cc_socket s) {
	shutdown(s, SHUT_RDWR);
	close(s);
}

#if defined CC_BUILD_DARWIN || defined CC_BUILD_BEOS || defined CC_BUILD_SYMBIAN
/* poll is broken on old OSX apparently https://daniel.haxx.se/docs/poll-vs-select.html */
/* BeOS lacks support for poll */
static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	FD_ZERO(&set);
	FD_SET(s, &set);

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(s + 1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(s + 1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return errno; }
	*success = FD_ISSET(s, &set) != 0; return 0;
}
#else
#include <poll.h>
static cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	if (poll(&pfd, 1, 0) == -1) { *success = false; return errno; }
	
	/* to match select, closed socket still counts as readable */
	flags    = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success = (pfd.revents & flags) != 0;
	return 0;
}
#endif

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return Socket_Poll(s, SOCKET_POLL_READ, readable);
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	socklen_t resultSize = sizeof(socklen_t);
	cc_result res = Socket_Poll(s, SOCKET_POLL_WRITE, writable);
	if (res || *writable) return res;

	/* https://stackoverflow.com/questions/29479953/so-error-value-after-successful-socket-operation */
	getsockopt(s, SOL_SOCKET, SO_ERROR, &res, &resultSize);
	return res;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = true;

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return SetGameArgs(args, numArgs);
}
void Process_Exit(cc_result code) { exit(code); }

/* implemented in Window_Symbian.cpp */
/* cc_result Process_StartOpen(const cc_string* args) */


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Updater_Supported = true;

const struct UpdaterInfo Updater_Info = {
	"&eRedownload and reinstall to update", 0, NULL
};

cc_bool Updater_Clean(void) { return true; }
cc_result Updater_Start(const char** action) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_MarkExecutable(void) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
const cc_string DynamicLib_Ext = String_FromConst(".dll");

void* DynamicLib_Load2(const cc_string* path) {
	cc_filepath str;
	Platform_EncodePath(&str, path);
	return dlopen(str.buffer, RTLD_NOW);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	void *result = dlsym(lib, name);
	return result;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	char* err = dlerror();
	if (err) String_AppendConst(dst, err);
	return err && err[0];
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Init(void) {
	cc_uintptr addr;
	signal(SIGCHLD, SIG_IGN);
	/* So writing to closed socket doesn't raise SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	/* Log runtime address of a known function to ease investigating crashes */
	/* (on platforms with ASLR, function addresses change every time when run) */
	addr = (cc_uintptr)Process_Exit;
	Platform_Log1("Process_Exit addr: %x", &addr);
	
	Stopwatch_Init();
}
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	const char* err = strerror(res);
	if (!err || res >= 1000) return false;

	String_AppendUtf8(dst, err, String_Length(err));
	return true;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	// Directory is already set by platform: !:/private/e212a5c2
	return 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
	
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}

#define MACHINE_KEY "Symbian_Symbian_"

static cc_result GetMachineID(cc_uint32* key) {
	TInt res;
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);

	if (HAL::Get(HAL::ESerialNumber, res) == KErrNone) {
		key[0] = res;
	}
	if (HAL::Get(HAL::EMachineUid, res) == KErrNone) {
		key[1] = res;
	}
	return 0;
}
#endif

