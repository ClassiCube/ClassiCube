#include "SSL.h"
#include "Errors.h"

#if defined CC_BUILD_SCHANNEL
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOMINMAX
#include <windows.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <schannel.h>
#include "Platform.h"
#include "String.h"
#include "Funcs.h"

/* https://gist.github.com/mmozeiko/c0dfcc8fec527a90a02145d2cc0bfb6d */
/* https://web.archive.org/web/20210116110926/http://www.coastrd.com/c-schannel-smtp */

/* https://hpbn.co/transport-layer-security-tls/ */
#define TLS_MAX_PACKET_SIZE (16384 + 512) /* 16kb record size + header/mac/padding */
/* TODO: Check against sizes.cbMaximumMessage */

static void* schannel_lib;
static INIT_SECURITY_INTERFACE_A _InitSecurityInterfaceA;
static cc_bool _verifyCerts;

static ACQUIRE_CREDENTIALS_HANDLE_FN_A  FP_AcquireCredentialsHandleA;
static FREE_CREDENTIALS_HANDLE_FN       FP_FreeCredentialsHandle;
static INITIALIZE_SECURITY_CONTEXT_FN_A FP_InitializeSecurityContextA;
static ACCEPT_SECURITY_CONTEXT_FN       FP_AcceptSecurityContext;
static COMPLETE_AUTH_TOKEN_FN           FP_CompleteAuthToken;
static DELETE_SECURITY_CONTEXT_FN       FP_DeleteSecurityContext;
static QUERY_CONTEXT_ATTRIBUTES_FN_A    FP_QueryContextAttributesA;
static FREE_CONTEXT_BUFFER_FN           FP_FreeContextBuffer;
static ENCRYPT_MESSAGE_FN               FP_EncryptMessage;
static DECRYPT_MESSAGE_FN               FP_DecryptMessage;

void SSLBackend_Init(cc_bool verifyCerts) {
	/* secur32.dll is available on Win9x and later */
	/* Security.dll is available on NT 4 and later */

	/* Officially, InitSecurityInterfaceA and then AcquireCredentialsA from */
	/*  secur32.dll (or security.dll) should be called - however */
	/*  AcquireCredentialsA fails with SEC_E_SECPKG_NOT_FOUND on Win 9x */
	/* But if you instead directly call those functions from schannel.dll, */
	/*  then it DOES work. (and on later Windows versions, those functions */
	/*  exported from schannel.dll are just DLL forwards to secur32.dll */
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(InitSecurityInterfaceA)
	};
	static const cc_string schannel = String_FromConst("schannel.dll");
	_verifyCerts = verifyCerts;
	/* TODO: Load later?? it's unsafe to do on a background thread though */
	DynamicLib_LoadAll(&schannel, funcs, Array_Elems(funcs), &schannel_lib);
}

cc_bool SSLBackend_DescribeError(cc_result res, cc_string* dst) {
	switch (res) {
	case SEC_E_UNTRUSTED_ROOT:
		String_AppendConst(dst, "The website's SSL certificate was issued by an authority that is not trusted");
		return true;
	case SEC_E_CERT_EXPIRED:
		String_AppendConst(dst, "The website's SSL certificate has expired");
		return true;
	case TRUST_E_CERT_SIGNATURE:
		String_AppendConst(dst, "The signature of the website's SSL certificate cannot be verified");
		return true;
	case SEC_E_UNSUPPORTED_FUNCTION:
		/* https://learn.microsoft.com/en-us/windows/win32/secauthn/schannel-error-codes-for-tls-and-ssl-alerts */
		/* TLS1_ALERT_PROTOCOL_VERSION maps to this error code */
		String_AppendConst(dst, "The website uses an incompatible SSL/TLS version");
		return true;
	}
	return false;
}


struct SSLContext {
	cc_socket socket;
	CredHandle handle;
	CtxtHandle context;
	SecPkgContext_StreamSizes sizes;
	DWORD flags;
	int bufferLen;
	int leftover; /* number of unprocessed bytes leftover from last successful DecryptMessage */
	int decryptedSize;
	char* decryptedData;
	char incoming[TLS_MAX_PACKET_SIZE];
};

