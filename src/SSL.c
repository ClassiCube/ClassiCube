#include "SSL.h"
#include "Errors.h"

#if CC_SSL_BACKEND == CC_SSL_BACKEND_BEARSSL
#include "String_.h"
#include "Certs.h"
#include "../third_party/bearssl/bearssl.h"
#include "../misc/certs/certs.h"

// https://github.com/unkaktus/bearssl/blob/master/samples/client_basic.c#L283
#define SSL_ERROR_SHIFT 0xB5510000

typedef struct SSLContext {
	br_x509_minimal_context xc;
	br_ssl_client_context sc;
	struct X509CertContext x509;
	unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
	br_sslio_context ioc;
	cc_result readError, writeError;
	cc_socket socket;
} SSLContext;
static cc_bool _verifyCerts;

static void x509_start_cert(const br_x509_class** ctx, uint32_t length) {
	struct SSLContext* ssl = (struct SSLContext*)ctx;

	br_x509_minimal_vtable.start_cert(ctx, length);
	Certs_BeginCert(&ssl->x509, length);
}

static void x509_append(const br_x509_class** ctx, const unsigned char* buf, size_t len) {
	struct SSLContext* ssl = (struct SSLContext*)ctx;

	br_x509_minimal_vtable.append(ctx, buf, len);
	Certs_AppendCert(&ssl->x509, buf, len);
}

static void x509_end_cert(const br_x509_class** ctx) {
	struct SSLContext* ssl = (struct SSLContext*)ctx;

	br_x509_minimal_vtable.end_cert(ctx);
	Certs_FinishCert(&ssl->x509);
}

static void x509_start_chain(const br_x509_class** ctx, const char* server_name) {
	struct SSLContext* ssl = (struct SSLContext*)ctx;

	br_x509_minimal_vtable.start_chain(ctx, server_name);
	Certs_BeginChain(&ssl->x509);
}

static unsigned x509_maybe_skip_verify(unsigned r) {
	/* User selected to not care about certificate authenticity */
	if (r == BR_ERR_X509_NOT_TRUSTED && !_verifyCerts) return 0;

	return r;
}

static unsigned x509_maybe_system_verify(struct SSLContext* ssl, unsigned r) {
	/* Only fallback to system verification for potentially untrusted certificate case */
	/* This ensures consistent validation behaviour in other cases between all platforms */
	if (r != BR_ERR_X509_NOT_TRUSTED) return r;
	if (!ssl->x509.numCerts)          return r;

	if (Certs_VerifyChain(&ssl->x509) == 0) r = 0;
	return r;
}

static unsigned x509_end_chain(const br_x509_class** ctx) {
	struct SSLContext* ssl = (struct SSLContext*)ctx;
	
	unsigned r = br_x509_minimal_vtable.end_chain(ctx);
	r = x509_maybe_skip_verify(r);
	r = x509_maybe_system_verify(ssl, r);

	Certs_FreeChain(&ssl->x509);
	return r;
}

static const br_x509_pkey* x509_get_pkey(const br_x509_class*const* ctx, unsigned* usages) {
	return br_x509_minimal_vtable.get_pkey(ctx, usages);
}

static const br_x509_class cert_verifier_vtable = {
	sizeof(br_x509_minimal_context),
	x509_start_chain,
	x509_start_cert,
	x509_append,
	x509_end_cert,
	x509_end_chain,
	x509_get_pkey
};

void SSLBackend_Init(cc_bool verifyCerts) {
	_verifyCerts = verifyCerts;
	CertsBackend_Init();
}

cc_bool SSLBackend_DescribeError(cc_result res, cc_string* dst) {
	switch (res) {
	case SSL_ERROR_SHIFT | BR_ERR_X509_EXPIRED:
		String_AppendConst(dst, "The website's SSL certificate is expired or not yet valid");
		return true;
	case SSL_ERROR_SHIFT | BR_ERR_X509_NOT_TRUSTED:
		String_AppendConst(dst, "The website's SSL certificate was issued by an authority that is not trusted");
		return true;
	}
	return false; // TODO: error codes 
}

static void InjectEntropy(SSLContext* ctx) {
	char buf[32];
	cc_result res = Platform_GetEntropy(buf, 32);
	if (res) Platform_LogConst("SSL: Using insecure uninitialised stack data for entropy");

	br_ssl_engine_inject_entropy(&ctx->sc.eng, buf, 32);
}

static int ssl_time_check_callback(void* ctx,
	uint32_t not_before_days, uint32_t not_before_secs,
	uint32_t not_after_days,  uint32_t not_after_secs) 
{
#ifdef CC_BUILD_CONSOLE
	/* It's fairly common for RTC on older console systems to not be set correctly */
	return 0;
#else
	cc_uint64 cur = DateTime_CurrentUTC();
	uint32_t days = (uint32_t)(cur / 86400) + 366;
	uint32_t secs = (uint32_t)(cur % 86400);

	/* This matches bearssl's default time calculation:
		time_t x = time(NULL);
		vd = (uint32_t)(x / 86400) + 719528;
		vs = (uint32_t)(x % 86400);
	*/

	if (days < not_before_days || (days == not_before_days && secs < not_before_secs)) return -1;
	if (days > not_after_days  || (days == not_after_days  && secs > not_after_secs))  return  1;

	return 0;
#endif
}

