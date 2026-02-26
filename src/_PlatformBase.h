#include "Platform.h"
#include "String_.h"
#include "Logger.h"
#include "Constants.h"
#include "Errors.h"
#include "Funcs.h"

#if CC_BUILD_MAXSTACK < (64 * 1024)

#if defined CC_BUILD_32X || defined CC_BUILD_GBA
static CC_BIG_VAR int temp_mem[31000 / 4];
#else
static CC_BIG_VAR int temp_mem[45000 / 4];
#endif

void* TempMem_Alloc(int size) {
	if (size > sizeof(temp_mem)) Process_Abort("TempMem overflow");

	return (void*)temp_mem;
}
#endif


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
int Mem_Equal(const void* a, const void* b, cc_uint32 numBytes) {
	const char* src = (const char*)a;
	const char* dst = (const char*)b;

	while (numBytes--) { 
		if (*src++ != *dst++) return false; 
	}
	return true;
}

#ifdef CC_BUILD_NOSTDLIB
void* Mem_Set(void* dst, cc_uint8 value,  unsigned numBytes) {
	char* dp = (char*)dst;

	while (numBytes--) *dp++ = value; /* TODO optimise */
	return dst;
}

void* Mem_Copy(void* dst, const void* src, unsigned numBytes) {
	char* sp = (char*)src;
	char* dp = (char*)dst;

	while (numBytes--) *dp++ = *sp++; /* TODO optimise */
	return dst;
}

void* Mem_Move(void* dst, const void* src, unsigned numBytes) { 
	char* sp = (char*)src;
	char* dp = (char*)dst;
	
	/* Check if destination range overlaps source range */
	/* If this happens, then need to copy backwards */
	if (dp >= sp && dp < (sp + numBytes)) {
		sp += numBytes;
		dp += numBytes;

		while (numBytes--) *--dp = *--sp;
	} else {
		while (numBytes--) *dp++ = *sp++;
	}
	return dst;
}
#else
void* Mem_Set(void*  dst, cc_uint8 value,  unsigned numBytes) { return (void*)memset( dst, value, numBytes); }
void* Mem_Copy(void* dst, const void* src, unsigned numBytes) { return (void*)memcpy( dst, src,   numBytes); }
void* Mem_Move(void* dst, const void* src, unsigned numBytes) { return (void*)memmove(dst, src,   numBytes); }
#endif


/*########################################################################################################################*
*---------------------------------------------------Memory management-----------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE static void AbortOnAllocFailed(const char* place) {	
	cc_string log; char logBuffer[STRING_SIZE+20 + 1];
	String_InitArray_NT(log, logBuffer);

	String_Format1(&log, "Out of memory! (when allocating %c)", place);
	log.buffer[log.length] = '\0';
	Process_Abort(log.buffer);
}

void* Mem_Alloc(cc_uint32 numElems, cc_uint32 elemsSize, const char* place) {
	void* ptr = Mem_TryAlloc(numElems, elemsSize);
	if (!ptr) AbortOnAllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(cc_uint32 numElems, cc_uint32 elemsSize, const char* place) {
	void* ptr = Mem_TryAllocCleared(numElems, elemsSize);
	if (!ptr) AbortOnAllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize, const char* place) {
	void* ptr = Mem_TryRealloc(mem, numElems, elemsSize);
	if (!ptr) AbortOnAllocFailed(place);
	return ptr;
}

static CC_NOINLINE cc_uint32 CalcMemSize(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 numBytes;
	if (!numElems || !elemsSize) return 1; /* treat 0 size as 1 byte */
	
	numBytes = numElems * elemsSize;
	if (numBytes / elemsSize != numElems) return 0; /* Overflow */
	return numBytes;
}

#ifndef OVERRIDE_MEM_FUNCTIONS
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
#endif


/*########################################################################################################################*
*--------------------------------------------------------Logging----------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log1(const char* format, const void* a1) {
	Platform_Log4(format, a1, NULL, NULL, NULL);
}
void Platform_Log2(const char* format, const void* a1, const void* a2) {
	Platform_Log4(format, a1, a2, NULL, NULL);
}
void Platform_Log3(const char* format, const void* a1, const void* a2, const void* a3) {
	Platform_Log4(format, a1, a2, a3, NULL);
}

void Platform_Log4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	cc_string msg; char msgBuffer[512];
	String_InitArray(msg, msgBuffer);

	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(msg.buffer, msg.length);
}

void Platform_LogConst(const char* message) {
	Platform_Log(message, String_Length(message));
}

/*########################################################################################################################*
*---------------------------------------------------Command line args-----------------------------------------------------*
*#########################################################################################################################*/
static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;
static cc_bool gameHasArgs;

