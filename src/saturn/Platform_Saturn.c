#define CC_XTEA_ENCRYPTION
#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING
#define CC_NO_OPEN
#define CC_NO_FILESYSTEM
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
#include <stdio.h>
#include <yaul.h>
#include <mm/tlsf.h>

//#define OVERRIDE_MEM_FUNCTIONS
void* calloc(size_t num, size_t size) {
	void* ptr = malloc(num * size);
	if (ptr) memset(ptr, 0, num * size);
	return ptr;
}

const char* Platform_AppNameSuffix  = " Saturn";
cc_bool Platform_ReadonlyFilesystem = true;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

int main(void) {
	SetupProgram(0, NULL);
	while (Window_Main.Exists) { 
		RunProgram(0, NULL);
	}
	
	Window_Free();
	return 0;
}


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
static tlsf_t lwram_mem; // Use LWRAM for 1 MB of memory

static void InitMemory(void) {
	lwram_mem = tlsf_pool_create(LWRAM(0), LWRAM_SIZE);
}

/*void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? tlsf_malloc(&lwram_mem, size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	void* ptr = Mem_TryAlloc(numElems, elemsSize);
	if (ptr) memset(ptr, 0, numElems * elemsSize);
	return ptr;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? tlsf_realloc(&lwram_mem, mem, size) : NULL;
}

void Mem_Free(void* mem) {
	tlsf_free(&lwram_mem, mem);
}*/


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
#define WRITE_ADDRESS CS0(0x00100001)
// in Medafen, patch DummyWrite in src/ss/Cart.cpp to instead log the value

void Platform_Log(const char* msg, int len) {
	for (int i = 0; i < len; i++) {
		MEMORY_WRITE(8, WRITE_ADDRESS, msg[i]);
	}
	MEMORY_WRITE(8, WRITE_ADDRESS, '\n');
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
#include "sh2_wdt.h"

static void Stopwatch_Init(void) {
	wdt_stop();

	wdt_set_irq_number(CPU_INTC_INTERRUPT_WDT_ITI);
	wdt_set_irq_priority(15);
    cpu_intc_ihr_set(CPU_INTC_INTERRUPT_WDT_ITI, wdt_handler);

	wdt_enable();
}

cc_uint64 Stopwatch_Measure(void) {
	return wdt_total_ticks();
}

#define US_PER_SEC     1000000
#define NTSC_320_CLOCK 26846587

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	cc_uint64 delta = end - beg;

	// TODO still wrong?? PAL detection ???
	return (delta * US_PER_SEC) / (NTSC_320_CLOCK / 1024);
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) {
	// TODO sleep a bit
	cc_uint32 cycles = 26846 * milliseconds;
	
	for (cc_uint32 i = 0; i < cycles; i++)
	{
		__asm__ volatile ("nop;");
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	Platform_LogConst("START CLASSICUBE");
	return SetGameArgs(args, numArgs);
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}

cc_result Platform_SetDefaultCurrentDirectory(void) { return 0; }

extern void *__end;
void Platform_Init(void) {
	cc_uint32 avail = HWRAM(HWRAM_SIZE) - (uint32_t)&__end;
	Platform_Log1("Free HWRAM: %i bytes", &avail);

	InitMemory();
	Stopwatch_Init();
}

void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return false;
}

void Process_Exit(cc_result code) { exit(code); }


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#define MACHINE_KEY "SaturnSaturnSEGA"

static cc_result GetMachineID(cc_uint32* key) {
	Mem_Copy(key, MACHINE_KEY, sizeof(MACHINE_KEY) - 1);
	return 0;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
