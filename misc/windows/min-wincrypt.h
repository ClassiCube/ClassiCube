typedef struct _CRYPTOAPI_BLOB {
	DWORD cbData;
	BYTE* pbData;
} DATA_BLOB;

static BOOL (WINAPI *_CryptProtectData  )(DATA_BLOB* dataIn, PCWSTR dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);
static BOOL (WINAPI *_CryptUnprotectData)(DATA_BLOB* dataIn, PWSTR* dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);