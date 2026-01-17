#include "../Stream.h"
#include "../ExtMath.h"
#include "../SystemFonts.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../Game.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <emscripten.h>

#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR   0x002
#define O_CREAT  0x040
#define O_EXCL   0x080
#define O_TRUNC  0x200

// Unfortunately, errno constants are different in some older emscripten versions
//  (linux errno numbers compared to WASI errno numbers)
// So just use the same errono numbers as interop_web.js
#define _ENOENT        2
#define _EAGAIN        6 // same as EWOULDBLOCK
#define _EEXIST       17
#define _EHOSTUNREACH 23
#define _EINPROGRESS  26

const cc_result ReturnCode_FileShareViolation = 1000000000; // Not used in web filesystem backend
const cc_result ReturnCode_FileNotFound     = _ENOENT;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_SocketInProgess  = _EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = _EAGAIN;
const cc_result ReturnCode_DirectoryExists  = _EEXIST;

const char* Platform_AppNameSuffix = "";
cc_uint8 Platform_Flags;
cc_bool  Platform_ReadonlyFilesystem;
#include "../_PlatformBase.h"


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}

cc_uint64 Stopwatch_Measure(void) {
	/* time is a milliseconds double */
	/*  convert to microseconds */
	return (cc_uint64)(emscripten_get_now() * 1000);
}

extern void interop_Log(const char* msg, int len);
void Platform_Log(const char* msg, int len) {
	interop_Log(msg, len);
}

TimeMS DateTime_CurrentUTC(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

extern void interop_GetLocalTime(struct cc_datetime* t);
void DateTime_CurrentLocal(struct cc_datetime* t) {
	interop_GetLocalTime(t);
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
void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	String_EncodeUtf8(str, path);
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	const char* str = path->buffer;
	String_AppendUtf8(dst, str, String_Length(str));
}

void Directory_GetCachePath(cc_string* path) { }

extern void interop_InitFilesystem(void);
cc_result Directory_Create2(const cc_filepath* path) {
	/* Web filesystem doesn't need directories */
	return 0;
}

extern int interop_FileExists(const char* path);
int File_Exists(const cc_filepath* path) {
	return interop_FileExists(path->buffer);
}

static void* enum_obj;
static Directory_EnumCallback enum_callback;
EMSCRIPTEN_KEEPALIVE void Directory_IterCallback(const char* src) {
	cc_string path; char pathBuffer[FILENAME_SIZE];

	String_InitArray(path, pathBuffer);
	String_AppendUtf8(&path, src, String_Length(src));
	enum_callback(&path, enum_obj, false);
}

extern int interop_DirectoryIter(const char* path);
cc_result Directory_Enum(const cc_string* path, void* obj, Directory_EnumCallback callback) {
	cc_filepath str;
	Platform_EncodePath(&str, path);

	enum_obj      = obj;
	enum_callback = callback;
	/* returned result is negative for error */
	return -interop_DirectoryIter(str.buffer);
}

extern int interop_FileCreate(const char* path, int mode);
static cc_result File_Do(cc_file* file, const char* path, int mode) {
	int res = interop_FileCreate(path, mode);

	/* returned result is negative for error */
	if (res >= 0) {
		*file = res; return 0;
	} else {
		*file = -1; return -res;
	}
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

extern int interop_FileRead(int fd, void* data, int count);
cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	int res = interop_FileRead(file, data, count);

	/* returned result is negative for error */
	if (res >= 0) {
		*bytesRead = res; return 0;
	} else {
		*bytesRead = -1;   return -res;
	}
}

extern int interop_FileWrite(int fd, const void* data, int count);
cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	int res = interop_FileWrite(file, data, count);

	/* returned result is negative for error */
	if (res >= 0) {
		*bytesWrote = res; return 0;
	} else {
		*bytesWrote = -1;  return -res;
	}
}

extern int interop_FileClose(int fd);
cc_result File_Close(cc_file file) {
	/* returned result is negative for error */
	return -interop_FileClose(file);
}