static CC_INLINE cc_result SetGameArgs(const cc_string* args, int numArgs) {
	int i;
	for (i = 0; i < numArgs; i++) 
	{
		String_CopyToRawArray(gameArgs[i], &args[i]);
	}
	
	gameHasArgs = true;
	gameNumArgs = numArgs;
	return 0;
}

static CC_INLINE int GetGameArgs(cc_string* args) {
	int i, count = gameNumArgs;
	for (i = 0; i < count; i++) 
	{
		args[i] = String_FromRawArray(gameArgs[i]);
	}
	
	/* clear arguments so after game is closed, launcher is started */
	gameNumArgs = 0;
	return count;
}

#ifdef DEFAULT_COMMANDLINE_FUNC
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	if (gameHasArgs) return GetGameArgs(args);
	/* Consoles *sometimes* doesn't use argv[0] for program name and so argc will be 0 */
	//* (e.g. when running via some emulators) */
	if (!argc) return 0;
	
	argc--; argv++; /* skip executable path argument */

	int count = min(argc, GAME_MAX_CMDARGS);
	Platform_Log1("ARGS: %i", &count);
	
	for (int i = 0; i < count; i++) 
	{
		args[i] = String_FromReadonly(argv[i]);
		Platform_Log2("  ARG %i = %c", &i, argv[i]);
	}
	return count;
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Misc-----------------------------------------------------------*
*#########################################################################################################################*/
int Stopwatch_ElapsedMS(cc_uint64 beg, cc_uint64 end) {
	cc_uint64 raw = Stopwatch_ElapsedMicroseconds(beg, end);
	if (raw > Int32_MaxValue) return Int32_MaxValue / 1000;
	return (int)raw / 1000;
}

static CC_INLINE void SocketAddr_Set(cc_sockaddr* addr, const void* src, unsigned srcLen) {
	if (srcLen > CC_SOCKETADDR_MAXSIZE) Process_Abort("Attempted to copy too large socket");

	Mem_Copy(addr->data, src, srcLen);
	addr->size = srcLen;
}


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
cc_result DynamicLib_Load(const cc_string* path, void** lib) {
	*lib = DynamicLib_Load2(path);
	return *lib == NULL;
}
cc_result DynamicLib_Get(void* lib, const char* name, void** symbol) {
	*symbol = DynamicLib_Get2(lib, name);
	return *symbol == NULL;
}


cc_bool DynamicLib_LoadAll(const cc_string* path, const struct DynamicLibSym* syms, int count, void** _lib) {
	cc_bool foundAllRequired = true;
	cc_string symName;
	int i;
	void* addr;
	void* lib;

	lib   = DynamicLib_Load2(path);
	*_lib = lib;
	if (!lib) { Logger_DynamicLibWarn("loading", path); return false; }

	for (i = 0; i < count; i++) {
		addr = DynamicLib_Get2(lib, syms[i].name);
		*syms[i].symAddr = addr;
				
		if (addr || !syms[i].required) continue;
		symName = String_FromReadonly(syms[i].name);
		
		Logger_DynamicLibWarn("loading symbol", &symName);
		foundAllRequired = false;
	}
	return foundAllRequired;
}


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_XTEA_ENCRYPTION

/* Encrypts data using XTEA block cipher, with OS specific method to get machine-specific key */
static void EncipherBlock(cc_uint32* v, const cc_uint32* key, cc_string* dst) {
	cc_uint32 v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	int i;

	for (i = 0; i < 12; i++) 
	{
		v0  += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1  += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
	}
	v[0] = v0; v[1] = v1;
	String_AppendAll(dst, v, 8);
}

