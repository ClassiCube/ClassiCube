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


#ifndef SGIP_UDP_H
#define SGIP_UDP_H

#include "sgIP_Config.h"
#include "sgIP_memblock.h"


enum SGIP_UDP_STATE {
	SGIP_UDP_STATE_UNBOUND, // newly allocated
	SGIP_UDP_STATE_BOUND,	// got a source address/port
	SGIP_UDP_STATE_UNUSED,	// no longer in use.
};


typedef struct SGIP_HEADER_UDP {
	unsigned short srcport,destport;
	unsigned short length,checksum;
} sgIP_Header_UDP;

typedef struct SGIP_RECORD_UDP {
	struct SGIP_RECORD_UDP * next;

	int state;
	unsigned long srcip;
	unsigned long destip;
	unsigned short srcport,destport;
	
	sgIP_memblock * incoming_queue;
	sgIP_memblock * incoming_queue_end;

} sgIP_Record_UDP;

#ifdef __cplusplus
extern "C" {
#endif

	void sgIP_UDP_Init();

	int sgIP_UDP_CalcChecksum(sgIP_memblock * mb, unsigned long srcip, unsigned long destip, int totallength);
	int sgIP_UDP_ReceivePacket(sgIP_memblock * mb, unsigned long srcip, unsigned long destip);
	int sgIP_UDP_SendPacket(sgIP_Record_UDP * rec, const char * data, int datalen, unsigned long destip, int destport);

	sgIP_Record_UDP * sgIP_UDP_AllocRecord();
	void sgIP_UDP_FreeRecord(sgIP_Record_UDP * rec);

	int sgIP_UDP_Bind(sgIP_Record_UDP * rec, int srcport, unsigned long srcip);
	int sgIP_UDP_RecvFrom(sgIP_Record_UDP * rec, char * destbuf, int buflength, int flags, unsigned long * sender_ip, unsigned short * sender_port);
	int sgIP_UDP_SendTo(sgIP_Record_UDP * rec, const char * buf, int buflength, int flags, unsigned long dest_ip, int dest_port);

#ifdef __cplusplus
};
#endif


#endif