/* Undefined in older MinGW versions */
#define _SP_PROT_TLS1_1_CLIENT 0x00000200
#define _SP_PROT_TLS1_2_CLIENT 0x00000800

static SECURITY_STATUS SSL_CreateHandle(struct SSLContext* ctx) {
	SCHANNEL_CRED cred = { 0 };
	cred.dwVersion = SCHANNEL_CRED_VERSION;
	cred.dwFlags   = SCH_CRED_NO_DEFAULT_CREDS | (_verifyCerts ? SCH_CRED_AUTO_CRED_VALIDATION : SCH_CRED_MANUAL_CRED_VALIDATION);
	cred.grbitEnabledProtocols = SP_PROT_TLS1_CLIENT | _SP_PROT_TLS1_1_CLIENT | _SP_PROT_TLS1_2_CLIENT;

	/* TODO: SCHANNEL_NAME_A ? */
	return FP_AcquireCredentialsHandleA(NULL, UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
						&cred, NULL, NULL, &ctx->handle, NULL);
}

static cc_result SSL_SendRaw(cc_socket socket, const cc_uint8* data, cc_uint32 count) {
	cc_uint32 sent;
	cc_result res;

	while (count)
	{
		if ((res = Socket_Write(socket, data, count, &sent))) return res;
		if (!sent) return ERR_END_OF_STREAM;

		data  += sent;
		count -= sent;
	}
	return 0;
}

static cc_result SSL_RecvRaw(struct SSLContext* ctx) {
	cc_uint32 read;
	cc_result res;
	
	/* server is sending too much garbage data instead of proper TLS packets ?? */
	if (ctx->bufferLen == sizeof(ctx->incoming)) return ERR_INVALID_ARGUMENT;

	res = Socket_Read(ctx->socket, ctx->incoming + ctx->bufferLen,
						sizeof(ctx->incoming) - ctx->bufferLen, &read);

	if (res)   return res;
	if (!read) return ERR_END_OF_STREAM;

	ctx->bufferLen += read;
	return 0;
}


/* Sends the initial TLS handshake ClientHello message to the server */
static SECURITY_STATUS SSL_Connect(struct SSLContext* ctx, const char* hostname) {
	SecBuffer out_buffers[1];
	SecBufferDesc out_desc;
	SECURITY_STATUS res;
	DWORD flags = ctx->flags;

	out_buffers[0].BufferType = SECBUFFER_TOKEN;
	out_buffers[0].pvBuffer   = NULL;
	out_buffers[0].cbBuffer   = 0;

	out_desc.ulVersion = SECBUFFER_VERSION;
	out_desc.cBuffers  = Array_Elems(out_buffers);
	out_desc.pBuffers  = out_buffers;

	res = FP_InitializeSecurityContextA(&ctx->handle, NULL, hostname, flags, 0, 0,
						NULL, 0, &ctx->context, &out_desc, &flags, NULL);
	if (res != SEC_I_CONTINUE_NEEDED) return res;
	res = 0;

	/* Send initial handshake to the server (if there is one) */
	if (out_buffers[0].pvBuffer) {
		res = SSL_SendRaw(ctx->socket, out_buffers[0].pvBuffer, out_buffers[0].cbBuffer);
		FP_FreeContextBuffer(out_buffers[0].pvBuffer);
	}
	return res;
}

