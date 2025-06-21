// DSWifi Project - sgIP Internet Protocol Stack Implementation
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
/****************************************************************************** 
DSWifi Lib and test materials are licenced under the MIT open source licence:
Copyright (c) 2005-2006 Stephen Stair

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#ifndef SGIP_SOCKETS_H
#define SGIP_SOCKETS_H

#include "sgIP_Config.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "netdb.h"

#define SGIP_SOCKET_FLAG_ALLOCATED			0x8000
#define SGIP_SOCKET_FLAG_NONBLOCKING		0x4000
#define SGIP_SOCKET_FLAG_VALID				0x2000
#define SGIP_SOCKET_FLAG_CLOSING			0x1000
#define SGIP_SOCKET_FLAG_TYPEMASK			0x0001
#define SGIP_SOCKET_FLAG_TYPE_TCP			0x0001
#define SGIP_SOCKET_FLAG_TYPE_UDP			0x0000
#define SGIP_SOCKET_MASK_CLOSE_COUNT		0xFFFF0000
#define SGIP_SOCKET_SHIFT_CLOSE_COUNT		16

// Maybe define a better value for this in the future. But 5 minutes sounds ok.
// TCP specification disagrees on this point, but this is a limited platform.
// 5 minutes assuming 1000ms ticks = 300 = 0x12c
#define SGIP_SOCKET_VALUE_CLOSE_COUNT		(0x12c << SGIP_SOCKET_SHIFT_CLOSE_COUNT)

typedef struct SGIP_SOCKET_DATA {
	unsigned int flags;
	void * conn_ptr;
} sgIP_socket_data;

#ifdef __cplusplus
extern "C" {
#endif

	extern void sgIP_sockets_Init();
	extern void sgIP_sockets_Timer1000ms();

	// sys/socket.h
	extern int socket(int domain, int type, int protocol);
	extern int bind(int socket, const struct sockaddr * addr, int addr_len);
	extern int connect(int socket, const struct sockaddr * addr, int addr_len);
	extern int send(int socket, const void * data, int sendlength, int flags);
	extern int recv(int socket, void * data, int recvlength, int flags);
	extern int sendto(int socket, const void * data, int sendlength, int flags, const struct sockaddr * addr, int addr_len);
	extern int recvfrom(int socket, void * data, int recvlength, int flags, struct sockaddr * addr, int * addr_len);
	extern int listen(int socket, int max_connections);
	extern int accept(int socket, struct sockaddr * addr, int * addr_len);
	extern int shutdown(int socket, int shutdown_type);
	extern int closesocket(int socket);
	extern int forceclosesocket(int socket);

	extern int ioctl(int socket, long cmd, void * arg);

	extern int setsockopt(int socket, int level, int option_name, const void * data, int data_len);
	extern int getsockopt(int socket, int level, int option_name, void * data, int * data_len);

	extern int getpeername(int socket, struct sockaddr *addr, int * addr_len);
	extern int getsockname(int socket, struct sockaddr *addr, int * addr_len);

	// sys/time.h (actually intersects partly with libnds, so I'm letting libnds handle fd_set for the time being)
	extern int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);

	// arpa/inet.h
	extern unsigned long inet_addr(const char *cp);

#ifdef __cplusplus
};
#endif


#endif
