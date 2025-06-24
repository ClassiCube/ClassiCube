#ifndef CC_CERT_H
#define CC_CERT_H
#include "Core.h"
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
	int offset;
};

void Certs_BeginChain( struct X509CertContext* ctx);
void Certs_FreeChain(  struct X509CertContext* ctx);
int  Certs_VerifyChain(struct X509CertContext* ctx);

void Certs_BeginCert( struct X509CertContext* ctx, int size);
void Certs_AppendCert(struct X509CertContext* ctx, const void* data, int len);
void Certs_FinishCert(struct X509CertContext* ctx);

CC_END_HEADER
#endif