/* Performs (Negotiates) the rest of the TLS handshake */
static SECURITY_STATUS SSL_Negotiate(struct SSLContext* ctx) {
	SecBuffer in_buffers[2];
	SecBuffer out_buffers[1];
	SecBufferDesc in_desc;
	SecBufferDesc out_desc;
	cc_uint32 leftover_len;
	SECURITY_STATUS sec;
	cc_result res;
	DWORD flags;

	for (;;)
	{
		/* buffer 0 = data received from server which SChannel processes */
		/* buffer 1 = any leftover data which SChannel didn't process this time */
		/*  (this data must be persisted, as it will be used next time around) */
		in_buffers[0].BufferType = SECBUFFER_TOKEN;
		in_buffers[0].pvBuffer   = ctx->incoming;
		in_buffers[0].cbBuffer   = ctx->bufferLen;
		in_buffers[1].BufferType = SECBUFFER_EMPTY;
		in_buffers[1].pvBuffer   = NULL;
		in_buffers[1].cbBuffer   = 0;

		out_buffers[0].BufferType = SECBUFFER_TOKEN;
		out_buffers[0].pvBuffer   = NULL;
		out_buffers[0].cbBuffer   = 0;

		in_desc.ulVersion  = SECBUFFER_VERSION;
		in_desc.cBuffers   = Array_Elems(in_buffers);
		in_desc.pBuffers   = in_buffers;

		out_desc.ulVersion = SECBUFFER_VERSION;
		out_desc.cBuffers  = Array_Elems(out_buffers);
		out_desc.pBuffers  = out_buffers;

		flags = ctx->flags;
		sec   = FP_InitializeSecurityContextA(&ctx->handle, &ctx->context, NULL, flags, 0, 0,
							&in_desc, 0, NULL, &out_desc, &flags, NULL);

		if (in_buffers[1].BufferType == SECBUFFER_EXTRA) {
			/* SChannel didn't process the entirety of the input buffer */
			/*  So move the leftover data back to the front of the input buffer */
			leftover_len = in_buffers[1].cbBuffer;
			memmove(ctx->incoming, ctx->incoming + (ctx->bufferLen - leftover_len), leftover_len);
			ctx->bufferLen = leftover_len;
		} else if (sec != SEC_E_INCOMPLETE_MESSAGE) {
			/* SChannel processed entirely of input buffer */
			ctx->bufferLen = 0;
		}

		/* Handshake completed */
		if (sec == SEC_E_OK) break;

		/* Need to send data to the server */
		if (sec == SEC_I_CONTINUE_NEEDED) {
			res = SSL_SendRaw(ctx->socket, out_buffers[0].pvBuffer, out_buffers[0].cbBuffer);
			FP_FreeContextBuffer(out_buffers[0].pvBuffer); /* TODO always free? */

			if (res) return res;
			continue;
		}
		
		if (sec != SEC_E_INCOMPLETE_MESSAGE) return sec;
		/* SEC_E_INCOMPLETE_MESSAGE case - need to read more data from the server first */
		if ((res = SSL_RecvRaw(ctx))) return res;
	}

	FP_QueryContextAttributesA(&ctx->context, SECPKG_ATTR_STREAM_SIZES, &ctx->sizes);
	return 0;
}


static void SSL_LoadSecurityFunctions(PSecurityFunctionTableA sspiFPs) {
	FP_AcquireCredentialsHandleA  = sspiFPs->AcquireCredentialsHandleA;
	FP_FreeCredentialsHandle      = sspiFPs->FreeCredentialsHandle;
	FP_InitializeSecurityContextA = sspiFPs->InitializeSecurityContextA;
	FP_AcceptSecurityContext      = sspiFPs->AcceptSecurityContext;
	FP_CompleteAuthToken          = sspiFPs->CompleteAuthToken;
	FP_DeleteSecurityContext      = sspiFPs->DeleteSecurityContext;
	FP_QueryContextAttributesA    = sspiFPs->QueryContextAttributesA;
	FP_FreeContextBuffer          = sspiFPs->FreeContextBuffer;

	FP_EncryptMessage = sspiFPs->EncryptMessage;
	FP_DecryptMessage = sspiFPs->DecryptMessage;
	/* Old Windows versions don't have EncryptMessage/DecryptMessage, */
	/*  but have the older SealMessage/UnsealMessage functions instead */
	if (!FP_EncryptMessage) FP_EncryptMessage = (ENCRYPT_MESSAGE_FN)sspiFPs->Reserved3;
	if (!FP_DecryptMessage) FP_DecryptMessage = (DECRYPT_MESSAGE_FN)sspiFPs->Reserved4;
}