static void DecipherBlock(cc_uint32* v, const cc_uint32* key) {
	cc_uint32 v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta * 12;
	int i;

	for (i = 0; i < 12; i++) 
	{
		v1  -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
		sum -= delta;
		v0  -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	v[0] = v0; v[1] = v1;
}

#define ENC1 0xCC005EC0
#define ENC2 0x0DA4A0DE 
#define ENC3 0xC0DED000
#define MACHINEID_LEN 32
#define ENC_SIZE 8 /* 2 32 bit ints per block */

static cc_result GetMachineID(cc_uint32* key);

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4] = { 0 };
	cc_result res;
	if ((res = GetMachineID(key))) return res;

	header[0] = ENC1; header[1] = ENC2;
	header[2] = ENC3; header[3] = len;
	EncipherBlock(header + 0, key, dst);
	EncipherBlock(header + 2, key, dst);

	for (; len > 0; len -= ENC_SIZE, src += ENC_SIZE) 
	{
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));
		EncipherBlock(header, key, dst);
	}
	return 0;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4] = { 0 };
	cc_result res;
	int dataLen;

	/* Total size must be >= header size */
	if (len < 16) return ERR_END_OF_STREAM;
	if ((res = GetMachineID(key))) return res;

	Mem_Copy(header, src, 16);
	DecipherBlock(header + 0, key);
	DecipherBlock(header + 2, key);

	if (header[0] != ENC1 || header[1] != ENC2 || header[2] != ENC3) return ERR_INVALID_ARGUMENT;
	len -= 16; src += 16;

	if (header[3] > len) return ERR_INVALID_ARGUMENT;
	dataLen = header[3];

	for (; dataLen > 0; len -= ENC_SIZE, src += ENC_SIZE, dataLen -= ENC_SIZE) 
	{
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));

		DecipherBlock(header, key);
		String_AppendAll(dst, header, min(dataLen, ENC_SIZE));
	}
	return 0;
}
#endif


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NETWORKING
/* Encodes port number in network (i.e. big endian) byte order) */
static CC_INLINE cc_uint16 SockAddr_EncodePort(int port) {
	cc_uint16 raw;
	Stream_SetU16_BE((cc_uint8*)&raw, port);
	return raw;
}

/* Parses IPv4 addresses in the form a.b.c.d */
static CC_INLINE cc_bool ParseIPv4Address(const cc_string* addr, cc_uint32* ip) {
	cc_string parts[5];
	int i;

	union ipv4_addr_raw {
		cc_uint8 bytes[4];
		cc_uint32 value;
	} raw;
	
	/* 4+1 in case user tries '1.1.1.1.1' */
	if (String_UNSAFE_Split(addr, '.', parts, 4 + 1) != 4) return false;

	for (i = 0; i < 4; i++) 
	{
		if (!Convert_ParseUInt8(&parts[i], &raw.bytes[i])) return false;
	}

	*ip = raw.value;
	return true;
}


static cc_bool ParseIPv4(const cc_string* ip, int port, cc_sockaddr* dst);
static cc_bool ParseIPv6(const char* ip, int port, cc_sockaddr* dst);
static cc_result ParseHost(const char* host, int port, cc_sockaddr* addrs, int* numValidAddrs);

cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	char str[NATIVE_STR_LEN];

	if (ParseIPv4(address, port, &addrs[0])) {
		*numValidAddrs = 1;
		return 0;
	}

	String_EncodeUtf8(str, address);
	if (ParseIPv6(str, port, &addrs[0])) {
		*numValidAddrs = 1;
		return 0;
	}

	*numValidAddrs = 0;
	return ParseHost(str, port, addrs, numValidAddrs);
}


static CC_INLINE cc_bool IPv4_ToString(const void* ip, const void* port, cc_string* dst) {
	int portNum = Stream_GetU16_BE(port);
	char* rawIP = (char*)ip;

	String_Format4(dst, "%b.%b.%b.%b", &rawIP[0], &rawIP[1], &rawIP[2], &rawIP[3]);
	String_Format1(dst, ":%i", &portNum);
	return true;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_NO_UPDATER
cc_bool Updater_Supported = false;
cc_bool Updater_Clean(void) { return true; }

const struct UpdaterInfo Updater_Info = { "&eRedownload or recompile to update", 0 };

cc_result Updater_Start(const char** action) {
	*action = "Starting game";
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) { return ERR_NOT_SUPPORTED; }

cc_result Updater_MarkExecutable(void) { return ERR_NOT_SUPPORTED; }

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) { return ERR_NOT_SUPPORTED; }
#endif


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_NO_DYNLIB
const cc_string DynamicLib_Ext = String_FromConst(".so");

void* DynamicLib_Load2(const cc_string* path)      { return NULL; }
void* DynamicLib_Get2(void* lib, const char* name) { return NULL; }

cc_bool DynamicLib_DescribeError(cc_string* dst)   { return false; }
#endif


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_NO_SOCKETS
cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

void Socket_Close(cc_socket s) { }

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_GetLastError(cc_socket s) {
	return ERR_INVALID_ARGUMENT;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_NO_THREADING
void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	*handle = NULL;
}

void Thread_Detach(void* handle) {
}

void Thread_Join(void* handle) {
}

void* Mutex_Create(const char* name) {
	return NULL;
}

void Mutex_Free(void* handle) {
}

void Mutex_Lock(void* handle) {
}

void Mutex_Unlock(void* handle) {
}

void* Waitable_Create(const char* name) {
	return NULL;
}

void Waitable_Free(void* handle) {
}

void Waitable_Signal(void* handle) {
}

void Waitable_Wait(void* handle) {
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
}
#endif
