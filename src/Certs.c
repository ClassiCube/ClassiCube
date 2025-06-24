#include "Certs.h"
#include "Platform.h"
#include "String.h"
#include "Stream.h"
#include <openssl/x509.h>
static X509_STORE* store;

void CertsBackend_Init(void) {
	Platform_LogConst("BKEND");

	store = X509_STORE_new();
	X509_STORE_set_default_paths(store);
}

void Certs_BeginChain(struct X509CertContext* ctx) {
	Platform_LogConst("CHAIN");
	ctx->chain = NULL;
	ctx->cert  = NULL;
}

void Certs_FreeChain( struct X509CertContext* ctx) {
}

int Certs_VerifyChain(struct X509CertContext* ctx) {
	return 0;
}


void Certs_BeginCert( struct X509CertContext* ctx, int size) {
	ctx->cert   = Mem_TryAllocCleared(1, size);
	ctx->offset = 0;
}

void Certs_AppendCert(struct X509CertContext* ctx, const void* data, int len) {
	if (!ctx->cert) return;

	Mem_Copy((char*)ctx->cert + ctx->offset, data, len);
	ctx->offset += len;
}

void Certs_FinishCert(struct X509CertContext* ctx) {

	Platform_LogConst("CERT"); static int counter;

	char buffer[128];
	cc_string buf = String_FromArray(buffer);
	String_Format1(&buf, "cert_%i.der", &counter); counter++;

	//Stream_WriteAllTo(&buf, ctx->cert, ctx->offset);

	const unsigned char* data = ctx->cert;
	X509* cert = d2i_X509(NULL, &data, ctx->offset);

	Mem_Free(ctx->cert);
	ctx->cert = NULL;
}