cc_result SSL_Init(cc_socket socket, const cc_string* host_, void** out_ctx) {
	PSecurityFunctionTableA sspiFPs;
	struct SSLContext* ctx;
	SECURITY_STATUS res;
	cc_winstring host;
	if (!_InitSecurityInterfaceA) return HTTP_ERR_NO_SSL;

	if (!FP_InitializeSecurityContextA) {
		sspiFPs = _InitSecurityInterfaceA();
		if (!sspiFPs) return ERR_NOT_SUPPORTED;
		SSL_LoadSecurityFunctions(sspiFPs);
	}

	ctx = Mem_TryAllocCleared(1, sizeof(struct SSLContext));
	if (!ctx) return ERR_OUT_OF_MEMORY;
	*out_ctx = (void*)ctx;

	ctx->flags = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_USE_SUPPLIED_CREDS | ISC_REQ_CONFIDENTIALITY | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;
	if (!_verifyCerts) ctx->flags |= ISC_REQ_MANUAL_CRED_VALIDATION;

	ctx->socket = socket;
	Platform_EncodeString(&host, host_);

	if ((res = SSL_CreateHandle(ctx)))	     return res;
	if ((res = SSL_Connect(ctx, host.ansi))) return res;
	if ((res = SSL_Negotiate(ctx)))		     return res;
	return 0;
}


static cc_result SSL_ReadDecrypted(struct SSLContext* ctx, cc_uint8* data, cc_uint32 count, cc_uint32* read) {
	int len = min(count, ctx->decryptedSize);
	Mem_Copy(data, ctx->decryptedData, len);

	if (len == ctx->decryptedSize) {
		/* incoming buffer stores decrypted data and then any leftover ciphertext */
		/*  So move the leftover ciphertext back to the start of the input buffer */
		/* TODO: Share function with handshake function */
		memmove(ctx->incoming, ctx->incoming + (ctx->bufferLen - ctx->leftover), ctx->leftover);
		ctx->bufferLen = ctx->leftover;
		ctx->leftover  = 0;

		ctx->decryptedData = NULL;
		ctx->decryptedSize = 0;
	} else {
		ctx->decryptedData += len;
		ctx->decryptedSize -= len;
	}

	*read = len;
	return 0;
}

cc_result SSL_Read(void* ctx_, cc_uint8* data, cc_uint32 count, cc_uint32* read) {
	struct SSLContext* ctx = ctx_;
	SecBuffer buffers[4];
	SecBufferDesc desc;
	SECURITY_STATUS sec;
	cc_result res;

	/* decrypted data from previously */
	if (ctx->decryptedData) return SSL_ReadDecrypted(ctx, data, count, read);

	for (;;)
	{
		/* if any ciphertext data, then try to decrypt it */
		if (ctx->bufferLen) {
			/* https://learn.microsoft.com/en-us/windows/win32/secauthn/stream-contexts */
			buffers[0].BufferType = SECBUFFER_DATA;
			buffers[0].pvBuffer   = ctx->incoming;
			buffers[0].cbBuffer   = ctx->bufferLen;
			buffers[1].BufferType = SECBUFFER_EMPTY;
			buffers[2].BufferType = SECBUFFER_EMPTY;
			buffers[3].BufferType = SECBUFFER_EMPTY;

			desc.ulVersion = SECBUFFER_VERSION;
			desc.cBuffers  = Array_Elems(buffers);
			desc.pBuffers  = buffers;

			sec = FP_DecryptMessage(&ctx->context, &desc, 0, NULL);
			if (sec == SEC_E_OK) {				
				/* After successful decryption the SecBuffers will be: */
				/*   buffers[0] = headers */
				/*   buffers[1] = content */
				/*   buffers[2] = trailers */
				/*   buffers[3] = extra, if any leftover unprocessed data */
				ctx->decryptedData = buffers[1].pvBuffer;
				ctx->decryptedSize = buffers[1].cbBuffer;
				ctx->leftover	   = buffers[3].BufferType == SECBUFFER_EXTRA ? buffers[3].cbBuffer : 0;

				return SSL_ReadDecrypted(ctx, data, count, read);
			}

			/* TODO properly close the connection with TLS shutdown when this happens */
			if (sec == SEC_I_CONTEXT_EXPIRED) return SSL_ERR_CONTEXT_DEAD;
			
			if (sec != SEC_E_INCOMPLETE_MESSAGE) return sec;
			/* SEC_E_INCOMPLETE_MESSAGE case - still need to read more data from the server first */
		}

		/* not enough data received yet to decrypt, so need to read more data from the server */
		if ((res = SSL_RecvRaw(ctx))) return res;
	}
	return 0;
}

