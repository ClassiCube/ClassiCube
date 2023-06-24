#include "Platform.h"
#include "String.h"
#include "Logger.h"
#include "Constants.h"
cc_bool Platform_SingleProcess;

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
int Mem_Equal(const void* a, const void* b, cc_uint32 numBytes) {
	const cc_uint8* src = (const cc_uint8*)a;
	const cc_uint8* dst = (const cc_uint8*)b;

	while (numBytes--) { 
		if (*src++ != *dst++) return false; 
	}
	return true;
}

CC_NOINLINE static void AbortOnAllocFailed(const char* place) {	
	cc_string log; char logBuffer[STRING_SIZE+20 + 1];
	String_InitArray_NT(log, logBuffer);

	String_Format1(&log, "Out of memory! (when allocating %c)", place);
	log.buffer[log.length] = '\0';
	Logger_Abort(log.buffer);
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
	if (!numElems) return 1; /* treat 0 size as 1 byte */
	cc_uint32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	if (numBytes < numElems) return 0; /* TODO: Use proper overflow checking */
	return numBytes;
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
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

int Stopwatch_ElapsedMS(cc_uint64 beg, cc_uint64 end) {
	cc_uint64 raw = Stopwatch_ElapsedMicroseconds(beg, end);
	if (raw > Int32_MaxValue) return Int32_MaxValue / 1000;
	return (int)raw / 1000;
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
	int i, loaded = 0;
	void* addr;
	void* lib;

	lib   = DynamicLib_Load2(path);
	*_lib = lib;
	if (!lib) { Logger_DynamicLibWarn("loading", path); return false; }

	for (i = 0; i < count; i++) {
		addr = DynamicLib_Get2(lib, syms[i].name);
		if (addr) loaded++;
		*syms[i].symAddr = addr;
	}
	return loaded == count;
}
