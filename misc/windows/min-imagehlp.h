/* had problems getting imagehlp.h working on MSVC 4.0, so this is why it's here. */
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

typedef BOOL (CALLBACK *PENUMLOADED_MODULES_CALLBACK)(
  PCSTR ModuleName, ULONG ModuleBase, ULONG ModuleSize,
  PVOID UserContext
);