static cc_result SSL_WriteChunk(struct SSLContext* s, const cc_uint8* data, cc_uint32 count) {
	char buffer[TLS_MAX_PACKET_SIZE];
	SecBuffer buffers[3];
	SecBufferDesc desc;
	SECURITY_STATUS res;
	int total;

	/* https://learn.microsoft.com/en-us/windows/win32/secauthn/encryptmessage--schannel */
	buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
	buffers[0].pvBuffer   = buffer;
	buffers[0].cbBuffer   = s->sizes.cbHeader;
	buffers[1].BufferType = SECBUFFER_DATA;
	buffers[1].pvBuffer   = buffer + s->sizes.cbHeader;
	buffers[1].cbBuffer   = count;
	buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
	buffers[2].pvBuffer   = buffer + s->sizes.cbHeader + count;
	buffers[2].cbBuffer   = s->sizes.cbTrailer;

	/* See https://learn.microsoft.com/en-us/windows/win32/api/sspi/nf-sspi-encryptmessage */
	/*  ".. The message is encrypted in place, overwriting the original contents of the structure" */
	Mem_Copy(buffers[1].pvBuffer, data, count);
	
	desc.ulVersion = SECBUFFER_VERSION;
	desc.cBuffers  = Array_Elems(buffers);
	desc.pBuffers  = buffers;
	if ((res = FP_EncryptMessage(&s->context, 0, &desc, 0))) return res;

	/* NOTE: Okay to write in one go, since all three buffers will be contiguous */
	/*  (as TLS record header size will always be the same size) */
	total = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
	return SSL_SendRaw(s->socket, buffer, total);
}

cc_result SSL_Write(void* ctx, const cc_uint8* data, cc_uint32 count, cc_uint32* wrote) {
	struct SSLContext* s = ctx;
	cc_result res;
	*wrote = 0;

	/* TODO: Don't loop here? move to HTTPConnection instead?? */
	while (count)
	{
		int len = min(count, s->sizes.cbMaximumMessage);
		if ((res = SSL_WriteChunk(s, data, len))) return res;

		*wrote += len;
		data   += len;
		count  -= len;
	}
	return 0;
}

cc_result SSL_Free(void* ctx_) {
	/* TODO send TLS close */
	struct SSLContext* ctx = (struct SSLContext*)ctx_;
	FP_DeleteSecurityContext(&ctx->context);
	FP_FreeCredentialsHandle(&ctx->handle);
	Mem_Free(ctx);
	return 0; 
}
#elif defined CC_BUILD_BEARSSL
#include "String.h"
#include "bearssl.h"
#include "../misc/certs.h"
// https://github.com/unkaktus/bearssl/blob/master/samples/client_basic.c#L283
#define SSL_ERROR_SHIFT 0xB5510000

typedef struct SSLContext {
	br_ssl_client_context sc;
	br_x509_minimal_context xc;
	unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
	br_sslio_context ioc;
	cc_result readError, writeError;
	cc_socket socket;
} SSLContext;

static cc_bool _verifyCerts;