static int sock_read(void* ctx_, unsigned char* buf, size_t len) {
	SSLContext* ctx = (SSLContext*)ctx_;
	cc_uint32 read;
	cc_result res = Socket_Read(ctx->socket, buf, len, &read);
	
	if (res) { ctx->readError = res; return -1; }
	return read;
}

static int sock_write(void* ctx_, const unsigned char* buf, size_t len) {
	SSLContext* ctx = (SSLContext*)ctx_;
	cc_uint32 wrote;
	cc_result res = Socket_Write(ctx->socket, buf, len, &wrote);
	
	if (res) { ctx->writeError = res; return -1; }
	return wrote;
}

cc_result SSL_Init(cc_socket socket, const cc_string* host_, void** out_ctx) {
	SSLContext* ctx;
	char host[NATIVE_STR_LEN];
	String_EncodeUtf8(host, host_);
	
	ctx = (SSLContext*)Mem_TryAlloc(1, sizeof(SSLContext));
	if (!ctx) return ERR_OUT_OF_MEMORY;
	*out_ctx = (void*)ctx;
	
#if defined CC_BUILD_SYMBIAN
	{
		TAs[3].pkey.key.ec.curve = BR_EC_secp384r1;
		TAs[3].pkey.key.ec.q = (unsigned char *)TA3_EC_Q;
		TAs[3].pkey.key.ec.qlen = sizeof TA3_EC_Q;
		
		TAs[6].pkey.key.ec.curve = BR_EC_secp384r1;
		TAs[6].pkey.key.ec.q = (unsigned char *)TA6_EC_Q;
		TAs[6].pkey.key.ec.qlen = sizeof TA6_EC_Q;
	}
#endif
	
	br_ssl_client_init_full(&ctx->sc, &ctx->xc, TAs, TAs_NUM);
	InjectEntropy(ctx);
	br_x509_minimal_set_time_callback(&ctx->xc, NULL, ssl_time_check_callback);
	ctx->socket = socket;

	br_ssl_engine_set_buffer(&ctx->sc.eng, ctx->iobuf, sizeof(ctx->iobuf), 1);
	br_ssl_client_reset(&ctx->sc, host, 0);
	ctx->xc.vtable = &cert_verifier_vtable;
	
	/* Account login must be done over TLS 1.2 */
	if (String_CaselessEqualsConst(host_, "www.classicube.net")) {
		br_ssl_engine_set_versions(&ctx->sc.eng, BR_TLS12, BR_TLS12);
	}
	
	br_sslio_init(&ctx->ioc, &ctx->sc.eng, 
			sock_read,  ctx, 
			sock_write, ctx);
			
	ctx->readError  = 0;
	ctx->writeError = 0;
	
	return 0;
}

static CC_NOINLINE cc_result SSL_GetError(SSLContext* ctx) {
	int err;
	if (ctx->writeError) return ctx->writeError;
	if (ctx->readError)  return ctx->readError;
		
	// TODO session resumption, proper connection closing ??
	err = br_ssl_engine_last_error(&ctx->sc.eng);
	if (err == 0 && br_ssl_engine_current_state(&ctx->sc.eng) == BR_SSL_CLOSED)
		return SSL_ERR_CONTEXT_DEAD;
		
	return SSL_ERROR_SHIFT | (err & 0xFFFF);
}

cc_result SSL_Read(void* ctx_, cc_uint8* data, cc_uint32 count, cc_uint32* read) { 
	SSLContext* ctx = (SSLContext*)ctx_;
	// TODO: just br_sslio_write ??
	int res = br_sslio_read(&ctx->ioc, data, count);
	if (res < 0) return SSL_GetError(ctx);	
	
	br_sslio_flush(&ctx->ioc);
	*read = res;
	return 0;
}

cc_result SSL_WriteAll(void* ctx_, const cc_uint8* data, cc_uint32 count) {
	SSLContext* ctx = (SSLContext*)ctx_;
	// TODO: just br_sslio_write ??
	int res = br_sslio_write_all(&ctx->ioc, data, count);
	if (res < 0) return SSL_GetError(ctx);	
	
	br_sslio_flush(&ctx->ioc);
	return 0;
}

cc_result SSL_Free(void* ctx_) {
	SSLContext* ctx = (SSLContext*)ctx_;
	if (ctx) br_sslio_close(&ctx->ioc);
	
	Mem_Free(ctx_);
	return 0;
}
#else
void SSLBackend_Init(cc_bool verifyCerts) { }
cc_bool SSLBackend_DescribeError(cc_result res, cc_string* dst) { return false; }

cc_result SSL_Init(cc_socket socket, const cc_string* host, void** ctx) {
	return HTTP_ERR_NO_SSL;
}

cc_result SSL_Read(void* ctx, cc_uint8* data, cc_uint32 count, cc_uint32* read) { 
	return ERR_NOT_SUPPORTED; 
}

cc_result SSL_WriteAll(void* ctx, const cc_uint8* data, cc_uint32 count) { 
	return ERR_NOT_SUPPORTED; 
}

cc_result SSL_Free(void* ctx) { return 0; }
#endif
