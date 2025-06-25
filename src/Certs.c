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
	//char buffer[128];
	//cc_string buf = String_FromArray(buffer);
	//String_Format1(&buf, "cert_%i.der", &ctx->numCerts);
	//Stream_WriteAllTo(&buf, ctx->cert->data, ctx->cert->offset);
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

#if CC_CRT_BACKEND_OPENSSL
#include <openssl/x509.h>
static X509_STORE* store;

void CertsBackend_Init(void) {
	Platform_LogConst("BKEND");

	store = X509_STORE_new();
	X509_STORE_set_default_paths(store);
}

int Certs_VerifyChain(struct X509CertContext* ctx) {
	

	//const unsigned char* data = ctx->cert->data;
	//X509* cert = d2i_X509(NULL, &data, ctx->cert->offset);
	return 0;
}
#endif

#endif