void SSLBackend_Init(cc_bool verifyCerts) {
	_verifyCerts = verifyCerts; // TODO support
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

#if defined CC_BUILD_3DS
#include <3ds.h>
static void InjectEntropy(SSLContext* ctx) {
	char buf[32];
	PS_GenerateRandomBytes(buf, 32);
	// NOTE: PS_GenerateRandomBytes isn't implemented in Citra
	
	br_ssl_engine_inject_entropy(&ctx->sc.eng, buf, 32);
}
#else
#warning "Using uninitialised stack data for entropy. This should be replaced with actual cryptographic RNG data"
static void InjectEntropy(SSLContext* ctx) {
	char buf[32];
	// TODO: Use actual APIs to retrieve random data
	
	br_ssl_engine_inject_entropy(&ctx->sc.eng, buf, 32);
}
#endif

static void SetCurrentTime(SSLContext* ctx) {
	cc_uint64 cur = DateTime_CurrentUTC_MS() / 1000;
	uint32_t days = (uint32_t)(cur / 86400) + 366;
	uint32_t secs = (uint32_t)(cur % 86400);
		
	br_x509_minimal_set_time(&ctx->xc, days, secs);
	/* This matches bearssl's default time calculation
		time_t x = time(NULL);
		vd = (uint32_t)(x / 86400) + 719528;
		vs = (uint32_t)(x % 86400);
	 */
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
	
	ctx = Mem_TryAlloc(1, sizeof(SSLContext));
	if (!ctx) return ERR_OUT_OF_MEMORY;
	*out_ctx = (void*)ctx;
	
	br_ssl_client_init_full(&ctx->sc, &ctx->xc, TAs, TAs_NUM);
	/*if (!_verify_certs) {
		br_x509_minimal_set_rsa(&ctx->xc,   &br_rsa_i31_pkcs1_vrfy);
		br_x509_minimal_set_ecdsa(&ctx->xc, &br_ec_prime_i31, &br_ecdsa_i31_vrfy_asn1);
	}*/
	InjectEntropy(ctx);
	SetCurrentTime(ctx);
	ctx->socket = socket;

	br_ssl_engine_set_buffer(&ctx->sc.eng, ctx->iobuf, sizeof(ctx->iobuf), 1);
	br_ssl_client_reset(&ctx->sc, host, 0);
	
	br_sslio_init(&ctx->ioc, &ctx->sc.eng, 
			sock_read,  ctx, 
			sock_write, ctx);
			
	ctx->readError  = 0;
	ctx->writeError = 0;
	
	return 0;
}

cc_result SSL_Read(void* ctx_, cc_uint8* data, cc_uint32 count, cc_uint32* read) { 
	SSLContext* ctx = (SSLContext*)ctx_;
	// TODO: just br_sslio_write ??
	int res = br_sslio_read(&ctx->ioc, data, count);
	int err;
	
	if (res < 0) {
		if (ctx->readError) return ctx->readError;
		
		// TODO session resumption, proper connection closing ??
		err = br_ssl_engine_last_error(&ctx->sc.eng);
		if (err == 0 && br_ssl_engine_current_state(&ctx->sc.eng) == BR_SSL_CLOSED)
			return SSL_ERR_CONTEXT_DEAD;
		return SSL_ERROR_SHIFT + err;
	}
	
	br_sslio_flush(&ctx->ioc);
	*read = res;
	return 0;
}

cc_result SSL_Write(void* ctx_, const cc_uint8* data, cc_uint32 count, cc_uint32* wrote) {
	SSLContext* ctx = (SSLContext*)ctx_;
	// TODO: just br_sslio_write ??
	int res = br_sslio_write_all(&ctx->ioc, data, count);
	
	if (res < 0) {
		if (ctx->writeError) return ctx->writeError;
		return SSL_ERROR_SHIFT + br_ssl_engine_last_error(&ctx->sc.eng);
	}
	
	br_sslio_flush(&ctx->ioc);
	*wrote = res;
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

cc_result SSL_Write(void* ctx, const cc_uint8* data, cc_uint32 count, cc_uint32* wrote) { 
	return ERR_NOT_SUPPORTED; 
}

cc_result SSL_Free(void* ctx) { return 0; }
#endif
