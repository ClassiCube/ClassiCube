#include "IO.h"
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

bool IO_FileExists(STRING_TRANSIENT String* path) {
	UInt32 attribs = GetFileAttributesA(path->buffer);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

bool IO_DirectoryExists(STRING_TRANSIENT String* path) {
	UInt32 attribs = GetFileAttributesA(path->buffer);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode IO_DirectoryCreate(STRING_TRANSIENT String* path) {
	BOOL success = CreateDirectoryA(path->buffer, NULL);
	return success ? 0 : GetLastError();
}


ReturnCode IO_FileOpen(void** file, STRING_TRANSIENT String* path, bool readOnly) {
	UINT32 access = GENERIC_READ;
	if (!readOnly) access |= GENERIC_WRITE;
	HANDLE handle = CreateFileA(path->buffer, access, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode IO_FileCreate(void** file, STRING_TRANSIENT String* path) {
	UINT32 access = GENERIC_READ | GENERIC_WRITE;
	HANDLE handle = CreateFileA(path->buffer, access, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	*file = (void*)handle;

	return handle != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode IO_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead) {
	BOOL success = ReadFile((HANDLE)file, buffer, count, bytesRead, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode IO_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten) {
	BOOL success = WriteFile((HANDLE)file, buffer, count, bytesWritten, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode IO_FileClose(void* file) {
	BOOL success = CloseHandle((HANDLE)file);
	return success ? 0 : GetLastError();
}