extern int interop_FileSeek(int fd, int offset, int whence);
cc_result File_Seek(cc_file file, int offset, int seekType) {
	/* returned result is negative for error */
	int res = interop_FileSeek(file, offset, seekType);
	/* FileSeek returns current position, discard that */
	return res >= 0 ? 0 : -res;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	/* FILE_SEEKFROM_CURRENT is same as SEEK_CUR */
	int res = interop_FileSeek(file, 0, FILE_SEEKFROM_CURRENT);
	/* returned result is negative for error */
	if (res >= 0) {
		*pos = res; return 0;
	} else {
		*pos = -1;  return -res;
	}
}

extern int interop_FileLength(int fd);
cc_result File_Length(cc_file file, cc_uint32* len) {
	int res = interop_FileLength(file);
	/* returned result is negative for error */
	if (res >= 0) {
		*len = res; return 0;
	} else {
		*len = -1;  return -res;
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
/* No real threading support with emscripten backend */
void  Thread_Sleep(cc_uint32 milliseconds) { }

void* Mutex_Create(const char* name) { return NULL; }
void  Mutex_Free(void* handle) { }
void  Mutex_Lock(void* handle) { }
void  Mutex_Unlock(void* handle) { }

void* Waitable_Create(const char* name) { return NULL; }
void  Waitable_Free(void* handle) { }
void  Waitable_Signal(void* handle) { }
void  Waitable_Wait(void* handle) { }
void  Waitable_WaitFor(void* handle, cc_uint32 milliseconds) { }


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
void Platform_LoadSysFonts(void) { }


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
extern void interop_InitSockets(void);

cc_bool SockAddr_ToString(const cc_sockaddr* addr, cc_string* dst) {
	const char* str = (const char*)addr->data;

	String_AppendUtf8(dst, str, String_Length(str));
	String_Format1(dst, ":%i", &addr->size);
	return true;
}



// not actually ipv4 address, just copies across hostname
static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst) {
	int len = String_EncodeUtf8(dst->data, ip);
	/* TODO can this ever happen */
	if (len >= CC_SOCKETADDR_MAXSIZE) Process_Abort("Overrun in Socket_ParseAddress");

	dst->size = port;
	return true;
}

static cc_bool ParseIPv6(const char* ip, int port, cc_sockaddr* dst) {
	return false;
}

static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	return 1;
}

extern int interop_SocketCreate(void);
extern int interop_SocketConnect(int sock, const cc_uint8* host, int port);

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	*s = interop_SocketCreate();
	return 0;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	/* size is used to store port number instead */
	/* returned result is negative for error */
	int res = -interop_SocketConnect(s, addr->data, addr->size);

	/* error returned when invalid address provided */
	if (res == _EHOSTUNREACH) return ERR_INVALID_ARGUMENT;
	return res;
}

extern int interop_SocketRecv(int sock, void* data, int len);
cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* read) {
	int res; 
	*read = 0;

	// interop_SocketRecv only reads one WebSocket frame at most, hence call it multiple times
	while (count) {
		// returned result is negative for error
		res = interop_SocketRecv(s, data, count);

		if (res >= 0) {
			*read += res;
			data  += res; count -= res;
		} else {
			// EAGAIN when no more data available
			if (res == -_EAGAIN) return *read == 0 ? _EAGAIN : 0;

			return -res;
		}
	}
	return 0;
}

extern int interop_SocketSend(int sock, const void* data, int len);
cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	/* returned result is negative for error */
	int res = interop_SocketSend(s, data, count);

	if (res >= 0) {
		*modified = res; return 0;
	} else {
		*modified = 0; return -res;
	}
}

extern int interop_SocketClose(int sock);
void Socket_Close(cc_socket s) {
	interop_SocketClose(s);
}

extern int interop_SocketWritable(int sock, cc_bool* writable);
cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	// returned result is negative for error
	return -interop_SocketWritable(s, writable);
}

