#ifndef CC_CRYPT32_FUNC
#define CC_CRYPT32_FUNC
#endif

typedef struct _CRYPTOAPI_BLOB {
	DWORD cbData;
	BYTE* pbData;
} DATA_BLOB;

CC_CRYPT32_FUNC BOOL (WINAPI *_CryptProtectData  )(DATA_BLOB* dataIn, PCWSTR dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);
CC_CRYPT32_FUNC BOOL (WINAPI *_CryptUnprotectData)(DATA_BLOB* dataIn, PWSTR* dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);

static void Crypt32_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_OptSym(CryptProtectData), DynamicLib_OptSym(CryptUnprotectData)
	};

	static const cc_string crypt32 = String_FromConst("CRYPT32.DLL");
	static void* crypt32_lib;
	
	if (crypt32_lib) return;
	DynamicLib_LoadAll(&crypt32, funcs, Array_Elems(funcs), &crypt32_lib);
}