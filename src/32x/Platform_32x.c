#define OVERRIDE_MEM_FUNCTIONS
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING
#define CC_NO_FILESYSTEM
#define CC_NO_ENCRYPTION
#define CC_NO_OPEN
#define CC_NO_CRASHHANDLER

#include "../Stream.h"
#include "../ExtMath.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../Options.h"
#include "../PackedCol.h"

#include <stdlib.h>
#include <string.h>

#include "../../misc/32x/32x.h"
#include "../../misc/32x/hw_32x.h"
#include "../../third_party/tinyalloc/tinyalloc.c"

const char* Platform_AppNameSuffix  = " 32x";
cc_bool Platform_ReadonlyFilesystem = true;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

int main(int argc, char** argv) {
	SetupProgram(argc, argv);
	while (Window_Main.Exists) {
		RunGame();
	}
	
	Window_Free();
	return 0;
}


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	void* ptr = size ? ta_alloc(size) : NULL;
	Platform_Log2("MALLOCED: %x (%i bytes)", &ptr, &size);
    return ptr;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	void* ptr = size ? ta_alloc(size) : NULL;
	Platform_Log2("CALLOCED: %x (%i bytes)", &ptr, &size);

	if (ptr) Mem_Set(ptr, 0, size);
    return ptr;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	return NULL; // TODO
}

void Mem_Free(void* mem) {
	if (mem) ta_free(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
static int logY;
void Platform_Log(const char* msg, int len) {
	for (int i = 0; i < 20; i++)
		HwMdPutc(' ',    0x0000, 1 + i, logY);

	for (int i = 0; i < min(len, 64); i++)
		HwMdPutc(msg[i], 0x0000, 1 + i, logY);

	logY = (logY + 1) % 26;
}

TimeMS DateTime_CurrentUTC(void) {
	return 0;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	Mem_Set(t, 0, sizeof(struct cc_datetime));
}


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#include "../saturn/sh2_wdt.h"

static void Stopwatch_Init(void) {
	wdt_stop();

	wdt_set_irq_number(5); // hardcoded in sh2_crt0.s
	wdt_set_irq_priority(15);

	wdt_enable();
}

cc_uint64 Stopwatch_Measure(void) {
	return wdt_total_ticks();
}

#define US_PER_SEC      1000000
#define NTSC_CPU_CLOCK 23011360 // TODO

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	cc_uint64 delta = end - beg;

	// TODO still completely wrong?? PAL detection ???
	return (delta * US_PER_SEC) / (NTSC_CPU_CLOCK / 1024);
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) {
	// TODO sleep a bit
	Hw32xDelay(1);
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
extern void _bss_end;
#define SDRAM_END 0x0603FFFF

void Platform_Init(void) {
	// TODO
	void* heap_beg = &_bss_end;
	void* heap_end = (void*)(SDRAM_END - 16384); // leave 16 KB for stack
	ta_init(heap_beg, heap_end, 256, 16, 4);

	int size = (int)(heap_end - heap_beg);
	Platform_Log3("HEAP SIZE: %i bytes (%x -> %x)", &size, &heap_beg, &heap_end);

	Stopwatch_Init();
}

void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return false;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return 0;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return 0;
}

cc_result Platform_SetDefaultCurrentDirectory(void) { return 0; }

void Process_Exit(cc_result code) { _exit(code); }

