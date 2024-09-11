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
#ifndef SGIP_ICMP_H
#define SGIP_ICMP_H

#include "sgIP_Config.h"
#include "sgIP_memblock.h"

typedef struct SGIP_HEADER_ICMP {
	unsigned char type,code;
	unsigned short checksum;
	unsigned long xtra;
} sgIP_Header_ICMP;

#ifdef __cplusplus
extern "C" {
#endif

   extern void sgIP_ICMP_Init();

   extern int sgIP_ICMP_ReceivePacket(sgIP_memblock * mb, unsigned long srcip, unsigned long destip);



#ifdef __cplusplus
};
#endif


#endif
