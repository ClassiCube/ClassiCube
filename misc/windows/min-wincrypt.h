#ifndef CC_CRYPT32_FUNC
#define CC_CRYPT32_FUNC
#endif

typedef void* HCERTSTORE;
typedef void* PCCERT_CONTEXT;

#define szOID_PKIX_KP_SERVER_AUTH "1.3.6.1.5.5.7.3.1"
#define szOID_SERVER_GATED_CRYPTO "1.3.6.1.4.1.311.10.3.3"
#define szOID_SGC_NETSCAPE        "2.16.840.1.113730.4.1"

#define CERT_STORE_PROV_MEMORY ((LPCSTR)2)
#define CERT_STORE_ADD_ALWAYS  4
#define X509_ASN_ENCODING      0x1

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

typedef struct _CERT_TRUST_STATUS {
	DWORD dwErrorStatus;
	DWORD dwInfoStatus;
} CERT_TRUST_STATUS, *PCERT_TRUST_STATUS;

typedef struct _CERT_CHAIN_CONTEXT {
	DWORD                cbSize;
	CERT_TRUST_STATUS    TrustStatus;
	DWORD                cChain;
	void*                rgpChain;
	DWORD                cLowerQualityChainContext;
	void*                rgpLowerQualityChainContext;
	BOOL                 fHasRevocationFreshnessTime;
	DWORD                dwRevocationFreshnessTime;
	DWORD                dwCreateFlags;
	GUID                 ChainId;
} CERT_CHAIN_CONTEXT, *PCERT_CHAIN_CONTEXT;
typedef const CERT_CHAIN_CONTEXT* PCCERT_CHAIN_CONTEXT;


CC_CRYPT32_FUNC BOOL (WINAPI *_CryptProtectData  )(DATA_BLOB* dataIn, PCWSTR dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);
CC_CRYPT32_FUNC BOOL (WINAPI *_CryptUnprotectData)(DATA_BLOB* dataIn, PWSTR* dataDescr, PVOID entropy, PVOID reserved, PVOID promptStruct, DWORD flags, DATA_BLOB* dataOut);

CC_CRYPT32_FUNC HCERTSTORE (WINAPI *_CertOpenStore)(LPCSTR storeProvider, DWORD encodingType, void* cryptProv, DWORD flags, const void* para);
CC_CRYPT32_FUNC BOOL       (WINAPI *_CertCloseStore)(HCERTSTORE certStore, DWORD flags);
CC_CRYPT32_FUNC BOOL       (WINAPI *_CertAddEncodedCertificateToStore)(HCERTSTORE certStore, DWORD certEncodingType, const BYTE* certEncoded, DWORD lenEncoded, DWORD addDisposition, PCCERT_CONTEXT* certContext);

CC_CRYPT32_FUNC BOOL (WINAPI *_CertGetCertificateChain)(PVOID chainEngine, PCCERT_CONTEXT certContext, LPFILETIME time, HCERTSTORE additionalStore, CERT_CHAIN_PARA* chainPara, DWORD flags, PVOID reserved, PCCERT_CHAIN_CONTEXT* chainContext);
CC_CRYPT32_FUNC void (WINAPI *_CertFreeCertificateChain)(PCCERT_CHAIN_CONTEXT chainContext);

CC_CRYPT32_FUNC BOOL (WINAPI *_CertFreeCertificateContext)(PCCERT_CONTEXT certContext);

static void Crypt32_LoadDynamicFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_OptSym(CryptProtectData),        DynamicLib_OptSym(CryptUnprotectData),
		DynamicLib_ReqSym(CertGetCertificateChain), DynamicLib_ReqSym(CertFreeCertificateChain),
		DynamicLib_ReqSym(CertOpenStore),           DynamicLib_ReqSym(CertCloseStore),
		DynamicLib_ReqSym(CertAddEncodedCertificateToStore),
		DynamicLib_ReqSym(CertFreeCertificateContext),
	};

	static const cc_string crypt32 = String_FromConst("CRYPT32.DLL");
	static void* crypt32_lib;
	
	if (crypt32_lib) return;
	DynamicLib_LoadAll(&crypt32, funcs, Array_Elems(funcs), &crypt32_lib);
}
