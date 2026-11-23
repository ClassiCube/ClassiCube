#ifndef CC_SSL_H
#define CC_SSL_H
#include "Platform.h"
CC_BEGIN_HEADER

/* 
Wraps a socket connection in a TLS/SSL connection
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

struct CallbackIOState {
	void* ctx; /* receive callback context */
	cc_pointer io_ctx; /* I/O context */

	/* data waiting to be sent */
	cc_uint8* send_head;
	int       send_left;

	/* buffer where received data is written to */
	cc_uint8* recv_buf;
	int       recv_size;
	/* callback when data has been read */
	cc_result (*recv_cb)(void* ctx, cc_uint8* buf, cc_uint32 len);

	cc_result (*RecvData)(cc_pointer io_ctx, cc_uint8* data,       cc_uint32 count, cc_uint32* bytesRead);
	cc_result (*SendData)(cc_pointer io_ctx, const cc_uint8* data, cc_uint32 count, cc_uint32* bytesWrote);
};

void SSLBackend_Init(cc_bool verifyCerts);
cc_bool SSLBackend_DescribeError(cc_result res, cc_string* dst);

cc_result SSL_Init(const cc_string* host, void** ctx);
cc_result SSL_Tick(void* ctx, struct CallbackIOState* io_state);

void SSL_Close(void* ctx, struct CallbackIOState* io_state);
void SSL_Free (void* ctx);

CC_END_HEADER
#endif
