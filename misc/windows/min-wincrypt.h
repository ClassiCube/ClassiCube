#ifndef CC_CRYPT32_FUNC
#define CC_CRYPT32_FUNC
#endif

typedef struct _CRYPTOAPI_BLOB {
	DWORD cbData;
	BYTE* pbData;
} DATA_BLOB;

typedef struct _CTL_USAGE {
	DWORD cUsageIdentifier;
	LPSTR *rgpszUsageIdentifier;
} CERT_ENHKEY_USAGE;

#define USAGE_MATCH_TYPE_AND 0x0
#define USAGE_MATCH_TYPE_OR 0x1

typedef struct _CERT_USAGE_MATCH {
	DWORD dwType;
	CERT_ENHKEY_USAGE Usage;
} CERT_USAGE_MATCH;

typedef struct _CERT_CHAIN_PARA {
    DWORD cbSize;
    CERT_USAGE_MATCH RequestedUsage;
} CERT_CHAIN_PARA;


CC_CRYPT32_FUNC BOOL (WINAPI *_CryptProtectData  )(DATA_BLOB* dataIn, PCWSTR dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);
CC_CRYPT32_FUNC BOOL (WINAPI *_CryptUnprotectData)(DATA_BLOB* dataIn, PWSTR* dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);

/*CC_CRYPT32_FUNC BOOL (WINAPI *_CertGetCertificateChain)(PVOID chainEngine, PCCERT_CONTEXT certContext, LPFILETIME time, HCERTSTORE additionalStore, CERT_CHAIN_PARA* chainPara, DWORD flags, PVOID reserved, PCCERT_CHAIN_CONTEXT* chainContext);

CC_CRYPT32_FUNC void (WINAPI *_CertFreeCertificateChain)(PCCERT_CHAIN_CONTEXT chainContext);*/

static void Crypt32_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_OptSym(CryptProtectData), DynamicLib_OptSym(CryptUnprotectData),
		//DynamicLib_OptSym(CertGetCertificateChain), DynamicLib_OptSym(CertFreeCertificateChain), 
	};

	static const cc_string crypt32 = String_FromConst("CRYPT32.DLL");
	static void* crypt32_lib;
	
	if (crypt32_lib) return;
	DynamicLib_LoadAll(&crypt32, funcs, Array_Elems(funcs), &crypt32_lib);
}
