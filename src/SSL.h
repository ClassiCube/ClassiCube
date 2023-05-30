#ifndef CC_SSL_H
#define CC_SSL_H
#include "Platform.h"
/* 
Wraps a socket connection in a TLS/SSL connection
Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/

void SSLBackend_Init(cc_bool verifyCerts);
cc_bool SSLBackend_DescribeError(cc_result res, cc_string* dst);

cc_result SSL_Init(cc_socket socket, const cc_string* host, void** ctx);
cc_result SSL_Read(void* ctx, cc_uint8* data, cc_uint32 count, cc_uint32* read);
cc_result SSL_Write(void* ctx, const cc_uint8* data, cc_uint32 count, cc_uint32* wrote);
cc_result SSL_Free(void* ctx);
#endif
