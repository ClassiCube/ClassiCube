#include "Platform.h"
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

HANDLE heap;
void Platform_Init() {
	heap = GetProcessHeap(); // TODO: HeapCreate instead? probably not
}

void Platform_Free() {
	HeapDestroy(heap);
}

void* Platform_MemAlloc(UInt32 numBytes) {
	return HeapAlloc(heap, 0, numBytes);
}

void Platform_MemFree(void* mem) {
	HeapFree(heap, 0, mem);
}

void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes) {
	UInt8* dstByte = (UInt8*)dst;
	// TODO: massively slow
	for (UInt32 i = 0; i < numBytes; i++) {
		*dstByte++ = value;
	}
}

void Platform_MemCpy(void* dst, void* src, UInt32 numBytes) {
	UInt8* dstByte = (UInt8*)dst;
	UInt8* srcByte = (UInt8*)src;
	// TODO: massively slow
	for (UInt32 i = 0; i < numBytes; i++) {
		*dstByte++ = *srcByte++;
	}
}