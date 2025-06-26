#include "Certs.h"

#if CC_CTX_BACKEND == CC_CRT_BACKEND_NONE
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
#include <openssl/x509.h>
#include "Errors.h"
static X509_STORE* store;

void CertsBackend_Init(void) {
}

static X509* ToOpenSSLCert(struct X509Cert* cert) {
	const unsigned char* data = cert->data;
	return d2i_X509(NULL, &data, cert->offset);
}

int Certs_VerifyChain(struct X509CertContext* chain) {
	STACK_OF(X509)* inter;
	X509_STORE_CTX* ctx;
	X509* cur;
	X509* cert;
	int i;

	/* Delay creating X509 store until necessary */
	if (!store) {
		store = X509_STORE_new();
		if (!store) return;

		X509_STORE_set_default_paths(store);
	}

	Platform_Log1("VERIFY CHAIN: %i", &chain->numCerts);
	if (!chain->numCerts) return ERR_NOT_SUPPORTED;

	/* End/Leaf certificate */
	cert = ToOpenSSLCert(&chain->certs[0]);
	if (!cert) return ERR_OUT_OF_MEMORY;

	inter = sk_X509_new_null();
	if (!inter) return ERR_OUT_OF_MEMORY;

	/* Intermediate certificates */
	for (i = 1; i < chain->numCerts; i++)
	{
		cur = ToOpenSSLCert(&chain->certs[i]);
		if (cur) sk_X509_push(inter, cur);
	}

	ctx = X509_STORE_CTX_new();
	X509_STORE_CTX_init(ctx, store, cert, inter);

    int status = X509_verify_cert(ctx);
    if (status == 1) {
        Platform_LogConst("Certificate verified");
    } else {
		int err = X509_STORE_CTX_get_error(ctx);
        Platform_LogConst(X509_verify_cert_error_string(err));
    }

	X509_STORE_CTX_free(ctx);
	sk_X509_pop_free(inter, X509_free);
	X509_free(cert);

	return 0;
}
#endif

#endif

