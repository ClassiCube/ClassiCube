#ifndef CC_IMAGEHLP_FUNC
#define CC_IMAGEHLP_FUNC
#endif

/*Not available on older SDKs */
typedef cc_uintptr _DWORD_PTR;

typedef PVOID      (WINAPI *_PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE process, _DWORD_PTR addrBase);
typedef _DWORD_PTR (WINAPI *_PGET_MODULE_BASE_ROUTINE)(HANDLE process,       _DWORD_PTR address);
typedef BOOL       (WINAPI *_PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE process,   _DWORD_PTR baseAddress, PVOID buffer, DWORD size, DWORD* numRead);

CC_IMAGEHLP_FUNC _DWORD_PTR (WINAPI *_SymGetModuleBase)(HANDLE process, _DWORD_PTR addr);
CC_IMAGEHLP_FUNC PVOID      (WINAPI *_SymFunctionTableAccess)(HANDLE process, _DWORD_PTR addr);
CC_IMAGEHLP_FUNC BOOL       (WINAPI *_SymInitialize)(HANDLE process, PCSTR userSearchPath, BOOL fInvadeProcess);

CC_IMAGEHLP_FUNC BOOL       (WINAPI *_SymGetSymFromAddr)(HANDLE process, _DWORD_PTR addr, _DWORD_PTR* displacement, IMAGEHLP_SYMBOL* sym);
CC_IMAGEHLP_FUNC BOOL       (WINAPI *_SymGetModuleInfo) (HANDLE process, _DWORD_PTR addr, IMAGEHLP_MODULE* module);
CC_IMAGEHLP_FUNC BOOL       (WINAPI *_SymGetLineFromAddr)(HANDLE hProcess, _DWORD_PTR addr, DWORD* displacement, IMAGEHLP_LINE* line); /* displacement is intentionally DWORD */

CC_IMAGEHLP_FUNC BOOL      (WINAPI *_EnumerateLoadedModules)(HANDLE process, PENUMLOADED_MODULES_CALLBACK callback, PVOID userContext);

CC_IMAGEHLP_FUNC BOOL      (WINAPI *_StackWalk)(DWORD machineType, HANDLE process, HANDLE thread, STACKFRAME* stackFrame, PVOID contextRecord,
									_PREAD_PROCESS_MEMORY_ROUTINE readMemory, _PFUNCTION_TABLE_ACCESS_ROUTINE functionTableAccess,
									_PGET_MODULE_BASE_ROUTINE getModuleBase, PTRANSLATE_ADDRESS_ROUTINE translateAddress);

static void ImageHlp_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
	#ifdef _IMAGEHLP64
		{ "EnumerateLoadedModules64", (void**)&_EnumerateLoadedModules},
		{ "SymFunctionTableAccess64", (void**)&_SymFunctionTableAccess},
		{ "SymInitialize",            (void**)&_SymInitialize },
		{ "SymGetModuleBase64",       (void**)&_SymGetModuleBase },
		{ "SymGetModuleInfo64",       (void**)&_SymGetModuleInfo },
		{ "SymGetLineFromAddr64",     (void**)&_SymGetLineFromAddr },
		{ "SymGetSymFromAddr64",      (void**)&_SymGetSymFromAddr },
		{ "StackWalk64",              (void**)&_StackWalk },
	#else
		{ "EnumerateLoadedModules",   (void**)&_EnumerateLoadedModules },
		{ "SymFunctionTableAccess",   (void**)&_SymFunctionTableAccess },
		{ "SymInitialize",            (void**)&_SymInitialize },
		{ "SymGetModuleBase",         (void**)&_SymGetModuleBase },
		{ "SymGetModuleInfo",         (void**)&_SymGetModuleInfo },
		{ "SymGetLineFromAddr",       (void**)&_SymGetLineFromAddr },
		{ "SymGetSymFromAddr",        (void**)&_SymGetSymFromAddr },
		{ "StackWalk",                (void**)&_StackWalk },
	#endif
	};

	static const cc_string imagehlp = String_FromConst("IMAGEHLP.DLL");
	void* lib;
	DynamicLib_LoadAll(&imagehlp, funcs, Array_Elems(funcs), &lib);
}
