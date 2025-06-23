#ifndef CC_SSL_H
#define CC_SSL_H
#include "Platform.h"
CC_BEGIN_HEADER

/* 
Validates an X509 certificate chain for verifying a SSL/TLS connection.
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

void CertsBackend_Init(void);

struct X509CertContext {
	void* ctx;
	void* chain;
	void* cert;
};

cc_result Certs_BeginChain( struct X509CertContext* ctx);
cc_result Certs_FreeChain(  struct X509CertContext* ctx);
cc_result Certs_VerifyChain(struct X509CertContext* ctx);

void Certs_BeginCert( struct X509CertContext* ctx, int size);
void Certs_AppendCert(struct X509CertContext* ctx, void* data, int len);
void Certs_FinishCert(struct X509CertContext* ctx);

CC_END_HEADER
#endif
