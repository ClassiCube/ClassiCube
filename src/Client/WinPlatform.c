#include "Platform.h"
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

HANDLE heap;
void Platform_Init(void) {
	heap = GetProcessHeap(); // TODO: HeapCreate instead? probably not
}

void Platform_Free(void) {
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
	/* TODO: massively slow */
	for (UInt32 i = 0; i < numBytes; i++) {
		*dstByte++ = value;
	}
}

void Platform_MemCpy(void* dst, void* src, UInt32 numBytes) {
	UInt8* dstByte = (UInt8*)dst;
	UInt8* srcByte = (UInt8*)src;
	/* TODO: massively slow */
	for (UInt32 i = 0; i < numBytes; i++) {
		*dstByte++ = *srcByte++;
	}
}


void Platform_Log(String message) {
	/* TODO: log to console */
}

void Platform_NewUuid(UInt8* uuid) {
	GUID guid;
	CoCreateGuid(&guid);
	Platform_MemCpy(uuid, &guid, 16);
}

/* It's not worth making this an actual function, just use an inline macro. */
#define Platform_ReturnDateTime(sysTime)\
DateTime time;\
time.Year = sysTime.wYear;\
time.Month = sysTime.wMonth;\
time.Day = sysTime.wDay;\
time.Hour = sysTime.wHour;\
time.Minute = sysTime.wMinute;\
time.Second = sysTime.wSecond;\
time.Milliseconds = sysTime.wMilliseconds;\
return time;\

DateTime Platform_CurrentUTCTime(void) {
	SYSTEMTIME utcTime;
	GetSystemTime(&utcTime);
	Platform_ReturnDateTime(utcTime);
}

DateTime Platform_CurrentLocalTime(void) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	Platform_ReturnDateTime(localTime);
}