extern int interop_SocketLastError(int sock);
cc_result Socket_GetLastError(cc_socket s) {
	// returned result is negative for error
	return -interop_SocketLastError(s);
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = true;

void Process_Exit(cc_result code) {
	Window_Free(); // 'Window' (i.e. the web canvas) isn't implicitly closed when process is exited
	emscripten_cancel_main_loop();
	exit(code);
}

extern int interop_OpenTab(const char* url);
cc_result Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	cc_result res;
	String_EncodeUtf8(str, args);

	res = interop_OpenTab(str);
	/* interop error code -> ClassiCube error code */
	if (res == 1) res = ERR_INVALID_OPEN_URL;
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char* str;
	int len;

	/* For unrecognised error codes, strerror might return messages */
	/*  such as 'No error information', which is not very useful */
	if (res >= 1000) return false;

	str = strerror(res);
	if (!str) return false;

	len = String_CalcLen(str, NATIVE_STR_LEN);
	String_AppendUtf8(dst, str, len);
	return true;
}

EMSCRIPTEN_KEEPALIVE void Platform_LogError(const char* msg) {
	cc_string str = String_FromReadonly(msg);
	Logger_WarnFunc(&str);
}

extern void interop_InitModule(void);
void Platform_Init(void) {
	interop_InitModule();
	interop_InitFilesystem();
	interop_InitSockets();
}
void Platform_Free(void) { }

cc_result Platform_SetDefaultCurrentDirectory(int argc, char** argv) { return 0; }


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) { 
	return ERR_NOT_SUPPORTED; 
}
cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) { 
	return ERR_NOT_SUPPORTED; 
}
cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*------------------------------------------------------Main driver--------------------------------------------------------*
*#########################################################################################################################*/
static void DoNextFrame(void) {
	if (Game_Running) {
		Game_RenderFrame();
		return;
	}

	Game_Free();
	Window_Free();
	emscripten_cancel_main_loop();
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; // skip executable path argument

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) { args[i] = String_FromReadonly(argv[i]); }
	return count;
}

#include "../main_impl.h"
static int    _argc;
static char** _argv;

// webclient does some asynchronous initialisation first, then kickstarts the game after that
static void web_main(void) {
	SetupProgram(_argc, _argv);

	switch (ProcessProgramArgs(_argc, _argv))
	{
	case ARG_RESULT_RUN_LAUNCHER:
		String_AppendConst(&Game_Username, DEFAULT_USERNAME);
		/* fallthrough */
	case ARG_RESULT_RUN_GAME:
		// This needs to be called before Game_Setup, as that
		//  in turn calls Game_Load -> Gfx_Create -> GLContext_SetVSync
		// (which requires a main loop to already be running)
		emscripten_cancel_main_loop();
		emscripten_set_main_loop(DoNextFrame, 0, false);

		Game_Setup();
		return;

	default:
		Process_Exit(1);
	}
}


extern void interop_FS_Init(void);
extern void interop_DirectorySetWorking(const char* path);
extern void interop_AsyncDownloadTexturePack(const char* path);

EMSCRIPTEN_KEEPALIVE int main(int argc, char** argv) {
	_argc = argc; _argv = argv;

	// Game loads resources asynchronously, then actually starts itself:
	// main
	//  > texture pack download (async)
	//     > load indexedDB (async)
	//        > web_main (game actually starts)
	interop_FS_Init();
	interop_DirectorySetWorking("/classicube");
	interop_AsyncDownloadTexturePack("texpacks/default.zip");
}

extern void interop_LoadIndexedDB(void);
extern void interop_AsyncLoadIndexedDB(void);

// Asynchronous callback after texture pack is downloaded
EMSCRIPTEN_KEEPALIVE void main_phase1(void) {
	interop_LoadIndexedDB(); // legacy compatibility
	interop_AsyncLoadIndexedDB();
}

// Asynchronous callback after IndexedDB is loaded
EMSCRIPTEN_KEEPALIVE void main_phase2(void) { web_main(); }

