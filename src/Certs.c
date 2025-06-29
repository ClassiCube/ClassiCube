#include "Certs.h"

#ifndef CC_CRT_BACKEND
#include "Errors.h"

void CertsBackend_Init(void) { }

void Certs_BeginChain(struct X509CertContext* ctx) { }
void Certs_FreeChain( struct X509CertContext* ctx) { }
int Certs_VerifyChain(struct X509CertContext* ctx) { return ERR_NOT_SUPPORTED; }

void Certs_BeginCert( struct X509CertContext* ctx, int size) { }
void Certs_AppendCert(struct X509CertContext* ctx, const void* data, int len) { }
void Certs_FinishCert(struct X509CertContext* ctx) { }
#else
#include "Platform.h"
#include "String.h"
#include "Stream.h"

void Certs_BeginCert( struct X509CertContext* ctx, int size) {
	void* data;
	ctx->cert = NULL;

	/* Should never happen, but never know */
	if (ctx->numCerts >= X509_MAX_CERTS) return;

	data = Mem_TryAllocCleared(1, size);
	if (!data) return;

	ctx->cert         = &ctx->certs[ctx->numCerts++];	
	ctx->cert->data   = data;
	ctx->cert->offset = 0;
}

void Certs_AppendCert(struct X509CertContext* ctx, const void* data, int len) {
	if (!ctx->cert) return;

	Mem_Copy((char*)ctx->cert->data + ctx->cert->offset, data, len);
	ctx->cert->offset += len;
}

void Certs_FinishCert(struct X509CertContext* ctx) {
}

void Certs_BeginChain(struct X509CertContext* ctx) {
	ctx->cert     = NULL;
	ctx->numCerts = 0;
}

void Certs_FreeChain( struct X509CertContext* ctx) {
	int i;
	for (i = 0; i < ctx->numCerts; i++)
	{
		Mem_Free(ctx->certs[i].data);
	}
	ctx->numCerts = 0;
}

#if CC_CRT_BACKEND == CC_CRT_BACKEND_OPENSSL
#include "Errors.h"
#include "Funcs.h"
/* === BEGIN OPENSSL HEADERS === */
typedef struct X509_           X509;
typedef struct X509_STORE_     X509_STORE;
typedef struct X509_STORE_CTX_ X509_STORE_CTX;
typedef struct OPENSSL_STACK_  OPENSSL_STACK;
typedef void (*OPENSSL_PopFunc)(void* data);

static OPENSSL_STACK* (*_OPENSSL_sk_new_null)(void);
int                   (*_OPENSSL_sk_push)(OPENSSL_STACK* st, const void* data);
void                  (*_OPENSSL_sk_pop_free)(OPENSSL_STACK* st, OPENSSL_PopFunc func);

static X509* (*_d2i_X509)(X509** px, const unsigned char** in, int len);

static X509* (*_X509_new)(void);
static void  (*_X509_free)(X509* a);

static X509_STORE* (*_X509_STORE_new)(void);
static int (*_X509_STORE_set_default_paths)(X509_STORE* ctx);

static int (*_X509_verify_cert)(X509_STORE_CTX* ctx);
static const char* (*_X509_verify_cert_error_string)(long n);

static X509_STORE_CTX* (*_X509_STORE_CTX_new)(void);
static void            (*_X509_STORE_CTX_free)(X509_STORE_CTX* ctx);

static int (*_X509_STORE_CTX_get_error)(X509_STORE_CTX* ctx);
static int (*_X509_STORE_CTX_init)(X509_STORE_CTX* ctx, X509_STORE* store,
                        X509* x509, OPENSSL_STACK* chain);
/* === END OPENSSL HEADERS === */

#if defined CC_BUILD_WIN
static const cc_string cryptoLib = String_FromConst("libcrypto.dll");
#elif defined CC_BUILD_HAIKU
static const cc_string cryptoLib = String_FromConst("libcrypto.so.3");
static const cc_string cryptoAlt = String_FromConst("libcrypto.so");
#else
static const cc_string cryptoLib = String_FromConst("libcrypto.so");
static const cc_string cryptoAlt = String_FromConst("libcrypto.so.3");
#endif

static X509_STORE* store;
static cc_bool ossl_loaded;

void CertsBackend_Init(void) {
	static const struct DynamicLibSym funcs[] = {		
		DynamicLib_ReqSym(OPENSSL_sk_new_null),
		DynamicLib_ReqSym(OPENSSL_sk_push),
		DynamicLib_ReqSym(OPENSSL_sk_pop_free),
		DynamicLib_ReqSym(d2i_X509),
		DynamicLib_ReqSym(X509_new),
		DynamicLib_ReqSym(X509_free),
		DynamicLib_ReqSym(X509_STORE_new),
		DynamicLib_ReqSym(X509_STORE_set_default_paths),
		DynamicLib_ReqSym(X509_STORE_CTX_new),
		DynamicLib_ReqSym(X509_STORE_CTX_free),
		DynamicLib_ReqSym(X509_STORE_CTX_get_error),
		DynamicLib_ReqSym(X509_STORE_CTX_init),
		DynamicLib_ReqSym(X509_verify_cert),
		DynamicLib_ReqSym(X509_verify_cert_error_string),
	};
	void* lib;

	ossl_loaded = DynamicLib_LoadAll(&cryptoLib,     funcs, Array_Elems(funcs), &lib);
	if (!lib) { 
		ossl_loaded = DynamicLib_LoadAll(&cryptoAlt, funcs, Array_Elems(funcs), &lib);
	}
}

static X509* ToOpenSSLCert(struct X509Cert* cert) {
	const unsigned char* data = cert->data;
	return _d2i_X509(NULL, &data, cert->offset);
}

int Certs_VerifyChain(struct X509CertContext* chain) {
	OPENSSL_STACK* inter;
	X509_STORE_CTX* ctx;
	int i, status, ret;
	X509* cur;
	X509* cert;
	if (!ossl_loaded) return ERR_NOT_SUPPORTED;

	/* Delay creating X509 store until necessary */
	if (!store) {
		store = _X509_STORE_new();
		if (!store) return ERR_OUT_OF_MEMORY;

		_X509_STORE_set_default_paths(store);
	}

	Platform_Log1("VERIFY CHAIN: %i", &chain->numCerts);
	if (!chain->numCerts) return ERR_INVALID_ARGUMENT;

	/* End/Leaf certificate */
	cert = ToOpenSSLCert(&chain->certs[0]);
	if (!cert) return ERR_OUT_OF_MEMORY;

	inter = _OPENSSL_sk_new_null();
	if (!inter) return ERR_OUT_OF_MEMORY;

	/* Intermediate certificates */
	for (i = 1; i < chain->numCerts; i++)
	{
		cur = ToOpenSSLCert(&chain->certs[i]);
		if (cur) _OPENSSL_sk_push(inter, cur);
	}

	ctx = _X509_STORE_CTX_new();
	_X509_STORE_CTX_init(ctx, store, cert, inter);

    status = _X509_verify_cert(ctx);
    if (status == 1) {
        Platform_LogConst("Certificate verified");
		ret = 0;
    } else {
		int err = _X509_STORE_CTX_get_error(ctx);
        Platform_LogConst(_X509_verify_cert_error_string(err));
		ret = -1;
    }

	_X509_STORE_CTX_free(ctx);
	_OPENSSL_sk_pop_free(inter, (OPENSSL_PopFunc)_X509_free);
	_X509_free(cert);

	return ret;
}
#endif

#endif

