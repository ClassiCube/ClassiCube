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
#ifndef SGIP_IP_H
#define SGIP_IP_H

#include "sgIP_memblock.h"

#define PROTOCOL_IP_ICMP      1
#define PROTOCOL_IP_TCP       6
#define PROTOCOL_IP_UDP       17

typedef struct SGIP_HEADER_IP {
	unsigned char version_ihl; // version = top 4 bits == 4, IHL = header length in 32bit increments = bottom 4 bits
	unsigned char type_of_service; // [3bit prescidence][ D ][ T ][ R ][ 0 0 ] - D=low delya, T=high thoroughput, R= high reliability
	unsigned short tot_length; // total length of packet including header
	unsigned short identification; // value assigned by sender to aid in packet reassembly
	unsigned short fragment_offset; // top 3 bits are flags [0][DF][MF] (Don't Fragment / More Fragments Exist) - offset is in 8-byte chunks.
	unsigned char TTL; // time to live, measured in hops
	unsigned char protocol; // protocols: ICMP=1, TCP=6, UDP=17
	unsigned short header_checksum; // checksum:
	unsigned long src_address; // src address is 32bit IP address
	unsigned long dest_address; // dest address is 32bit IP address
	unsigned char options[4]; // optional options come here.
} sgIP_Header_IP;


#ifdef __cplusplus
extern "C" {
#endif

	extern int sgIP_IP_ReceivePacket(sgIP_memblock * mb);
	extern int sgIP_IP_MaxContentsSize(unsigned long destip);
	extern int sgIP_IP_RequiredHeaderSize();
	extern int sgIP_IP_SendViaIP(sgIP_memblock * mb, int protocol, unsigned long srcip, unsigned long destip);
	extern unsigned long sgIP_IP_GetLocalBindAddr(unsigned long srcip, unsigned long destip);

#ifdef __cplusplus
};
#endif


#endif
