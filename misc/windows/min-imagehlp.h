typedef struct _IMAGEHLP_SYMBOL {
	DWORD SizeOfStruct;
	DWORD Address;
	DWORD Size;
	DWORD Flags;
	DWORD MaxNameLength;
	CHAR Name[1];
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef enum {
	SymNone,
	SymCoff,
	SymCv,
	SymPdb,
	SymExport,
	SymDeferred,
	SymSym,
	SymDia,
	SymVirtual,
	NumSymTypes
} SYM_TYPE;

typedef struct _IMAGEHLP_MODULE {
  DWORD SizeOfStruct;
	DWORD BaseOfImage;
	DWORD ImageSize;
	DWORD TimeDateStamp;
	DWORD CheckSum;
	DWORD NumSyms;
	SYM_TYPE SymType;
	CHAR ModuleName[32];
	CHAR ImageName[256];
	CHAR LoadedImageName[256];
} IMAGEHLP_MODULE, *PIMAGEHLP_MODULE;

typedef struct _IMAGEHLP_LINE {
	DWORD SizeOfStruct;
	PVOID Key;
	DWORD LineNumber;
	PCHAR FileName;
	DWORD Address;
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef enum {
	AddrMode1616,
	AddrMode1632,
	AddrModeReal,
	AddrModeFlat
} ADDRESS_MODE;

typedef struct _ADDRESS {
	DWORD Offset;
	WORD Segment;
	ADDRESS_MODE Mode;
} ADDRESS, *LPADDRESS;

typedef struct _KDHELP {
	DWORD Thread;
	DWORD ThCallbackStack;
	DWORD NextCallback;
	DWORD FramePointer;
	DWORD KiCallUserMode;
	DWORD KeUserCallbackDispatcher;
	DWORD SystemRangeStart;
} KDHELP, *PKDHELP;

typedef struct _STACKFRAME {
	ADDRESS AddrPC;
	ADDRESS AddrReturn;
	ADDRESS AddrFrame;
	ADDRESS AddrStack;
	PVOID FuncTableEntry;
	DWORD Params[4];
	BOOL Far;
	BOOL Virtual;
	DWORD Reserved[4];
	KDHELP kdHelp;
} STACKFRAME, *LPSTACKFRAME;

typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(PCSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize, PVOID UserContext);
typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE hProcess, DWORD lpBaseAddress, PVOID lpBuffer, DWORD nSize, PDWORD lpNumberOfBytesRead);
typedef PVOID (CALLBACK *PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE, DWORD);
typedef DWORD (CALLBACK *PGET_MODULE_BASE_ROUTINE)(HANDLE, DWORD);
typedef BOOL (CALLBACK *PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE hProcess, DWORD lpBaseAddress, PVOID lpBuffer, DWORD nSize, PDWORD lpNumberOfBytesRead);
typedef DWORD (CALLBACK *PTRANSLATE_ADDRESS_ROUTINE)(HANDLE, HANDLE, LPADDRESS);

static DWORD (WINAPI *_SymGetModuleBase)(HANDLE process, DWORD addr);
static PVOID     (WINAPI *_SymFunctionTableAccess)(HANDLE process, DWORD addr);
static BOOL      (WINAPI *_SymInitialize)(HANDLE process, PCSTR userSearchPath, BOOL fInvadeProcess);

static BOOL      (WINAPI *_SymGetSymFromAddr)(HANDLE process, DWORD addr, DWORD* displacement, IMAGEHLP_SYMBOL* sym);
static BOOL      (WINAPI *_SymGetModuleInfo) (HANDLE process, DWORD addr, IMAGEHLP_MODULE* module);
static BOOL      (WINAPI *_SymGetLineFromAddr)(HANDLE hProcess, DWORD addr, DWORD* displacement, IMAGEHLP_LINE* line); /* displacement is intentionally DWORD */

static BOOL      (WINAPI *_EnumerateLoadedModules)(HANDLE process, PENUMLOADED_MODULES_CALLBACK callback, PVOID userContext);

static BOOL      (WINAPI *_StackWalk)(DWORD machineType, HANDLE process, HANDLE thread, STACKFRAME* stackFrame, PVOID contextRecord,
									PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
									PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);

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
