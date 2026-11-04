#include "SSL.h"
#include "Errors.h"

#if CC_SSL_BACKEND == CC_SSL_BACKEND_BEARSSL
#include "String_.h"
#include "Certs.h"
#include "Funcs.h"
#include "../third_party/bearssl/bearssl.h"
#include "../misc/certs/certs.h"

// https://github.com/unkaktus/bearssl/blob/master/samples/client_basic.c#L283
#define SSL_ERROR_SHIFT 0xB5510000

typedef struct SSLContext {
	br_x509_minimal_context xc;
	br_ssl_client_context sc;
	struct X509CertContext x509;
	unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
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

cc_result SSL_Init(const cc_string* host_, void** out_ctx) {
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

	br_ssl_engine_set_buffer(&ctx->sc.eng, ctx->iobuf, sizeof(ctx->iobuf), 1);
	br_ssl_client_reset(&ctx->sc, host, 0);
	ctx->xc.vtable = &cert_verifier_vtable;
	
	/* Account login must be done over TLS 1.2 */
	if (String_CaselessEqualsConst(host_, "www.classicube.net")) {
		br_ssl_engine_set_versions(&ctx->sc.eng, BR_TLS12, BR_TLS12);
	}

	return 0;
}

static cc_result SSL_Run(br_ssl_engine_context* engine, struct CallbackIOState* io) {
	unsigned char* buf;
	size_t len;
	cc_result res;

	int state = br_ssl_engine_current_state(engine);

	if (state & BR_SSL_CLOSED) {
		int err = br_ssl_engine_last_error(engine);
		if (err == 0 && br_ssl_engine_current_state(engine) == BR_SSL_CLOSED)
			return SSL_ERR_CONTEXT_DEAD;
		
		return SSL_ERROR_SHIFT | (err & 0xFFFF);
	}

	/* any TLS record data to send takes top priority */
	if (state & BR_SSL_SENDREC) {
		buf = br_ssl_engine_sendrec_buf(engine, &len);
		cc_uint32 wrote = 0;

		res = io->SendData(io->io_ctx, buf, len, &wrote);
		if (res) return res;

		br_ssl_engine_sendrec_ack(engine, wrote);
	}

	/* able to send app data ? */
	if (io->send_left && (state & BR_SSL_SENDAPP)) {
		buf = br_ssl_engine_sendapp_buf(engine, &len);
		len = min(len, io->send_left);

		br_memcpy(buf, io->send_head, len);
		br_ssl_engine_sendapp_ack(engine, len);

		io->send_head += len;
		io->send_left -= len;
		br_ssl_engine_flush(engine, 0);
		return 0;		
	}

	/* able to read app data ? */
	if (state & BR_SSL_RECVAPP) {
		buf = br_ssl_engine_recvapp_buf(engine, &len);
		len = min(len, io->recv_size);

		br_memcpy(io->recv_buf, buf, len);
		br_ssl_engine_recvapp_ack(engine, len);

		if (io->recv_cb) io->recv_cb(io->ctx, io->recv_buf, len);
	}

	/* ready to read next TLS record? */
	if (state & BR_SSL_RECVREC) {
		buf = br_ssl_engine_recvrec_buf(engine, &len);
		cc_uint32 read = 0;

		res = io->RecvData(io->io_ctx, buf, len, &read);
		if (res) return res;

		br_ssl_engine_recvrec_ack(engine, read);
	}
	return 0;
}

void SSL_Close(void* ctx_, struct CallbackIOState* io_state) {
	SSLContext* ctx = (SSLContext*)ctx_;
	cc_result res;

	if (ctx && io_state) {
		br_ssl_engine_close(&ctx->sc.eng);

		while (br_ssl_engine_current_state(&ctx->sc.eng) != BR_SSL_CLOSED) {
			// TODO could this get stuck looping ?
			if ((res = SSL_Run(&ctx->sc.eng, io_state))) return;
		}
	}
}

cc_result SSL_Tick(void* ctx_, struct CallbackIOState* io_state) {
	SSLContext* ctx = (SSLContext*)ctx_;
	return SSL_Run(&ctx->sc.eng, io_state);
}

void SSL_Free(void* ctx_) {
	Mem_Free(ctx_);
}
#else
void SSLBackend_Init(cc_bool verifyCerts) { }
cc_bool SSLBackend_DescribeError(cc_result res, cc_string* dst) { return false; }

cc_result SSL_Init(const cc_string* host, void** ctx) {
	return HTTP_ERR_NO_SSL;
}

cc_result SSL_Tick(void* ctx, struct CallbackIOState* io_state) {
	return ERR_NOT_SUPPORTED;
}

void SSL_Close(void* ctx, struct CallbackIOState* io_state) { }
void SSL_Free (void* ctx) { }
#endif
