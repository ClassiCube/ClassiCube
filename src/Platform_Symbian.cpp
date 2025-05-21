#include "Core.h"
#if defined CC_BUILD_SYMBIAN
extern "C" {
#include "Errors.h"
#include "Platform.h"
#include "Logger.h"
#include "String.h"
#include <unistd.h>
#include <errno.h>
}
#include <e32base.h>
#include <hal.h>

TInt tickPeriod;

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

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	// Directory is already set by platform: !:/private/e212a5c2
	return 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
	
}

static CC_NOINLINE cc_uint32 CalcMemSize(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 numBytes;
	if (!numElems || !elemsSize) return 1; /* treat 0 size as 1 byte */
	
	numBytes = numElems * elemsSize;
	if (numBytes / elemsSize != numElems) return 0; /* Overflow */
	return numBytes;
}

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? User::Alloc(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? User::AllocZ(size) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? User::ReAlloc(mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) User::Free(mem);
}

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

extern "C" void Symbian_Stopwatch_Init(void);

void Symbian_Stopwatch_Init(void) {
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

#define MACHINE_KEY "Symbian_Symbian_"

extern "C" cc_result Symbian_GetMachineID(cc_uint32* key);

cc_result Symbian_GetMachineID(cc_uint32* key) {
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
