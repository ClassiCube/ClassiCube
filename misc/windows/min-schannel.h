typedef void* HCERTSTORE;

typedef struct _CERT_CONTEXT {
	DWORD      dwCertEncodingType;
	BYTE*      pbCertEncoded;
	DWORD      cbCertEncoded;
	CERT_INFO* pCertInfo;
	HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;


#define UNISP_NAME_A  "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W L"Microsoft Unified Security Protocol Provider"

#define SCH_CRED_NO_SYSTEM_MAPPER       0x00000002
#define SCH_CRED_NO_SERVERNAME_CHECK    0x00000004
#define SCH_CRED_MANUAL_CRED_VALIDATION 0x00000008
#define SCH_CRED_NO_DEFAULT_CREDS       0x00000010
#define SCH_CRED_AUTO_CRED_VALIDATION   0x00000020
#define SCH_CRED_USE_DEFAULT_CREDS      0x00000040
#define SCH_CRED_DISABLE_RECONNECTS     0x00000080

#define SCH_CRED_REVOCATION_CHECK_END_CERT           0x00000100
#define SCH_CRED_REVOCATION_CHECK_CHAIN              0x00000200
#define SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x00000400
#define SCH_CRED_IGNORE_NO_REVOCATION_CHECK          0x00000800
#define SCH_CRED_IGNORE_REVOCATION_OFFLINE           0x00001000
#define SCH_CRED_REVOCATION_CHECK_CACHE_ONLY         0x00004000

#define SP_PROT_SSL3_CLIENT   0x00000020
#define SP_PROT_TLS1_CLIENT   0x00000080
#define SP_PROT_TLS1_1_CLIENT 0x00000200
#define SP_PROT_TLS1_2_CLIENT 0x00000800


#define SCH_CRED_V1           0x00000001
#define SCH_CRED_V2           0x00000002
#define SCH_CRED_VERSION      0x00000002
#define SCH_CRED_V3           0x00000003
#define SCHANNEL_CRED_VERSION 0x00000004

struct _HMAPPER;
typedef struct _SCHANNEL_CRED {
	DWORD           dwVersion;
	DWORD           cCreds;
	PCCERT_CONTEXT* paCred;
	HCERTSTORE      hRootStore;
	DWORD           cMappers;
	struct _HMAPPER **aphMappers;
	DWORD           cSupportedAlgs;
	ALG_ID*         palgSupportedAlgs;
	DWORD           grbitEnabledProtocols;
	DWORD           dwMinimumCipherStrength;
	DWORD           dwMaximumCipherStrength;
	DWORD           dwSessionLifespan;
	DWORD           dwFlags;
	DWORD           dwCredFormat;
} SCHANNEL_CRED;