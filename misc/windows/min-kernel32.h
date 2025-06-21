#ifndef CC_KERN32_FUNC
#define CC_KERN32_FUNC
#endif

CC_KERN32_FUNC void (WINAPI *_GetSystemTimeAsFileTime)(LPFILETIME sysTime);
/* Fallback support for NT 3.5 */
static void WINAPI Fallback_GetSystemTimeAsFileTime(LPFILETIME sysTime) {
	SYSTEMTIME curTime;
	GetSystemTime(&curTime);
	SystemTimeToFileTime(&curTime, sysTime);
}

CC_KERN32_FUNC BOOL (WINAPI *_AttachConsole)(DWORD processId);
CC_KERN32_FUNC BOOL (WINAPI *_IsDebuggerPresent)(void);
CC_KERN32_FUNC void (NTAPI *_RtlCaptureContext)(CONTEXT* ContextRecord);


static void Kernel32_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_OptSym(AttachConsole), 
		DynamicLib_OptSym(IsDebuggerPresent),
		DynamicLib_OptSym(GetSystemTimeAsFileTime),
		DynamicLib_OptSym(RtlCaptureContext)
	};

	static const cc_string kernel32 = String_FromConst("KERNEL32.DLL");
	void* lib;
	DynamicLib_LoadAll(&kernel32, funcs, Array_Elems(funcs), &lib);
	/* Not present on Windows NT 3.5 */
	if (!_GetSystemTimeAsFileTime) _GetSystemTimeAsFileTime = Fallback_GetSystemTimeAsFileTime;
}